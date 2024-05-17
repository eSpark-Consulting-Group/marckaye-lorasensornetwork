
#include "gps.h"
#include "sfe_bus.h"
#include "u-blox_config_keys.h"
#include "u-blox_external_typedefs.h"
#include "u-blox_Class_and_ID.h"
#include "u-blox_structs.h"

uint8_t gps_state;
int32_t gps_lat;
int32_t gps_lon;

#define GPS_ADDR 0x42 // Default 7-bit unshifted address of the ublox 6/7/8/M8/F9 series
#define MAXWAIT 1000
#define i2cTransactionSize 32
#define i2cPollingWait 100    // Default to 100ms. Adjusted when user calls setNavigationFrequency() or setHNRNavigationRate() or setMeasurementRate()

static unsigned long lastCheck = 0;

// Flag if this packet is unrequested (and so should be ignored and not copied into packetCfg or packetAck)
static bool ignoreThisPayload = false;

static bool _resetCurrentSentenceOnBusError = true;

static bool _signsOfLife = false;

static UBX_NAV_PVT_t packetUBXNAVPVT;           

// Identify which buffer is in use
// Data is stored in packetBuf until the requested class and ID can be validated
// If a match is seen, data is diverted into packetAck or packetCfg
//"Automatic" messages which have RAM allocated for them are diverted into packetAuto
static sfe_ublox_packet_buffer_e activePacketBuffer = SFE_UBLOX_PACKET_PACKETBUF;

static uint16_t ubxFrameCounter; // Count all UBX frame bytes. [Fixed header(2bytes), CLS(1byte), ID(1byte), length(2bytes), payload(x bytes), checksums(2bytes)]
static uint8_t rollingChecksumA; // Rolls forward as we receive incoming bytes. Checked against the last two A/B checksum bytes
static uint8_t rollingChecksumB; // Rolls forward as we receive incoming bytes. Checked against the last two A/B checksum bytes

static int8_t nmeaByteCounter; // Count all NMEA message bytes.
static uint8_t nmeaAddressField[6]; // NMEA Address Field - includes the start character (*)
static sfe_ublox_nmea_filtering_t _processNMEA; // Flags to indicate which NMEA messages should be passed to processNMEA

static uint16_t rtcmFrameCounter = 0; // Tracks the type of incoming byte inside RTCM frame

// Depending on the sentence type the processor will load characters into different arrays
enum sfe_ublox_sentence_types_e
{
	SFE_UBLOX_SENTENCE_TYPE_NONE = 0,
	SFE_UBLOX_SENTENCE_TYPE_NMEA,
	SFE_UBLOX_SENTENCE_TYPE_UBX,
	SFE_UBLOX_SENTENCE_TYPE_RTCM
} currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;

static uint8_t payloadAck[2];           // Holds the requested ACK/NACK
static uint8_t payloadBuf[2];           // Temporary buffer used to screen incoming packets or dump unrequested packets
static uint8_t payloadCfg[UBX_NAV_SAT_MAX_LEN];
static uint8_t payloadAuto[UBX_NAV_SAT_MAX_LEN];

static ubxPacket packetCfg = {0, 0, 0, 0, 0, payloadCfg, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
static ubxPacket packetAck = {0, 0, 0, 0, 0, payloadAck, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
static ubxPacket packetBuf = {0, 0, 0, 0, 0, payloadBuf, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
static ubxPacket packetAuto = {0, 0, 0, 0, 0, payloadAuto, 0, 0, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED, SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED};
	
static sfe_ublox_status_e sendCommand(ubxPacket *outgoingUBX, uint16_t maxWait, bool expectACKonly);
static bool checkUbloxInternal(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID);

extern U32 get_ticker(void);

static uint32_t millis(void)
{
	return get_ticker() *20 ;
}

extern void main_tasks(void);
	
static void delay(uint16_t ms)
{
	
		uint32_t t=millis();
		
		while(millis() - t < ms)
			main_tasks();
			
}

void GPS_init(void)
{
	gps_state=0;
}

// Given a message, calc and store the two byte "8-Bit Fletcher" checksum over the entirety of the message
// This is called before we send a command message
static void calcChecksum(ubxPacket *msg)
{
	msg->checksumA = 0;
	msg->checksumB = 0;

	msg->checksumA += msg->cls;
	msg->checksumB += msg->checksumA;

	msg->checksumA += msg->id;
	msg->checksumB += msg->checksumA;

	msg->checksumA += (msg->len & 0xFF);
	msg->checksumB += msg->checksumA;

	msg->checksumA += (msg->len >> 8);
	msg->checksumB += msg->checksumA;

	for (uint16_t i = 0; i < msg->len; i++)
	{
		msg->checksumA += msg->payload[i];
		msg->checksumB += msg->checksumA;
	}
}

// Returns false if sensor fails to respond to I2C traffic
static sfe_ublox_status_e sendI2cCommand(ubxPacket *outgoingUBX)
{
	uint8_t buf[i2cTransactionSize];
			
	// From the integration guide:
	// "The receiver does not provide any write access except for writing UBX and NMEA messages to the
	//  receiver, such as configuration or aiding data. Therefore, the register set mentioned in section Read
	//  Access is not writeable. Following the start condition from the master, the 7-bit device address and
	//  the RW bit (which is a logic low for write access) are clocked onto the bus by the master transmitter.
	//  The receiver answers with an acknowledge (logic low) to indicate that it is responsible for the given
	//  address. Now, the master can write 2 to N bytes to the receiver, generating a stop condition after the
	//  last byte being written. The number of data bytes must be at least 2 to properly distinguish from
	//  the write access to set the address counter in random read accesses."
	// I take two things from this:
	// 1) We do not need to write 0xFF to point at register 0xFF. We're already pointing at it.
	// 2) We must always write at least 2 bytes, otherwise it looks like we are starting to do a read.
	// Point 2 is important. It means:
	// * In this function:
	//     if we do multiple writes (because we're trying to write more than i2cTransactionSize),
	//     we may need to write one byte less in the penultimate write to ensure we always have two bytes left for the final write.
	// * In pushRawData:
	//     if there is one byte to write, or one byte left to write, we need to do the same thing and may need to store a single
	//     byte until pushRawData is called again.

	// The total number of bytes to be written is: payload len + 8
	// UBX_SYNCH_1
	// UBX_SYNCH_2
	// cls
	// id
	// len (MSB)
	// len (LSB)
	// < payload >
	// checksumA
	// checksumB

	// i2cTransactionSize will be at least 8. We don't need to check for smaller values than that.

	uint16_t bytesLeftToSend = outgoingUBX->len; // How many bytes remain to be sent
	uint16_t startSpot = 0;                      // Payload pointer

	// Check if we can send all the data in one transfer?
	if (bytesLeftToSend + 8 <= i2cTransactionSize)
	{

		buf[0] = UBX_SYNCH_1; // μ - oh ublox, you're funny. I will call you micro-blox from now on.
		buf[1] = UBX_SYNCH_2; // b
		buf[2] = outgoingUBX->cls;
		buf[3] = outgoingUBX->id;
		buf[4] = outgoingUBX->len & 0xFF; // LSB
		buf[5] = outgoingUBX->len >> 8;   // MSB
		uint16_t i = 0;
		for (; i < outgoingUBX->len; i++)
		buf[i + 6] = outgoingUBX->payload[startSpot + i];
		buf[i + 6] = outgoingUBX->checksumA;
		buf[i + 7] = outgoingUBX->checksumB;

		if (SfeI2C_writeBytes(buf, bytesLeftToSend + 8) != bytesLeftToSend + 8)
			return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK
	}

	else
	{
		buf[0] = UBX_SYNCH_1; // μ - oh ublox, you're funny. I will call you micro-blox from now on.
		buf[1] = UBX_SYNCH_2; // b
		buf[2] = outgoingUBX->cls;
		buf[3] = outgoingUBX->id;
		buf[4] = outgoingUBX->len & 0xFF; // LSB
		buf[5] = outgoingUBX->len >> 8;   // MSB

		if (SfeI2C_writeBytes(buf, 6) != 6)
			return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK

		// If bytesLeftToSend is zero, that's OK.
		// If bytesLeftToSend is >= 2, that's OK.
		// But if bytesLeftToSend is 1, we need to carry that byte over and send it with the checksum bytes
		while (bytesLeftToSend > 1)
		{
			uint16_t len = bytesLeftToSend; // How many bytes should we actually write?
			if (len > i2cTransactionSize)   // Limit len to i2cTransactionSize
			len = i2cTransactionSize;

			bytesLeftToSend -= len; // Calculate how many bytes will be left after we do this write

			// Write a portion of the payload to the bus.
			// Keep going until we've sent as many bytes as we can in this transmission (x == len)
			// or until we reach the end of the payload ((startSpot + x) == (outgoingUBX->len))
			uint16_t x = len;
			if ((startSpot + x) >= (outgoingUBX->len))
			x = outgoingUBX->len - startSpot;

			if (SfeI2C_writeBytes(&outgoingUBX->payload[startSpot], x) != x)
				return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK

			startSpot += x;
		}

		// Finally, write any left-over bytes plus the checksum
		if (bytesLeftToSend == 1)
		{
			buf[0] = outgoingUBX->payload[startSpot];
			buf[1] = outgoingUBX->checksumA;
			buf[2] = outgoingUBX->checksumB;

			if (SfeI2C_writeBytes(buf, 3) != 3)
				return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK
		}
		else
		{
			buf[0] = outgoingUBX->checksumA;
			buf[1] = outgoingUBX->checksumB;

			if (SfeI2C_writeBytes(buf, 2) != 2)
				return (SFE_UBLOX_STATUS_I2C_COMM_FAILURE); // Sensor did not ACK
		}
	}

	return (SFE_UBLOX_STATUS_SUCCESS);
}

// Get the latest Position/Velocity/Time solution and fill all global variables
static bool getPVT(uint16_t maxWait)
{
	
	if (packetUBXNAVPVT.automaticFlags.flags.bits.automatic && packetUBXNAVPVT.automaticFlags.flags.bits.implicitUpdate)
	{
		// The GPS is automatically reporting, we just check whether we got unread data
		checkUbloxInternal(&packetCfg, 0, 0); // Call checkUbloxInternal to parse any incoming data. Don't overwrite the requested Class and ID
		return packetUBXNAVPVT.moduleQueried.moduleQueried1.bits.all;
	}
	else if (packetUBXNAVPVT.automaticFlags.flags.bits.automatic && !packetUBXNAVPVT.automaticFlags.flags.bits.implicitUpdate)
	{
		// Someone else has to call checkUblox for us...
		return (false);
	}
	else
	{
		// The GPS is not automatically reporting navigation position so we have to poll explicitly
		packetCfg.cls = UBX_CLASS_NAV;
		packetCfg.id = UBX_NAV_PVT;
		packetCfg.len = 0;
		packetCfg.startingSpot = 0;
		// packetCfg.startingSpot = 20; //Begin listening at spot 20 so we can record up to 20+packetCfgPayloadSize = 84 bytes Note:now hard-coded in processUBX

		// The data is parsed as part of processing the response
		sfe_ublox_status_e retVal = sendCommand(&packetCfg, maxWait, true);

		if (retVal == SFE_UBLOX_STATUS_DATA_RECEIVED)
		return (true);

		if (retVal == SFE_UBLOX_STATUS_DATA_OVERWRITTEN)
		return (true);

		return (false);
	}
}

// Get the current longitude in degrees
// Returns a long representing the number of degrees *10^-7
static int32_t getLongitude(uint16_t maxWait)
{
	
	if (packetUBXNAVPVT.moduleQueried.moduleQueried1.bits.lon == false)
		getPVT(maxWait);
		
	packetUBXNAVPVT.moduleQueried.moduleQueried1.bits.lon = false; // Since we are about to give this to user, mark this data as stale
	packetUBXNAVPVT.moduleQueried.moduleQueried1.bits.all = false;
	return (packetUBXNAVPVT.data.lon);
}

// Get the current latitude in degrees
// Returns a long representing the number of degrees *10^-7
static int32_t getLatitude(uint16_t maxWait)
{
	
	if (packetUBXNAVPVT.moduleQueried.moduleQueried1.bits.lat == false)
		getPVT(maxWait);
	packetUBXNAVPVT.moduleQueried.moduleQueried1.bits.lat = false; // Since we are about to give this to user, mark this data as stale
	packetUBXNAVPVT.moduleQueried.moduleQueried1.bits.all = false;
	return (packetUBXNAVPVT.data.lat);
}


// PRIVATE: Check if we have storage allocated for an incoming "automatic" message
// Also calculate how much RAM is needed to store the payload for a given automatic message
static bool autoLookup(uint8_t Class, uint8_t ID, uint16_t *maxSize)
{
	if (maxSize != NULL)
		*maxSize = 0;

	switch (Class)
	{
		case UBX_CLASS_NAV:
			if (ID == UBX_NAV_PVT)
			{
				if (maxSize != NULL)
					*maxSize = UBX_NAV_PVT_LEN;
					
				return true;
			}
		
			break;
			
		default:
			return false;
		
	}
	return false;
}

// PRIVATE: Return true if the NMEA header is valid
static bool isNMEAHeaderValid(void)
{
	if (nmeaAddressField[0] != '*')
	return (false);
	if (nmeaAddressField[1] != 'G')
	return (false);
	if ((nmeaAddressField[3] == 'D') && (nmeaAddressField[4] == 'T') && (nmeaAddressField[5] == 'M'))
	return (true);
	if (nmeaAddressField[3] == 'G')
	{
		if ((nmeaAddressField[4] == 'A') && (nmeaAddressField[5] == 'Q'))
		return (true);
		if ((nmeaAddressField[4] == 'B') && (nmeaAddressField[5] == 'Q'))
		return (true);
		if ((nmeaAddressField[4] == 'B') && (nmeaAddressField[5] == 'S'))
		return (true);
		if ((nmeaAddressField[4] == 'G') && (nmeaAddressField[5] == 'A'))
		return (true);
		if ((nmeaAddressField[4] == 'L') && (nmeaAddressField[5] == 'L'))
		return (true);
		if ((nmeaAddressField[4] == 'L') && (nmeaAddressField[5] == 'Q'))
		return (true);
		if ((nmeaAddressField[4] == 'N') && (nmeaAddressField[5] == 'Q'))
		return (true);
		if ((nmeaAddressField[4] == 'N') && (nmeaAddressField[5] == 'S'))
		return (true);
		if ((nmeaAddressField[4] == 'P') && (nmeaAddressField[5] == 'Q'))
		return (true);
		if ((nmeaAddressField[4] == 'Q') && (nmeaAddressField[5] == 'Q'))
		return (true);
		if ((nmeaAddressField[4] == 'R') && (nmeaAddressField[5] == 'S'))
		return (true);
		if ((nmeaAddressField[4] == 'S') && (nmeaAddressField[5] == 'A'))
		return (true);
		if ((nmeaAddressField[4] == 'S') && (nmeaAddressField[5] == 'T'))
		return (true);
		if ((nmeaAddressField[4] == 'S') && (nmeaAddressField[5] == 'V'))
		return (true);
	}
	if ((nmeaAddressField[3] == 'R') && (nmeaAddressField[4] == 'L') && (nmeaAddressField[5] == 'M'))
	return (true);
	if ((nmeaAddressField[3] == 'R') && (nmeaAddressField[4] == 'M') && (nmeaAddressField[5] == 'C'))
	return (true);
	if ((nmeaAddressField[3] == 'T') && (nmeaAddressField[4] == 'H') && (nmeaAddressField[5] == 'S'))
	return (true);
	if ((nmeaAddressField[3] == 'T') && (nmeaAddressField[4] == 'X') && (nmeaAddressField[5] == 'T'))
	return (true);
	if ((nmeaAddressField[3] == 'V') && (nmeaAddressField[4] == 'L') && (nmeaAddressField[5] == 'W'))
	return (true);
	if ((nmeaAddressField[3] == 'V') && (nmeaAddressField[4] == 'T') && (nmeaAddressField[5] == 'G'))
	return (true);
	if ((nmeaAddressField[3] == 'Z') && (nmeaAddressField[4] == 'D') && (nmeaAddressField[5] == 'A'))
	return (true);
	return (false);
}

// PRIVATE: Return true if we should pass this NMEA message to processNMEA
static bool processThisNMEA(void )
{
	if (_processNMEA.bits.all == 1)
	return (true);
	if ((nmeaAddressField[3] == 'D') && (nmeaAddressField[4] == 'T') && (nmeaAddressField[5] == 'M') && (_processNMEA.bits.UBX_NMEA_DTM == 1))
	return (true);
	if (nmeaAddressField[3] == 'G')
	{
		if ((nmeaAddressField[4] == 'A') && (nmeaAddressField[5] == 'Q') && (_processNMEA.bits.UBX_NMEA_GAQ == 1))
		return (true);
		if ((nmeaAddressField[4] == 'B') && (nmeaAddressField[5] == 'Q') && (_processNMEA.bits.UBX_NMEA_GBQ == 1))
		return (true);
		if ((nmeaAddressField[4] == 'B') && (nmeaAddressField[5] == 'S') && (_processNMEA.bits.UBX_NMEA_GBS == 1))
		return (true);
		if ((nmeaAddressField[4] == 'G') && (nmeaAddressField[5] == 'A') && (_processNMEA.bits.UBX_NMEA_GGA == 1))
		return (true);
		if ((nmeaAddressField[4] == 'L') && (nmeaAddressField[5] == 'L') && (_processNMEA.bits.UBX_NMEA_GLL == 1))
		return (true);
		if ((nmeaAddressField[4] == 'L') && (nmeaAddressField[5] == 'Q') && (_processNMEA.bits.UBX_NMEA_GLQ == 1))
		return (true);
		if ((nmeaAddressField[4] == 'N') && (nmeaAddressField[5] == 'Q') && (_processNMEA.bits.UBX_NMEA_GNQ == 1))
		return (true);
		if ((nmeaAddressField[4] == 'N') && (nmeaAddressField[5] == 'S') && (_processNMEA.bits.UBX_NMEA_GNS == 1))
		return (true);
		if ((nmeaAddressField[4] == 'P') && (nmeaAddressField[5] == 'Q') && (_processNMEA.bits.UBX_NMEA_GPQ == 1))
		return (true);
		if ((nmeaAddressField[4] == 'Q') && (nmeaAddressField[5] == 'Q') && (_processNMEA.bits.UBX_NMEA_GQQ == 1))
		return (true);
		if ((nmeaAddressField[4] == 'R') && (nmeaAddressField[5] == 'S') && (_processNMEA.bits.UBX_NMEA_GRS == 1))
		return (true);
		if ((nmeaAddressField[4] == 'S') && (nmeaAddressField[5] == 'A') && (_processNMEA.bits.UBX_NMEA_GSA == 1))
		return (true);
		if ((nmeaAddressField[4] == 'S') && (nmeaAddressField[5] == 'T') && (_processNMEA.bits.UBX_NMEA_GST == 1))
		return (true);
		if ((nmeaAddressField[4] == 'S') && (nmeaAddressField[5] == 'V') && (_processNMEA.bits.UBX_NMEA_GSV == 1))
		return (true);
	}
	if ((nmeaAddressField[3] == 'R') && (nmeaAddressField[4] == 'L') && (nmeaAddressField[5] == 'M') && (_processNMEA.bits.UBX_NMEA_RLM == 1))
	return (true);
	if ((nmeaAddressField[3] == 'R') && (nmeaAddressField[4] == 'M') && (nmeaAddressField[5] == 'C') && (_processNMEA.bits.UBX_NMEA_RMC == 1))
	return (true);
	if ((nmeaAddressField[3] == 'T') && (nmeaAddressField[4] == 'H') && (nmeaAddressField[5] == 'S') && (_processNMEA.bits.UBX_NMEA_THS == 1))
	return (true);
	if ((nmeaAddressField[3] == 'T') && (nmeaAddressField[4] == 'X') && (nmeaAddressField[5] == 'T') && (_processNMEA.bits.UBX_NMEA_TXT == 1))
	return (true);
	if ((nmeaAddressField[3] == 'V') && (nmeaAddressField[4] == 'L') && (nmeaAddressField[5] == 'W') && (_processNMEA.bits.UBX_NMEA_VLW == 1))
	return (true);
	if ((nmeaAddressField[3] == 'V') && (nmeaAddressField[4] == 'T') && (nmeaAddressField[5] == 'G') && (_processNMEA.bits.UBX_NMEA_VTG == 1))
	return (true);
	if ((nmeaAddressField[3] == 'Z') && (nmeaAddressField[4] == 'D') && (nmeaAddressField[5] == 'A') && (_processNMEA.bits.UBX_NMEA_ZDA == 1))
	return (true);
	return (false);
}

// This is the default or generic NMEA processor. We're only going to pipe the data to serial port so we can see it.
// User could overwrite this function to pipe characters to nmea.process(c) of tinyGPS or MicroNMEA
// Or user could pipe each character to a buffer, radio, etc.
static void processNMEA(char incoming)
{
	
}

// This function is called for each byte of an RTCM frame
// Ths user can overwrite this function and process the RTCM frame as they please
// Bytes can be piped to Serial or other interface. The consumer could be a radio or the internet (Ntrip broadcaster)
static void processRTCM(uint8_t incoming)
{
	
}
// We need to be able to identify an RTCM packet and then the length
// so that we know when the RTCM message is completely received and we then start
// listening for other sentences (like NMEA or UBX)
// RTCM packet structure is very odd. I never found RTCM STANDARD 10403.2 but
// http://d1.amobbs.com/bbs_upload782111/files_39/ourdev_635123CK0HJT.pdf is good
// https://dspace.cvut.cz/bitstream/handle/10467/65205/F3-BP-2016-Shkalikava-Anastasiya-Prenos%20polohove%20informace%20prostrednictvim%20datove%20site.pdf?sequence=-1
// Lead me to: https://forum.u-blox.com/index.php/4348/how-to-read-rtcm-messages-from-neo-m8p
// RTCM 3.2 bytes look like this:
// Byte 0: Always 0xD3
// Byte 1: 6-bits of zero
// Byte 2: 10-bits of length of this packet including the first two-ish header bytes, + 6.
// byte 3 + 4 bits: Msg type 12 bits
// Example: D3 00 7C 43 F0 ... / 0x7C = 124+6 = 130 bytes in this packet, 0x43F = Msg type 1087
static enum sfe_ublox_sentence_types_e processRTCMframe(uint8_t incoming, uint16_t *_rtcmFrameCounter)
{
  static uint16_t rtcmLen = 0; // Static - length is retained between calls

  if (*_rtcmFrameCounter == 1)
  {
    rtcmLen = (incoming & 0x03) << 8; // Get the last two bits of this byte. Bits 8&9 of 10-bit length
  }
  else if (*_rtcmFrameCounter == 2)
  {
    rtcmLen |= incoming; // Bits 0-7 of packet length
    rtcmLen += 6;        // There are 6 additional bytes of what we presume is header, msgType, CRC, and stuff
  }
  /*else if (*_rtcmFrameCounter == 3)
  {
    rtcmMsgType = incoming << 4; //Message Type, MS 4 bits
  }
  else if (_*rtcmFrameCounter == 4)
  {
    rtcmMsgType |= (incoming >> 4); //Message Type, bits 0-7
  }*/

  *_rtcmFrameCounter = *_rtcmFrameCounter + 1; // Increment rtcmFrameCounter

  processRTCM(incoming); // Here is where we expose this byte to the user

  // If rtcmLen is not yet known, return SFE_UBLOX_SENTENCE_TYPE_RTCM
  if (*_rtcmFrameCounter <= 2) // If this is header byte 0 or 1 (rtcmFrameCounter has been incremented)
    return SFE_UBLOX_SENTENCE_TYPE_RTCM;

  // Reset and start looking for next sentence type when done
  return (*_rtcmFrameCounter == rtcmLen) ? SFE_UBLOX_SENTENCE_TYPE_NONE : SFE_UBLOX_SENTENCE_TYPE_RTCM;
}

static uint8_t extractByte(ubxPacket *msg, uint16_t spotToStart)
{
	return (msg->payload[spotToStart]);
}

// Given a spot in the payload array, extract four bytes and build a long
static uint32_t extractLong(ubxPacket *msg, uint16_t spotToStart)
{
	uint32_t val = 0;
	for (uint8_t i = 0; i < 4; i++)
	val |= (uint32_t)msg->payload[spotToStart + i] << (8 * i);
	return (val);
}

// Just so there is no ambiguity about whether a uint32_t will cast to a int32_t correctly...
static int32_t extractSignedLong(ubxPacket *msg, uint16_t spotToStart)
{
	return (int32_t)extractLong(msg, spotToStart);
}

// Given a spot in the payload array, extract two bytes and build an int
static uint16_t extractInt(ubxPacket *msg, uint16_t spotToStart)
{
	uint16_t val = (uint16_t)msg->payload[spotToStart + 0] << 8 * 0;
	val |= (uint16_t)msg->payload[spotToStart + 1] << 8 * 1;
	return (val);
}

// Just so there is no ambiguity about whether a uint16_t will cast to a int16_t correctly...
static int16_t extractSignedInt(ubxPacket *msg, uint16_t spotToStart)
{
	return (int16_t)extractInt(msg, spotToStart);
}

// Once a packet has been received and validated, identify this packet's class/id and update internal flags
static void processUBXpacket(ubxPacket *msg)
{
	switch (msg->cls)
	{
		case UBX_CLASS_NAV:
		
		if (msg->id == UBX_NAV_PVT && msg->len == UBX_NAV_PVT_LEN)
		{
			// Parse various byte fields into storage - but only if we have memory allocated for it
			
				packetUBXNAVPVT.data.iTOW = extractLong(msg, 0);
				packetUBXNAVPVT.data.year = extractInt(msg, 4);
				packetUBXNAVPVT.data.month = extractByte(msg, 6);
				packetUBXNAVPVT.data.day = extractByte(msg, 7);
				packetUBXNAVPVT.data.hour = extractByte(msg, 8);
				packetUBXNAVPVT.data.min = extractByte(msg, 9);
				packetUBXNAVPVT.data.sec = extractByte(msg, 10);
				packetUBXNAVPVT.data.valid.all = extractByte(msg, 11);
				packetUBXNAVPVT.data.tAcc = extractLong(msg, 12);
				packetUBXNAVPVT.data.nano = extractSignedLong(msg, 16); // Includes milliseconds
				packetUBXNAVPVT.data.fixType = extractByte(msg, 20);
				packetUBXNAVPVT.data.flags.all = extractByte(msg, 21);
				packetUBXNAVPVT.data.flags2.all = extractByte(msg, 22);
				packetUBXNAVPVT.data.numSV = extractByte(msg, 23);
				packetUBXNAVPVT.data.lon = extractSignedLong(msg, 24);
				packetUBXNAVPVT.data.lat = extractSignedLong(msg, 28);
				packetUBXNAVPVT.data.height = extractSignedLong(msg, 32);
				packetUBXNAVPVT.data.hMSL = extractSignedLong(msg, 36);
				packetUBXNAVPVT.data.hAcc = extractLong(msg, 40);
				packetUBXNAVPVT.data.vAcc = extractLong(msg, 44);
				packetUBXNAVPVT.data.velN = extractSignedLong(msg, 48);
				packetUBXNAVPVT.data.velE = extractSignedLong(msg, 52);
				packetUBXNAVPVT.data.velD = extractSignedLong(msg, 56);
				packetUBXNAVPVT.data.gSpeed = extractSignedLong(msg, 60);
				packetUBXNAVPVT.data.headMot = extractSignedLong(msg, 64);
				packetUBXNAVPVT.data.sAcc = extractLong(msg, 68);
				packetUBXNAVPVT.data.headAcc = extractLong(msg, 72);
				packetUBXNAVPVT.data.pDOP = extractInt(msg, 76);
				packetUBXNAVPVT.data.flags3.all = extractByte(msg, 78);
				packetUBXNAVPVT.data.headVeh = extractSignedLong(msg, 84);
				packetUBXNAVPVT.data.magDec = extractSignedInt(msg, 88);
				packetUBXNAVPVT.data.magAcc = extractInt(msg, 90);

				// Mark all datums as fresh (not read before)
				packetUBXNAVPVT.moduleQueried.moduleQueried1.all = 0xFFFFFFFF;
				packetUBXNAVPVT.moduleQueried.moduleQueried2.all = 0xFFFFFFFF;
			
		}
		break;
	}
}

// Given a message and a byte, add to rolling "8-Bit Fletcher" checksum
// This is used when receiving messages from module
static void addToChecksum(uint8_t incoming)
{
	rollingChecksumA += incoming;
	rollingChecksumB += rollingChecksumA;
}

// Given a character, file it away into the uxb packet structure
// Set valid to VALID or NOT_VALID once sentence is completely received and passes or fails CRC
// The payload portion of the packet can be 100s of bytes but the max array size is packetCfgPayloadSize bytes.
// startingSpot can be set so we only record a subset of bytes within a larger packet.
static void processUBX(uint8_t incoming, ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
	// If incomingUBX is a user-defined custom packet, then the payload size could be different to packetCfgPayloadSize.
	// TO DO: update this to prevent an overrun when receiving an automatic message
	//        and the incomingUBX payload size is smaller than packetCfgPayloadSize.
	uint16_t maximum_payload_size;
	if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG)
		maximum_payload_size = UBX_NAV_SAT_MAX_LEN;
	else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETAUTO)
	{
		// Calculate maximum payload size once Class and ID have been received
		// (This check is probably redundant as activePacketBuffer can only be SFE_UBLOX_PACKET_PACKETAUTO
		//  when ubxFrameCounter >= 3)
		// if (incomingUBX->counter >= 2)
		//{

		bool logBecauseAuto = autoLookup(incomingUBX->cls, incomingUBX->id, &maximum_payload_size);
		bool logBecauseEnabled = false;
		if ((!logBecauseAuto) && (logBecauseEnabled))
		maximum_payload_size = UBX_NAV_SAT_MAX_LEN;
		if (maximum_payload_size == 0)
		{
			
		}
		//}
		// else
		//  maximum_payload_size = 2;
	}
	else
	maximum_payload_size = 2;

	bool overrun = false;

	// Add all incoming bytes to the rolling checksum
	// Stop at len+4 as this is the checksum bytes to that should not be added to the rolling checksum
	if (incomingUBX->counter < (incomingUBX->len + 4))
	addToChecksum(incoming);

	if (incomingUBX->counter == 0)
	{
		incomingUBX->cls = incoming;
	}
	else if (incomingUBX->counter == 1)
	{
		incomingUBX->id = incoming;
	}
	else if (incomingUBX->counter == 2) // Len LSB
	{
		incomingUBX->len = incoming;
	}
	else if (incomingUBX->counter == 3) // Len MSB
	{
		incomingUBX->len |= incoming << 8;
	}
	else if (incomingUBX->counter == incomingUBX->len + 4) // ChecksumA
	{
		incomingUBX->checksumA = incoming;
	}
	else if (incomingUBX->counter == incomingUBX->len + 5) // ChecksumB
	{
		incomingUBX->checksumB = incoming;

		currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // We're done! Reset the sentence to being looking for a new start char

		// Validate this sentence
		if ((incomingUBX->checksumA == rollingChecksumA) && (incomingUBX->checksumB == rollingChecksumB))
		{
			incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_VALID; // Flag the packet as valid
			_signsOfLife = true;                                  // The checksum is valid, so set the _signsOfLife flag

			// Let's check if the class and ID match the requestedClass and requestedID
			// Remember - this could be a data packet or an ACK packet
			if ((incomingUBX->cls == requestedClass) && (incomingUBX->id == requestedID))
			{
				incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID; // If we have a match, set the classAndIDmatch flag to valid
			}

			// If this is an ACK then let's check if the class and ID match the requestedClass and requestedID
			else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->id == UBX_ACK_ACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
			{
				incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_VALID; // If we have a match, set the classAndIDmatch flag to valid
			}

			// If this is a NACK then let's check if the class and ID match the requestedClass and requestedID
			else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->id == UBX_ACK_NACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
			{
				incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_NOTACKNOWLEDGED; // If we have a match, set the classAndIDmatch flag to NOTACKNOWLEDGED
				
			}

			// This is not an ACK and we do not have a complete class and ID match
			// So let's check for an "automatic" message arriving
			else if (autoLookup(incomingUBX->cls, incomingUBX->id,NULL)) 
			{
				
			}

			// We've got a valid packet, now do something with it but only if ignoreThisPayload is false
			if (ignoreThisPayload == false)
			{
				processUBXpacket(incomingUBX);
			}
		}
		else // Checksum failure
		{
			incomingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID;

			// Let's check if the class and ID match the requestedClass and requestedID.
			// This is potentially risky as we are saying that we saw the requested Class and ID
			// but that the packet checksum failed. Potentially it could be the class or ID bytes
			// that caused the checksum error!
			if ((incomingUBX->cls == requestedClass) && (incomingUBX->id == requestedID))
			{
				incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID; // If we have a match, set the classAndIDmatch flag to not valid
			}
			// If this is an ACK then let's check if the class and ID match the requestedClass and requestedID
			else if ((incomingUBX->cls == UBX_CLASS_ACK) && (incomingUBX->payload[0] == requestedClass) && (incomingUBX->payload[1] == requestedID))
			{
				incomingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_VALID; // If we have a match, set the classAndIDmatch flag to not valid
			}
		}

		// Now that the packet is complete and has been processed, we need to delete the memory
		// allocated for packetAuto
		if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETAUTO)
		{
			
		}
	}
	else // Load this byte into the payload array
	{
		// If an automatic packet comes in asynchronously, we need to fudge the startingSpot
		uint16_t startingSpot = incomingUBX->startingSpot, maxlen;
		
		if (autoLookup(incomingUBX->cls, incomingUBX->id,&maxlen))
		startingSpot = 0;
		// Check if this is payload data which should be ignored
		if (ignoreThisPayload == false)
		{
			// Begin recording if counter goes past startingSpot
			if ((incomingUBX->counter - 4) >= startingSpot)
			{
				// Check to see if we have room for this byte
				if (((incomingUBX->counter - 4) - startingSpot) < maximum_payload_size) // If counter = 208, starting spot = 200, we're good to record.
				{
					incomingUBX->payload[(incomingUBX->counter - 4) - startingSpot] = incoming; // Store this byte into payload array
				}
				else
				{
					overrun = true;
				}
			}
		}
	}

	// incomingUBX->counter should never reach maximum_payload_size + class + id + len[2] + checksum[2]
	if (overrun || ((incomingUBX->counter == maximum_payload_size + 6) && (ignoreThisPayload == false)))
	{
		// Something has gone very wrong
		currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Reset the sentence to being looking for a new start char
		
	}

	// Increment the counter
	incomingUBX->counter++;
}


// Processes NMEA, RTCM and UBX binary sentences one byte at a time
// Take a given byte and file it into the proper array
static void process(uint8_t incoming, ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
	// Update storedClass and storedID if either requestedClass or requestedID is non-zero,
	// otherwise leave unchanged. This allows calls of checkUblox() (which defaults to checkUblox(0,0))
	// by other threads without overwriting the requested / expected Class and ID.
	volatile static uint8_t storedClass = 0;
	volatile static uint8_t storedID = 0;
	if (requestedClass || requestedID) // If either is non-zero, store the requested Class and ID
	{
		storedClass = requestedClass;
		storedID = requestedID;
	}

	if ((currentSentence == SFE_UBLOX_SENTENCE_TYPE_NONE) || (currentSentence == SFE_UBLOX_SENTENCE_TYPE_NMEA))
	{
		if (incoming == UBX_SYNCH_1) // UBX binary frames start with 0xB5, aka μ
		{
			// This is the start of a binary sentence. Reset flags.
			// We still don't know the response class
			ubxFrameCounter = 0;
			currentSentence = SFE_UBLOX_SENTENCE_TYPE_UBX;
			// Reset the packetBuf.counter even though we will need to reset it again when ubxFrameCounter == 2
			packetBuf.counter = 0;
			ignoreThisPayload = false; // We should not ignore this payload - yet
			// Store data in packetBuf until we know if we have a stored class and ID match
			activePacketBuffer = SFE_UBLOX_PACKET_PACKETBUF;
		}
		else if (incoming == '$')
		{
			nmeaByteCounter = 0; // Reset the NMEA byte counter
			currentSentence = SFE_UBLOX_SENTENCE_TYPE_NMEA;
		}
		else if (incoming == 0xD3) // RTCM frames start with 0xD3
		{
			rtcmFrameCounter = 0;
			currentSentence = SFE_UBLOX_SENTENCE_TYPE_RTCM;
		}
		else
		{
			// This character is unknown or we missed the previous start of a sentence
			
		}
	}

	uint16_t maxPayload = 0;

	// Depending on the sentence, pass the character to the individual processor
	if (currentSentence == SFE_UBLOX_SENTENCE_TYPE_UBX)
	{
		// Decide what type of response this is
		if ((ubxFrameCounter == 0) && (incoming != UBX_SYNCH_1))      // ISO 'μ'
		currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;             // Something went wrong. Reset.
		else if ((ubxFrameCounter == 1) && (incoming != UBX_SYNCH_2)) // ASCII 'b'
		currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE;             // Something went wrong. Reset.
		// Note to future self:
		// There may be some duplication / redundancy in the next few lines as processUBX will also
		// load information into packetBuf, but we'll do it here too for clarity
		else if (ubxFrameCounter == 2) // Class
		{
			// Record the class in packetBuf until we know what to do with it
			packetBuf.cls = incoming; // (Duplication)
			rollingChecksumA = 0;     // Reset our rolling checksums here (not when we receive the 0xB5)
			rollingChecksumB = 0;
			packetBuf.counter = 0;                                   // Reset the packetBuf.counter (again)
			packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // Reset the packet validity (redundant?)
			packetBuf.startingSpot = incomingUBX->startingSpot;      // Copy the startingSpot
		}
		else if (ubxFrameCounter == 3) // ID
		{
			// Record the ID in packetBuf until we know what to do with it
			packetBuf.id = incoming; // (Duplication)
			// We can now identify the type of response
			// If the packet we are receiving is not an ACK then check for a class and ID match
			if (packetBuf.cls != UBX_CLASS_ACK)
			{
				bool logBecauseAuto = autoLookup(packetBuf.cls, packetBuf.id, &maxPayload);
				bool logBecauseEnabled = false; // logThisUBX(packetBuf.cls, packetBuf.id);

				// This is not an ACK so check for a class and ID match
				if ((packetBuf.cls == storedClass) && (packetBuf.id == storedID))
				{
					// This is not an ACK and we have a class and ID match
					// So start diverting data into incomingUBX (usually packetCfg)
					activePacketBuffer = SFE_UBLOX_PACKET_PACKETCFG;
					incomingUBX->cls = packetBuf.cls; // Copy the class and ID into incomingUBX (usually packetCfg)
					incomingUBX->id = packetBuf.id;
					incomingUBX->counter = packetBuf.counter; // Copy over the .counter too
				}
				// This is not an ACK and we do not have a complete class and ID match
				// So let's check if this is an "automatic" message which has its own storage defined
				else if (logBecauseAuto || logBecauseEnabled)
				{
					
					if ((!logBecauseAuto) && (logBecauseEnabled))
					{	maxPayload = UBX_NAV_SAT_MAX_LEN; }
					
						// The RAM allocation was successful so we start diverting data into packetAuto and process it
						activePacketBuffer = SFE_UBLOX_PACKET_PACKETAUTO;
						packetAuto.cls = packetBuf.cls; // Copy the class and ID into packetAuto
						packetAuto.id = packetBuf.id;
						packetAuto.counter = packetBuf.counter;           // Copy over the .counter too
						packetAuto.startingSpot = packetBuf.startingSpot; // And the starting spot? (Probably redundant)
						
					
				}
				else
				{
					// This is not an ACK and we do not have a class and ID match
					// so we should keep diverting data into packetBuf and ignore the payload
					ignoreThisPayload = true;
				}
			}
			else
			{
				// This is an ACK so it is to early to do anything with it
				// We need to wait until we have received the length and data bytes
				// So we should keep diverting data into packetBuf
			}
		}
		else if (ubxFrameCounter == 4) // Length LSB
		{
			// We should save the length in packetBuf even if activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG
			packetBuf.len = incoming; // (Duplication)
		}
		else if (ubxFrameCounter == 5) // Length MSB
		{
			// We should save the length in packetBuf even if activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG
			packetBuf.len |= incoming << 8; // (Duplication)
		}
		else if (ubxFrameCounter == 6) // This should be the first byte of the payload unless .len is zero
		{
			if (packetBuf.len == 0) // Check if length is zero (hopefully this is impossible!)
			{
				
				// If length is zero (!) this will be the first byte of the checksum so record it
				packetBuf.checksumA = incoming;
			}
			else
			{
				// The length is not zero so record this byte in the payload
				packetBuf.payload[0] = incoming;
			}
		}
		else if (ubxFrameCounter == 7) // This should be the second byte of the payload unless .len is zero or one
		{
			if (packetBuf.len == 0) // Check if length is zero (hopefully this is impossible!)
			{
				// If length is zero (!) this will be the second byte of the checksum so record it
				packetBuf.checksumB = incoming;
			}
			else if (packetBuf.len == 1) // Check if length is one
			{
				// The length is one so this is the first byte of the checksum
				packetBuf.checksumA = incoming;
			}
			else // Length is >= 2 so this must be a payload byte
			{
				packetBuf.payload[1] = incoming;
			}
			// Now that we have received two payload bytes, we can check for a matching ACK/NACK
			if ((activePacketBuffer == SFE_UBLOX_PACKET_PACKETBUF) // If we are not already processing a data packet
			&& (packetBuf.cls == UBX_CLASS_ACK)                // and if this is an ACK/NACK
			&& (packetBuf.payload[0] == storedClass)           // and if the class matches
			&& (packetBuf.payload[1] == storedID))             // and if the ID matches
			{
				if (packetBuf.len == 2) // Check if .len is 2
				{
					// Then this is a matching ACK so copy it into packetAck
					activePacketBuffer = SFE_UBLOX_PACKET_PACKETACK;
					packetAck.cls = packetBuf.cls;
					packetAck.id = packetBuf.id;
					packetAck.len = packetBuf.len;
					packetAck.counter = packetBuf.counter;
					packetAck.payload[0] = packetBuf.payload[0];
					packetAck.payload[1] = packetBuf.payload[1];
				}
				else // Length is not 2 (hopefully this is impossible!)
				{
					
				}
			}
		}

		// Divert incoming into the correct buffer
		if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETACK)
		processUBX(incoming, &packetAck, storedClass, storedID);
		else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETCFG)
		processUBX(incoming, incomingUBX, storedClass, storedID);
		else if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETBUF)
		processUBX(incoming, &packetBuf, storedClass, storedID);
		else // if (activePacketBuffer == SFE_UBLOX_PACKET_PACKETAUTO)
		processUBX(incoming, &packetAuto, storedClass, storedID);

		// Finally, increment the frame counter
		ubxFrameCounter++;
	}
	else if (currentSentence == SFE_UBLOX_SENTENCE_TYPE_NMEA) // Process incoming NMEA mesages. Selectively log if desired.
	{
		if ((nmeaByteCounter == 0) && (incoming != '$'))
		{
			currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Something went wrong. Reset. (Almost certainly redundant!)
		}
		else if ((nmeaByteCounter == 1) && (incoming != 'G'))
		{
			currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Something went wrong. Reset.
		}
		else if ((nmeaByteCounter >= 0) && (nmeaByteCounter <= 5))
		{
			nmeaAddressField[nmeaByteCounter] = incoming; // Store the start character and NMEA address field
		}

		if (nmeaByteCounter == 5)
		{
			if (!_signsOfLife) // If _signsOfLife is not already true, set _signsOfLife to true if the NMEA header is valid
			{
				_signsOfLife = isNMEAHeaderValid();
			}

			if (processThisNMEA())
			{
				for (uint8_t i = 0; i < 6; i++)
				{
					processNMEA(nmeaAddressField[i]); // Process the start character and address field
					
				}
			}
		}

		if ((nmeaByteCounter > 5) || (nmeaByteCounter < 0)) // Should we add incoming to the file buffer and/or pass it to processNMEA?
		{
			
			if (processThisNMEA())
			{
				processNMEA(incoming); // Pass incoming to processNMEA
				
			}
		}

		if (incoming == '*')
		nmeaByteCounter = -5; // We are expecting * plus two checksum bytes plus CR and LF

		nmeaByteCounter++; // Increment the byte counter

		if (nmeaByteCounter == SFE_UBLOX_MAX_NMEA_BYTE_COUNT)          // Check if we have processed too many bytes
		currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Something went wrong. Reset.

		if (nmeaByteCounter == 0) // Check if we are done
		{
			
			currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // All done!
		}
	}
	else if (currentSentence == SFE_UBLOX_SENTENCE_TYPE_RTCM)
	{
		currentSentence = processRTCMframe(incoming, &rtcmFrameCounter); // Deal with RTCM bytes
	}
}

// Returns true if new bytes are available
static bool checkUbloxI2C(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
	uint8_t buf[i2cTransactionSize];
				
	if (millis() - lastCheck >= i2cPollingWait)
	{
		// Get the number of bytes available from the module
		// From the u-blox integration manual:
		// "There are two forms of DDC read transfer. The "random access" form includes a peripheral register
		//  address and thus allows any register to be read. The second "current address" form omits the
		//  register address. If this second form is used, then an address pointer in the receiver is used to
		//  determine which register to read. This address pointer will increment after each read unless it
		//  is already pointing at register 0xFF, the highest addressable register, in which case it remains
		//  unaltered."
		uint16_t bytesAvailable = SfeI2C_available();

		if (bytesAvailable == 0)
		{
			// #ifndef SFE_UBLOX_REDUCED_PROG_MEM
			//       if (_printDebug == true)
			//       {
			//         _debugSerial.println(F("checkUbloxI2C: OK, zero bytes available"));
			//       }
			// #endif
			lastCheck = millis(); // Put off checking to avoid I2C bus traffic
			return (false);
		}

		// Check for undocumented bit error. We found this doing logic scans.
		// This error is rare but if we incorrectly interpret the first bit of the two 'data available' bytes as 1
		// then we have far too many bytes to check. May be related to I2C setup time violations: https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library/issues/40
		if (bytesAvailable & ((uint16_t)1 << 15))
		{
			// Clear the MSbit
			bytesAvailable &= ~((uint16_t)1 << 15);
		}

		while (bytesAvailable)
		{
			// Limit to 32 bytes or whatever the buffer limit is for given platform
			uint16_t bytesToRead = bytesAvailable; // 16-bit
			if (bytesToRead > i2cTransactionSize)  // Limit for i2cTransactionSize is 8-bit
			bytesToRead = i2cTransactionSize;

			// Here it would be desireable to use a restart where possible / supported, but only if there will be multiple reads.
			// However, if an individual requestFrom fails, we could end up leaving the bus hanging.
			// On balance, it is probably safest to not use restarts here.

			uint8_t bytesReturned = SfeI2C_readBytes(buf, (uint8_t)bytesToRead);
			if ((uint16_t)bytesReturned == bytesToRead)
			{
				for (uint16_t x = 0; x < bytesToRead; x++)
				{
					process(buf[x], incomingUBX, requestedClass, requestedID); // Process this valid character
				}
			}
			else
			{
				// Something has gone very wrong. Sensor did not respond - or a bus error happened...
				if (_resetCurrentSentenceOnBusError)
					currentSentence = SFE_UBLOX_SENTENCE_TYPE_NONE; // Reset the sentence to being looking for a new start char
				
				return (false);
			}

			bytesAvailable -= bytesToRead;
		}
	}

	return (true);

} // end checkUbloxI2C()

// PRIVATE: Called regularly to check for available bytes on the user' specified port
static bool checkUbloxInternal(ubxPacket *incomingUBX, uint8_t requestedClass, uint8_t requestedID)
{
	return checkUbloxI2C(incomingUBX, requestedClass, requestedID);
}

// messages from the class CFG are sent to the receiver, the receiver will send an "acknowledge"(UBX - ACK - ACK) or a
//"not acknowledge"(UBX-ACK-NAK) message back to the sender, depending on whether or not the message was processed correctly.
// Some messages from other classes also use the same acknowledgement mechanism.

// When we poll or get a setting, we will receive _both_ a config packet and an ACK
// If the poll or get request is not valid, we will receive _only_ a NACK

// If we are trying to get or poll a setting, then packetCfg.len will be 0 or 1 when the packetCfg is _sent_.
// If we poll the setting for a particular port using UBX-CFG-PRT then .len will be 1 initially
// For all other gets or polls, .len will be 0 initially
//(It would be possible for .len to be 2 _if_ we were using UBX-CFG-MSG to poll the settings for a particular message - but we don't use that (currently))

// If the get or poll _fails_, i.e. is NACK'd, then packetCfg.len could still be 0 or 1 after the NACK is received
// But if the get or poll is ACK'd, then packetCfg.len will have been updated by the incoming data and will always be at least 2

// If we are going to set the value for a setting, then packetCfg.len will be at least 3 when the packetCfg is _sent_.
//(UBX-CFG-MSG appears to have the shortest set length of 3 bytes)

// We need to think carefully about how interleaved PVT packets affect things.
// It is entirely possible that our packetCfg and packetAck were received successfully
// but while we are still in the "if (checkUblox() == true)" loop a PVT packet is processed
// or _starts_ to arrive (remember that Serial data can arrive very slowly).

// Returns SFE_UBLOX_STATUS_DATA_RECEIVED if we got an ACK and a valid packetCfg (module is responding with register content)
// Returns SFE_UBLOX_STATUS_DATA_SENT if we got an ACK and no packetCfg (no valid packetCfg needed, module absorbs new register data)
// Returns SFE_UBLOX_STATUS_FAIL if something very bad happens (e.g. a double checksum failure)
// Returns SFE_UBLOX_STATUS_COMMAND_NACK if the packet was not-acknowledged (NACK)
// Returns SFE_UBLOX_STATUS_CRC_FAIL if we had a checksum failure
// Returns SFE_UBLOX_STATUS_TIMEOUT if we timed out
// Returns SFE_UBLOX_STATUS_DATA_OVERWRITTEN if we got an ACK and a valid packetCfg but that the packetCfg has been
//  or is currently being overwritten (remember that Serial data can arrive very slowly)
static sfe_ublox_status_e waitForACKResponse(ubxPacket *outgoingUBX, uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime)
{
	outgoingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a response to the packet we sent
	packetAck.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	packetAuto.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	outgoingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a packet that matches the requested class and ID
	packetAck.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	packetBuf.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	packetAuto.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;

	unsigned long startTime = millis();
	while (millis() < (startTime + (unsigned long)maxTime))
	{
		if (checkUbloxInternal(outgoingUBX, requestedClass, requestedID) == true) // See if new data is available. Process bytes as they come in.
		{
			// If both the outgoingUBX->classAndIDmatch and packetAck.classAndIDmatch are VALID
			// and outgoingUBX->valid is _still_ VALID and the class and ID _still_ match
			// then we can be confident that the data in outgoingUBX is valid
			if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
			{
				
				return (SFE_UBLOX_STATUS_DATA_RECEIVED); // We received valid data and a correct ACK!
			}

			// We can be confident that the data packet (if we are going to get one) will always arrive
			// before the matching ACK. So if we sent a config packet which only produces an ACK
			// then outgoingUBX->classAndIDmatch will be NOT_DEFINED and the packetAck.classAndIDmatch will VALID.
			// We should not check outgoingUBX->valid, outgoingUBX->cls or outgoingUBX->id
			// as these may have been changed by an automatic packet.
			else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID))
			{
				
				return (SFE_UBLOX_STATUS_DATA_SENT); // We got an ACK but no data...
			}

			// If both the outgoingUBX->classAndIDmatch and packetAck.classAndIDmatch are VALID
			// but the outgoingUBX->cls or ID no longer match then we can be confident that we had
			// valid data but it has been or is currently being overwritten by an automatic packet (e.g. PVT).
			// If (e.g.) a PVT packet is _being_ received: outgoingUBX->valid will be NOT_DEFINED
			// If (e.g.) a PVT packet _has been_ received: outgoingUBX->valid will be VALID (or just possibly NOT_VALID)
			// So we cannot use outgoingUBX->valid as part of this check.
			// Note: the addition of packetBuf should make this check redundant!
			else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && ((outgoingUBX->cls != requestedClass) || (outgoingUBX->id != requestedID)))
			{
				
				return (SFE_UBLOX_STATUS_DATA_OVERWRITTEN); // Data was valid but has been or is being overwritten
			}

			// If packetAck.classAndIDmatch is VALID but both outgoingUBX->valid and outgoingUBX->classAndIDmatch
			// are NOT_VALID then we can be confident we have had a checksum failure on the data packet
			else if ((packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID))
			{
				
				return (SFE_UBLOX_STATUS_CRC_FAIL); // Checksum fail
			}

			// If our packet was not-acknowledged (NACK) we do not receive a data packet - we only get the NACK.
			// So you would expect outgoingUBX->valid and outgoingUBX->classAndIDmatch to still be NOT_DEFINED
			// But if a full PVT packet arrives afterwards outgoingUBX->valid could be VALID (or just possibly NOT_VALID)
			// but outgoingUBX->cls and outgoingUBX->id would not match...
			// So I think this is telling us we need a special state for packetAck.classAndIDmatch to tell us
			// the packet was definitely NACK'd otherwise we are possibly just guessing...
			// Note: the addition of packetBuf changes the logic of this, but we'll leave the code as is for now.
			else if (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_NOTACKNOWLEDGED)
			{
				
				return (SFE_UBLOX_STATUS_COMMAND_NACK); // We received a NACK!
			}

			// If the outgoingUBX->classAndIDmatch is VALID but the packetAck.classAndIDmatch is NOT_VALID
			// then the ack probably had a checksum error. We will take a gamble and return DATA_RECEIVED.
			// If we were playing safe, we should return FAIL instead
			else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
			{
				
				return (SFE_UBLOX_STATUS_DATA_RECEIVED); // We received valid data and an invalid ACK!
			}

			// If the outgoingUBX->classAndIDmatch is NOT_VALID and the packetAck.classAndIDmatch is NOT_VALID
			// then we return a FAIL. This must be a double checksum failure?
			else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID))
			{
				
				return (SFE_UBLOX_STATUS_FAIL); // We received invalid data and an invalid ACK!
			}

			// If the outgoingUBX->classAndIDmatch is VALID and the packetAck.classAndIDmatch is NOT_DEFINED
			// then the ACK has not yet been received and we should keep waiting for it
			else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED))
			{
				
			}

		} // checkUbloxInternal == true

		delay(1); // Allow an RTOS to get an elbow in (#11)
	}           // while (millis() < (startTime + (unsigned long)maxTime))

	// We have timed out...
	// If the outgoingUBX->classAndIDmatch is VALID then we can take a gamble and return DATA_RECEIVED
	// even though we did not get an ACK
	if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (packetAck.classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
	{
		
		return (SFE_UBLOX_STATUS_DATA_RECEIVED); // We received valid data... But no ACK!
	}

	return (SFE_UBLOX_STATUS_TIMEOUT);
}

// For non-CFG queries no ACK is sent so we use this function
// Returns SFE_UBLOX_STATUS_DATA_RECEIVED if we got a config packet full of response data that has CLS/ID match to our query packet
// Returns SFE_UBLOX_STATUS_CRC_FAIL if we got a corrupt config packet that has CLS/ID match to our query packet
// Returns SFE_UBLOX_STATUS_TIMEOUT if we timed out
// Returns SFE_UBLOX_STATUS_DATA_OVERWRITTEN if we got an a valid packetCfg but that the packetCfg has been
//  or is currently being overwritten (remember that Serial data can arrive very slowly)
static sfe_ublox_status_e waitForNoACKResponse(ubxPacket *outgoingUBX, uint8_t requestedClass, uint8_t requestedID, uint16_t maxTime)
{
	outgoingUBX->valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a response to the packet we sent
	packetAck.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	packetBuf.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	packetAuto.valid = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	outgoingUBX->classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED; // This will go VALID (or NOT_VALID) when we receive a packet that matches the requested class and ID
	packetAck.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	packetBuf.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;
	packetAuto.classAndIDmatch = SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED;

	unsigned long startTime = millis();
	while (millis() - startTime < maxTime)
	{
		if (checkUbloxInternal(outgoingUBX, requestedClass, requestedID) == true) // See if new data is available. Process bytes as they come in.
		{

			// If outgoingUBX->classAndIDmatch is VALID
			// and outgoingUBX->valid is _still_ VALID and the class and ID _still_ match
			// then we can be confident that the data in outgoingUBX is valid
			if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID) && (outgoingUBX->cls == requestedClass) && (outgoingUBX->id == requestedID))
			{
				
				return (SFE_UBLOX_STATUS_DATA_RECEIVED); // We received valid data!
			}

			// If the outgoingUBX->classAndIDmatch is VALID
			// but the outgoingUBX->cls or ID no longer match then we can be confident that we had
			// valid data but it has been or is currently being overwritten by another packet (e.g. PVT).
			// If (e.g.) a PVT packet is _being_ received: outgoingUBX->valid will be NOT_DEFINED
			// If (e.g.) a PVT packet _has been_ received: outgoingUBX->valid will be VALID (or just possibly NOT_VALID)
			// So we cannot use outgoingUBX->valid as part of this check.
			// Note: the addition of packetBuf should make this check redundant!
			else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_VALID) && ((outgoingUBX->cls != requestedClass) || (outgoingUBX->id != requestedID)))
			{
				
				return (SFE_UBLOX_STATUS_DATA_OVERWRITTEN); // Data was valid but has been or is being overwritten
			}

			// If outgoingUBX->classAndIDmatch is NOT_DEFINED
			// and outgoingUBX->valid is VALID then this must be (e.g.) a PVT packet
			else if ((outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_DEFINED) && (outgoingUBX->valid == SFE_UBLOX_PACKET_VALIDITY_VALID))
			{
				
			}

			// If the outgoingUBX->classAndIDmatch is NOT_VALID then we return CRC failure
			else if (outgoingUBX->classAndIDmatch == SFE_UBLOX_PACKET_VALIDITY_NOT_VALID)
			{
				
				return (SFE_UBLOX_STATUS_CRC_FAIL); // We received invalid data
			}
		}

		delay(1); // Allow an RTOS to get an elbow in (#11)
	}

	return (SFE_UBLOX_STATUS_TIMEOUT);
}

// Given a packet and payload, send everything including CRC bytes via I2C port
static sfe_ublox_status_e sendCommand(ubxPacket *outgoingUBX, uint16_t maxWait, bool expectACKonly)
{
	sfe_ublox_status_e retVal = SFE_UBLOX_STATUS_SUCCESS;

	calcChecksum(outgoingUBX); // Sets checksum A and B bytes of the packet

	retVal = sendI2cCommand(outgoingUBX);
	if (retVal != SFE_UBLOX_STATUS_SUCCESS)
	{
		return retVal;
	}
	
	if (maxWait > 0)
	{
		// Depending on what we just sent, either we need to look for an ACK or not
		if ((outgoingUBX->cls == UBX_CLASS_CFG) || (expectACKonly == true))
		{
			retVal = waitForACKResponse(outgoingUBX, outgoingUBX->cls, outgoingUBX->id, maxWait); // Wait for Ack response
		}
		else
		{
			retVal = waitForNoACKResponse(outgoingUBX, outgoingUBX->cls, outgoingUBX->id, maxWait); // Wait for Ack response
		}
	}
	
	return retVal;
}

// CONFIGURATION INTERFACE (protocol v27 and above)

// Given a key, load the payload with data that can then be extracted to 8, 16, or 32 bits
// This function takes a full 32-bit key
// Default layer is RAM
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
static sfe_ublox_status_e getVal(uint32_t key, uint8_t layer, uint16_t maxWait)
{
	packetCfg.cls = UBX_CLASS_CFG;
	packetCfg.id = UBX_CFG_VALGET;
	packetCfg.len = 4 + 4 * 1; // While multiple keys are allowed, we will send only one key at a time
	packetCfg.startingSpot = 0;

	// Clear packet payload
	memset(payloadCfg, 0, packetCfg.len);

	// VALGET uses different memory layer definitions to VALSET
	// because it can only return the value for one layer.
	// So we need to fiddle the layer here.
	// And just to complicate things further, the ZED-F9P only responds
	// correctly to layer 0 (RAM) and layer 7 (Default)!
	uint8_t getLayer = VAL_LAYER_DEFAULT; // 7 is the "Default Layer"
	if (layer == VAL_LAYER_RAM)           // Did the user request the RAM layer?
	{
		getLayer = 0; // Layer 0 is RAM
	}
	else if (layer == VAL_LAYER_BBR) // Did the user request the BBR layer?
	{
		getLayer = 1; // Layer 1 is BBR
	}
	else if (layer == VAL_LAYER_FLASH) // Did the user request the Flash layer?
	{
		getLayer = 2; // Layer 2 is Flash
	}

	payloadCfg[0] = 0;        // Message Version - set to 0
	payloadCfg[1] = getLayer; // Layer

	// Load key into outgoing payload
	key &= ~UBX_CFG_SIZE_MASK;    // Mask off the size identifer bits
	payloadCfg[4] = key >> (8 * 0); // Key LSB
	payloadCfg[5] = key >> (8 * 1);
	payloadCfg[6] = key >> (8 * 2);
	payloadCfg[7] = key >> (8 * 3);

	// Send VALGET command with this key
	sfe_ublox_status_e retVal = sendCommand(&packetCfg, maxWait, false);
	
	// Verify the response is the correct length as compared to what the user called (did the module respond with 8-bits but the user called getVal32?)
	// Response is 8 bytes plus cfg data
	// if(packet->len > 8+1)

	// The response is now sitting in payload, ready for extraction
	return (retVal);
}


// Given a key, return its value
// This function takes a full 32-bit key
// Default layer is RAM
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
static bool getVal8(uint32_t key, uint8_t *val, uint8_t layer, uint16_t maxWait)
{
	bool result = getVal(key, layer, maxWait) == SFE_UBLOX_STATUS_DATA_RECEIVED;
	if (result)
	*val = extractByte(&packetCfg, 8);
	return result;
}
	
static bool isConnected(uint16_t maxWait)
{
	uint8_t en;
	
	if(!SfeI2C_ping())
		return false;
		
	return getVal8(UBLOX_CFG_I2CINPROT_UBX, &en, VAL_LAYER_RAM, maxWait);
}

uint8_t _numCfgKeys;
uint16_t _autoSendAtSpaceRemaining;

// Start defining a new (empty) UBX-CFG-VALSET ubxPacket
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
static bool newCfgValset(uint8_t layer)
{
	packetCfg.cls = UBX_CLASS_CFG;
	packetCfg.id = UBX_CFG_VALSET;
	packetCfg.len = 4; // 4 byte header
	packetCfg.startingSpot = 0;

	_numCfgKeys = 0;

	// Clear all of packet payload
	memset(payloadCfg, 0, UBX_NAV_SAT_MAX_LEN);

	payloadCfg[0] = 0;     // Message Version - set to 0
	payloadCfg[1] = layer; // By default we ask for the BBR layer

	// All done
	return (true);
}


// Add another key and value to an existing UBX-CFG-VALSET ubxPacket
// This function takes a full 32-bit key and N-byte value
static bool addCfgValsetN(uint32_t key, uint8_t *value, uint8_t N)
{
	if ((_autoSendAtSpaceRemaining > 0) && (packetCfg.len >= (UBX_NAV_SAT_MAX_LEN - _autoSendAtSpaceRemaining)))
	{
		if (sendCommand(&packetCfg,MAXWAIT,true) != SFE_UBLOX_STATUS_DATA_SENT) // We are only expecting an ACK
		return false;
		packetCfg.len = 4; // 4 byte header
		packetCfg.startingSpot = 0;
		_numCfgKeys = 0;
		memset(&payloadCfg[4], 0, UBX_NAV_SAT_MAX_LEN - 4);
	}

	if (packetCfg.len >= (UBX_NAV_SAT_MAX_LEN - (4 + N)))
	{
		return false;
	}

	if (_numCfgKeys == CFG_VALSET_MAX_KEYS)
	{
		return false;
	}

	// Load key into outgoing payload
	key &= ~UBX_CFG_SIZE_MASK; // Mask off the size identifer bits
	for (uint8_t i = 0; i < 4; i++)
	payloadCfg[packetCfg.len + i] = key >> (8 * i); // Key

	// Load user's value
	for (uint8_t i = 0; i < N; i++)
	payloadCfg[packetCfg.len + i + 4] = *value++; // Value

	// Update packet length: 4 byte key ID, 8 bytes of value
	packetCfg.len = packetCfg.len + 4 + N;

	_numCfgKeys++;

	// All done
	return (true);
}

// Add another key and value to an existing UBX-CFG-VALSET ubxPacket
// This function takes a full 32-bit key and 8-bit value
static bool addCfgValset8(uint32_t key, uint8_t value)
{
	uint8_t val[1] = {value};
	return (addCfgValsetN(key, val, 1));
}

// Send the UBX-CFG-VALSET ubxPacket
static bool sendCfgValset(uint16_t maxWait)
{
	if (_numCfgKeys == 0)
	return true; // Nothing to send...

	// Send VALSET command with this key and value
	bool success = sendCommand(&packetCfg, maxWait, true) == SFE_UBLOX_STATUS_DATA_SENT; // We are only expecting an ACK

	if (success)
	_numCfgKeys = 0;

	return success;
}

// Given a key, set a N-byte value
// This function takes a full 32-bit key
// Default layer is RAM+BBR
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
static bool setValN(uint32_t key, uint8_t *value, uint8_t N, uint8_t layer, uint16_t maxWait)
{
	packetCfg.cls = UBX_CLASS_CFG;
	packetCfg.id = UBX_CFG_VALSET;
	packetCfg.len = 4 + 4 + N; // 4 byte header, 4 byte key ID, N bytes of value
	packetCfg.startingSpot = 0;

	// Clear packet payload
	memset(payloadCfg, 0, packetCfg.len);

	payloadCfg[0] = 0;     // Message Version - set to 0
	payloadCfg[1] = layer; // By default we ask for the BBR layer

	// Load key into outgoing payload
	key &= ~UBX_CFG_SIZE_MASK; // Mask off the size identifer bits
	for (uint8_t i = 0; i < 4; i++)
	payloadCfg[i + 4] = key >> (8 * i); // Key

	// Load user's value
	for (uint8_t i = 0; i < N; i++)
	payloadCfg[i + 8] = *value++;

	// Send VALSET command with this key and value
	return (sendCommand(&packetCfg, maxWait, true) == SFE_UBLOX_STATUS_DATA_SENT); // We are only expecting an ACK
}

// Given a key, set an 8-bit value
// This function takes a full 32-bit key
// Default layer is RAM+BBR
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
static bool setVal8(uint32_t key, uint8_t value, uint8_t layer, uint16_t maxWait)
{
	uint8_t val[1] = {value};
	return (setValN(key, val, 1, layer, maxWait));
}

// Given a key, set a 16-bit value
// This function takes a full 32-bit key
// Default layer is RAM+BBR
// Configuration of modern u-blox modules is now done via getVal/setVal/delVal, ie protocol v27 and above found on ZED-F9P
static bool setVal16(uint32_t key, uint16_t value, uint8_t layer, uint16_t maxWait)
{
	uint8_t val[2] = {(uint8_t)(value >> 0), (uint8_t)(value >> 8)};
	return (setValN(key, val, 2, layer, maxWait));
}

// Configure a port to output UBX, NMEA, RTCM3 or a combination thereof
static bool setI2COutput(uint8_t comSettings, uint8_t layer, uint16_t maxWait)
{
	bool result = newCfgValset(layer);
	result &= addCfgValset8(UBLOX_CFG_I2COUTPROT_UBX, (comSettings & COM_TYPE_UBX) == 0 ? 0 : 1);
	result &= addCfgValset8(UBLOX_CFG_I2COUTPROT_NMEA, (comSettings & COM_TYPE_NMEA) == 0 ? 0 : 1);
	result &= sendCfgValset(maxWait);
	result |= setVal8(UBLOX_CFG_I2COUTPROT_RTCM3X, (comSettings & COM_TYPE_RTCM3) == 0 ? 0 : 1, layer, maxWait); // This will be NACK'd if the module does not support RTCM3
	return result;
}

// Set the rate at which the module will give us an updated navigation solution
// Expects a number that is the updates per second. For example 1 = 1Hz, 2 = 2Hz, etc.
// Max is 40Hz(?!)
static bool setNavigationFrequency(uint8_t navFreq, uint8_t layer, uint16_t maxWait)
{
	if (navFreq == 0) // Return now if navFreq is zero
	return (false);

	if (navFreq > 40)
	navFreq = 40; // Limit navFreq to 40Hz so i2cPollingWait is set correctly

	uint16_t measurementRate = 1000 / navFreq;

	return setVal16(UBLOX_CFG_RATE_MEAS, measurementRate, layer, maxWait);
}

// Enable or disable automatic navigation message generation by the GNSS. This changes the way getPVT
// works.
static bool setAutoPVTrate(uint8_t rate, bool implicitUpdate, uint8_t layer, uint16_t maxWait)
{
	
	if (rate > 127)
	rate = 127;

	uint32_t key = UBLOX_CFG_MSGOUT_UBX_NAV_PVT_I2C;
	
	bool ok = setVal8(key, rate, layer, maxWait);
	if (ok)
	{
		packetUBXNAVPVT.automaticFlags.flags.bits.automatic = (rate > 0);
		packetUBXNAVPVT.automaticFlags.flags.bits.implicitUpdate = implicitUpdate;
	}
	packetUBXNAVPVT.moduleQueried.moduleQueried1.bits.all = false;
	return ok;
}

// Enable or disable automatic navigation message generation by the GNSS. This changes the way getPVT
// works.
static bool setAutoPVT(bool enable, uint8_t layer, uint16_t maxWait)
{
	return setAutoPVTrate(enable ? 1 : 0, true, layer, maxWait);
}

void GPS_poll(void)
{
	switch(gps_state)
	{
		
		default:
		case 0:
		
			if(SfeI2C_init(GPS_ADDR))
				gps_state=1;
		
			break;
			
		case 1:
			if(isConnected(MAXWAIT))
				gps_state=2;
			
			break;
	
		case 2:
		
			setI2COutput(COM_TYPE_UBX,0,MAXWAIT);	// only UBX packets
			setNavigationFrequency(2,0,MAXWAIT);	// Produce two solutions per second
			setAutoPVT(true,0,MAXWAIT);				// Tell the GNSS to output each solution periodically
			
			gps_state=3;
			break;	
			
		case 3:
			if (getPVT(MAXWAIT) == true)
			{
				gps_lon = getLongitude(MAXWAIT);
				gps_lat = getLatitude(MAXWAIT);
				
			}	
			break;
	}
	
}