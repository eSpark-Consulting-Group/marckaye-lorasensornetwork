/*
  An Arduino Library which allows you to communicate seamlessly with u-blox GNSS modules using the Configuration Interface

  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/15136
  https://www.sparkfun.com/products/16481
  https://www.sparkfun.com/products/16344
  https://www.sparkfun.com/products/18037
  https://www.sparkfun.com/products/18719
  https://www.sparkfun.com/products/18774
  https://www.sparkfun.com/products/19663
  https://www.sparkfun.com/products/17722

  Original version by Nathan Seidle @ SparkFun Electronics, September 6th, 2018
  v2.0 rework by Paul Clark @ SparkFun Electronics, December 31st, 2020
  v3.0 rework by Paul Clark @ SparkFun Electronics, December 8th, 2022

  https://github.com/sparkfun/SparkFun_u-blox_GNSS_v3

  This library is an updated version of the popular SparkFun u-blox GNSS Arduino Library.
  v3 uses the u-blox Configuration Interface (VALSET and VALGET) to:
  detect the module (during begin); configure message intervals; configure the base location; etc..

  This version of the library will not work with older GNSS modules.
  It is specifically written for newer modules like the ZED-F9P, ZED-F9R and MAX-M10S.
  For older modules, please use v2 of the library: https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.19

  SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
  The MIT License (MIT)
  Copyright (c) 2018 SparkFun Electronics
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
  do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial
  portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// u-blox Class and ID definitions

#define UBX_SYNCH_1  0xB5
#define UBX_SYNCH_2  0x62

// The following are UBX Class IDs. Descriptions taken from ZED-F9P Interface Description Document page 32, NEO-M8P Interface Description page 145
#define UBX_CLASS_NAV  0x01  // Navigation Results Messages: Position, Speed, Time, Acceleration, Heading, DOP, SVs used
#define UBX_CLASS_RXM  0x02  // Receiver Manager Messages: Satellite Status, RTC Status
#define UBX_CLASS_INF  0x04  // Information Messages: Printf-Style Messages, with IDs such as Error, Warning, Notice
#define UBX_CLASS_ACK  0x05  // Ack/Nak Messages: Acknowledge or Reject messages to UBX-CFG input messages
#define UBX_CLASS_CFG  0x06  // Configuration Input Messages: Configure the receiver.
#define UBX_CLASS_UPD  0x09  // Firmware Update Messages: Memory/Flash erase/write, Reboot, Flash identification, etc.
#define UBX_CLASS_MON  0x0A  // Monitoring Messages: Communication Status, CPU Load, Stack Usage, Task Status
#define UBX_CLASS_AID  0x0B  //(NEO-M8P ONLY!!!) AssistNow Aiding Messages: Ephemeris, Almanac, other A-GPS data input
#define UBX_CLASS_TIM  0x0D  // Timing Messages: Time Pulse Output, Time Mark Results
#define UBX_CLASS_ESF  0x10  //(NEO-M8P ONLY!!!) External Sensor Fusion Messages: External Sensor Measurements and Status Information
#define UBX_CLASS_MGA  0x13  // Multiple GNSS Assistance Messages: Assistance data for various GNSS
#define UBX_CLASS_LOG  0x21  // Logging Messages: Log creation, deletion, info and retrieval
#define UBX_CLASS_SEC  0x27  // Security Feature Messages
#define UBX_CLASS_HNR  0x28  //(NEO-M8P ONLY!!!) High Rate Navigation Results Messages: High rate time, position speed, heading
#define UBX_CLASS_NMEA  0xF0 // NMEA Strings: standard NMEA strings
#define UBX_CLASS_PUBX  0xF1 // Proprietary NMEA-format messages defined by u-blox

// Class: CFG
// The following are used for configuration. Descriptions are from the ZED-F9P Interface Description pg 33-34 and NEO-M9N Interface Description pg 47-48
#define UBX_CFG_ANT  0x13       // Antenna Control Settings. Used to configure the antenna control settings
#define UBX_CFG_BATCH  0x93     // Get/set data batching configuration.
#define UBX_CFG_CFG  0x09       // Clear, Save, and Load Configurations. Used to save current configuration
#define UBX_CFG_DAT  0x06       // Set User-defined Datum or The currently defined Datum
#define UBX_CFG_DGNSS  0x70     // DGNSS configuration
#define UBX_CFG_ESFALG  0x56    // ESF alignment
#define UBX_CFG_ESFA  0x4C      // ESF accelerometer
#define UBX_CFG_ESFG  0x4D      // ESF gyro
#define UBX_CFG_GEOFENCE  0x69  // Geofencing configuration. Used to configure a geofence
#define UBX_CFG_GNSS  0x3E      // GNSS system configuration
#define UBX_CFG_HNR  0x5C       // High Navigation Rate
#define UBX_CFG_INF  0x02       // Depending on packet length, either: poll configuration for one protocol, or information message configuration
#define UBX_CFG_ITFM  0x39      // Jamming/Interference Monitor configuration
#define UBX_CFG_LOGFILTER  0x47 // Data Logger Configuration
#define UBX_CFG_MSG 0x01       // Poll a message configuration, or Set Message Rate(s), or Set Message Rate
#define UBX_CFG_NAV5 0x24      // Navigation Engine Settings. Used to configure the navigation engine including the dynamic model.
#define UBX_CFG_NAVX5 0x23     // Navigation Engine Expert Settings
#define UBX_CFG_NMEA 0x17      // Extended NMEA protocol configuration V1
#define UBX_CFG_ODO 0x1E       // Odometer, Low-speed COG Engine Settings
#define UBX_CFG_PM2 0x3B       // Extended power management configuration
#define UBX_CFG_PMS 0x86       // Power mode setup
#define UBX_CFG_PRT 0x00       // Used to configure port specifics. Polls the configuration for one I/O Port, or Port configuration for UART ports, or Port configuration for USB port, or Port configuration for SPI port, or Port configuration for DDC port
#define UBX_CFG_PWR 0x57       // Put receiver in a defined power state
#define UBX_CFG_RATE 0x08      // Navigation/Measurement Rate Settings. Used to set port baud rates.
#define UBX_CFG_RINV 0x34      // Contents of Remote Inventory
#define UBX_CFG_RST 0x04       // Reset Receiver / Clear Backup Data Structures. Used to reset device.
#define UBX_CFG_RXM 0x11       // RXM configuration
#define UBX_CFG_SBAS 0x16      // SBAS configuration
#define UBX_CFG_TMODE3 0x71    // Time Mode Settings 3. Used to enable Survey In Mode
#define UBX_CFG_TP5 0x31       // Time Pulse Parameters
#define UBX_CFG_USB 0x1B       // USB Configuration
#define UBX_CFG_VALDEL 0x8C    // Used for config of higher version u-blox modules (ie protocol v27 and above). Deletes values corresponding to provided keys/ provided keys with a transaction
#define UBX_CFG_VALGET 0x8B    // Used for config of higher version u-blox modules (ie protocol v27 and above). Configuration Items
#define UBX_CFG_VALSET 0x8A    // Used for config of higher version u-blox modules (ie protocol v27 and above). Sets values corresponding to provided key-value pairs/ provided key-value pairs within a transaction.

// Class: NMEA
// The following are used to enable NMEA messages. Descriptions come from the NMEA messages overview in the ZED-F9P Interface Description
#define UBX_NMEA_MSB_ 0xF0 
// All NMEA enable commands have 0xF0 as MSB. Equal to UBX_CLASS_NMEA
#define UBX_NMEA_DTM_ 0x0A 
// GxDTM (datum reference)
#define UBX_NMEA_GAQ_ 0x45 
// GxGAQ (poll a standard message (if the current talker ID is GA))
#define UBX_NMEA_GBQ_ 0x44 // GxGBQ (poll a standard message (if the current Talker ID is GB))
#define UBX_NMEA_GBS_ 0x09 // GxGBS (GNSS satellite fault detection)
#define UBX_NMEA_GGA_ 0x00 // GxGGA (Global positioning system fix data)
#define UBX_NMEA_GLL_ 0x01 // GxGLL (latitude and long, whith time of position fix and status)
#define UBX_NMEA_GLQ_ 0x43 // GxGLQ (poll a standard message (if the current Talker ID is GL))
#define UBX_NMEA_GNQ_ 0x42 // GxGNQ (poll a standard message (if the current Talker ID is GN))
#define UBX_NMEA_GNS_ 0x0D // GxGNS (GNSS fix data)
#define UBX_NMEA_GPQ_ 0x40 // GxGPQ (poll a standard message (if the current Talker ID is GP))
#define UBX_NMEA_GQQ_ 0x47 // GxGQQ (poll a standard message (if the current Talker ID is GQ))
#define UBX_NMEA_GRS_ 0x06 // GxGRS (GNSS range residuals)
#define UBX_NMEA_GSA_ 0x02 // GxGSA (GNSS DOP and Active satellites)
#define UBX_NMEA_GST_ 0x07 // GxGST (GNSS Pseudo Range Error Statistics)
#define UBX_NMEA_GSV_ 0x03 // GxGSV (GNSS satellites in view)
#define UBX_NMEA_RLM_ 0x0B // GxRMC (Return link message (RLM))
#define UBX_NMEA_RMC_ 0x04 // GxRMC (Recommended minimum data)
#define UBX_NMEA_THS_ 0x0E // GxTHS (True heading and status)
#define UBX_NMEA_TXT_ 0x41 // GxTXT (text transmission)
#define UBX_NMEA_VLW_ 0x0F // GxVLW (dual ground/water distance)
#define UBX_NMEA_VTG_ 0x05 // GxVTG (course over ground and Ground speed)
#define UBX_NMEA_ZDA_ 0x08 // GxZDA (Time and Date)

// The following are used to configure the NMEA protocol main talker ID and GSV talker ID
#define UBX_NMEA_MAINTALKERID_NOTOVERRIDDEN  0x00 // main talker ID is system dependent
#define UBX_NMEA_MAINTALKERID_GP  0x01            // main talker ID is GPS
#define UBX_NMEA_MAINTALKERID_GL  0x02            // main talker ID is GLONASS
#define UBX_NMEA_MAINTALKERID_GN  0x03            // main talker ID is combined receiver
#define UBX_NMEA_MAINTALKERID_GA  0x04            // main talker ID is Galileo
#define UBX_NMEA_MAINTALKERID_GB  0x05            // main talker ID is BeiDou
#define UBX_NMEA_MAINTALKERID_GQ  0x07            // main talker ID is QZSS
#define UBX_NMEA_GSVTALKERID_GNSS  0x00           // GNSS specific Talker ID (as defined by NMEA)
#define UBX_NMEA_GSVTALKERID_MAIN  0x01           // use the main Talker ID

// Class: PUBX
// The following are used to enable PUBX messages with configureMessage
// See the M8 receiver description & protocol specification for more details
#define UBX_PUBX_CONFIG  0x41   // Set protocols and baud rate
#define UBX_PUBX_POSITION  0x00 // Lat/Long position data
#define UBX_PUBX_RATE  0x40     // Set/get NMEA message output rate
#define UBX_PUBX_SVSTATUS  0x03 // Satellite status
#define UBX_PUBX_TIME  0x04     // Time of day and clock information

// Class: HNR
// The following are used to configure the HNR message rates
#define UBX_HNR_ATT  0x01 // HNR Attitude
#define UBX_HNR_INS  0x02 // HNR Vehicle Dynamics
#define UBX_HNR_PVT  0x00 // HNR PVT

// Class: INF
// The following are used to configure INF UBX messages (information messages).  Descriptions from UBX messages overview (ZED_F9P Interface Description Document page 34)
#define UBX_INF_CLASS  0x04   // All INF messages have 0x04 as the class
#define UBX_INF_DEBUG  0x04   // ASCII output with debug contents
#define UBX_INF_ERROR  0x00   // ASCII output with error contents
#define UBX_INF_NOTICE  0x02  // ASCII output with informational contents
#define UBX_INF_TEST  0x03    // ASCII output with test contents
#define UBX_INF_WARNING  0x01 // ASCII output with warning contents

// Class: LOG
// The following are used to configure LOG UBX messages (loggings messages).  Descriptions from UBX messages overview (ZED_F9P Interface Description Document page 34)
#define UBX_LOG_CREATE  0x07           // Create Log File
#define UBX_LOG_ERASE  0x03            // Erase Logged Data
#define UBX_LOG_FINDTIME  0x0E         // Find index of a log entry based on a given time, or response to FINDTIME requested
#define UBX_LOG_INFO  0x08             // Poll for log information, or Log information
#define UBX_LOG_RETRIEVEPOSEXTRA  0x0F // Odometer log entry
#define UBX_LOG_RETRIEVEPOS  0x0B      // Position fix log entry
#define UBX_LOG_RETRIEVESTRING  0x0D   // Byte string log entry
#define UBX_LOG_RETRIEVE  0x09         // Request log data
#define UBX_LOG_STRING  0x04           // Store arbitrary string on on-board flash

// Class: MGA
// The following are used to configure MGA UBX messages (Multiple GNSS Assistance Messages).  Descriptions from UBX messages overview (ZED_F9P Interface Description Document page 34)
#define UBX_MGA_ACK_DATA0  0x60      // Multiple GNSS Acknowledge message
#define UBX_MGA_ANO  0x20            // Multiple GNSS AssistNow Offline assistance - NOT SUPPORTED BY THE ZED-F9P! "The ZED-F9P supports AssistNow Online only."
#define UBX_MGA_BDS_EPH  0x03        // BDS Ephemeris Assistance
#define UBX_MGA_BDS_ALM  0x03        // BDS Almanac Assistance
#define UBX_MGA_BDS_HEALTH  0x03     // BDS Health Assistance
#define UBX_MGA_BDS_UTC  0x03        // BDS UTC Assistance
#define UBX_MGA_BDS_IONO  0x03       // BDS Ionospheric Assistance
#define UBX_MGA_DBD  0x80            // Either: Poll the Navigation Database, or Navigation Database Dump Entry
#define UBX_MGA_GAL_EPH  0x02        // Galileo Ephemeris Assistance
#define UBX_MGA_GAL_ALM  0x02        // Galileo Almanac Assitance
#define UBX_MGA_GAL_TIMOFFSET  0x02  // Galileo GPS time offset assistance
#define UBX_MGA_GAL_UTC  0x02        // Galileo UTC Assistance
#define UBX_MGA_GLO_EPH  0x06        // GLONASS Ephemeris Assistance
#define UBX_MGA_GLO_ALM  0x06        // GLONASS Almanac Assistance
#define UBX_MGA_GLO_TIMEOFFSET  0x06 // GLONASS Auxiliary Time Offset Assistance
#define UBX_MGA_GPS_EPH  0x00        // GPS Ephemeris Assistance
#define UBX_MGA_GPS_ALM  0x00        // GPS Almanac Assistance
#define UBX_MGA_GPS_HEALTH  0x00     // GPS Health Assistance
#define UBX_MGA_GPS_UTC  0x00        // GPS UTC Assistance
#define UBX_MGA_GPS_IONO  0x00       // GPS Ionosphere Assistance
#define UBX_MGA_INI_POS_XYZ  0x40    // Initial Position Assistance
#define UBX_MGA_INI_POS_LLH  0x40    // Initial Position Assitance
#define UBX_MGA_INI_TIME_UTC  0x40   // Initial Time Assistance
#define UBX_MGA_INI_TIME_GNSS  0x40  // Initial Time Assistance
#define UBX_MGA_INI_CLKD  0x40       // Initial Clock Drift Assitance
#define UBX_MGA_INI_FREQ  0x40       // Initial Frequency Assistance
#define UBX_MGA_INI_EOP  0x40        // Earth Orientation Parameters Assistance
#define UBX_MGA_QZSS_EPH  0x05       // QZSS Ephemeris Assistance
#define UBX_MGA_QZSS_ALM  0x05       // QZSS Almanac Assistance
#define UBX_MGA_QZAA_HEALTH  0x05    // QZSS Health Assistance

// Class: MON
// The following are used to configure the MON UBX messages (monitoring messages). Descriptions from UBX messages overview (ZED_F9P Interface Description Document page 35)
#define UBX_MON_COMMS  0x36 // Comm port information
#define UBX_MON_GNSS  0x28  // Information message major GNSS selection
#define UBX_MON_HW2  0x0B   // Extended Hardware Status
#define UBX_MON_HW3  0x37   // HW I/O pin information
#define UBX_MON_HW  0x09    // Hardware Status
#define UBX_MON_IO  0x02    // I/O Subsystem Status
#define UBX_MON_MSGPP  0x06 // Message Parse and Process Status
#define UBX_MON_PATCH  0x27 // Output information about installed patches
#define UBX_MON_RF  0x38    // RF information
#define UBX_MON_RXBUF  0x07 // Receiver Buffer Status
#define UBX_MON_RXR  0x21   // Receiver Status Information
#define UBX_MON_SPAN  0x31  // Signal characteristics
#define UBX_MON_SYS  0x39   // Current system performance information
#define UBX_MON_TXBUF  0x08 // Transmitter Buffer Status. Used for query tx buffer size/state.
#define UBX_MON_VER  0x04   // Receiver/Software Version. Used for obtaining Protocol Version.

// Class: NAV
// The following are used to configure the NAV UBX messages (navigation results messages). Descriptions from UBX messages overview (ZED_F9P Interface Description Document page 35-36)
#define UBX_NAV_ATT  0x05       // Vehicle "Attitude" Solution
#define UBX_NAV_CLOCK  0x22     // Clock Solution
#define UBX_NAV_COV  0x36       // Covariance matrices
#define UBX_NAV_DOP  0x04       // Dilution of precision
#define UBX_NAV_EELL  0x3D      // Position error ellipse parameters
#define UBX_NAV_EOE  0x61       // End of Epoch
#define UBX_NAV_GEOFENCE  0x39  // Geofencing status. Used to poll the geofence status
#define UBX_NAV_HPPOSECEF  0x13 // High Precision Position Solution in ECEF. Used to find our positional accuracy (high precision).
#define UBX_NAV_HPPOSLLH  0x14  // High Precision Geodetic Position Solution. Used for obtaining lat/long/alt in high precision
#define UBX_NAV_ODO  0x09       // Odometer Solution
#define UBX_NAV_ORB  0x34       // GNSS Orbit Database Info
#define UBX_NAV_PL  0x62        // Protection Level Information
#define UBX_NAV_POSECEF  0x01   // Position Solution in ECEF
#define UBX_NAV_POSLLH  0x02    // Geodetic Position Solution
#define UBX_NAV_PVT  0x07       // All the things! Position, velocity, time, PDOP, height, h/v accuracies, number of satellites. Navigation Position Velocity Time Solution.
#define UBX_NAV_PVAT  0x17      // Navigation position velocity attitude time solution (ZED-F9R only)
#define UBX_NAV_RELPOSNED  0x3C // Relative Positioning Information in NED frame
#define UBX_NAV_RESETODO  0x10  // Reset odometer
#define UBX_NAV_SAT  0x35       // Satellite Information
#define UBX_NAV_SBAS  0x32      // SBAS subsystem
#define UBX_NAV_SLAS  0x42      // QZSS L1S SLAS status data
#define UBX_NAV_SIG  0x43       // Signal Information
#define UBX_NAV_STATUS  0x03    // Receiver Navigation Status
#define UBX_NAV_SVIN  0x3B      // Survey-in data. Used for checking Survey In status
#define UBX_NAV_TIMEBDS  0x24   // BDS Time Solution
#define UBX_NAV_TIMEGAL  0x25   // Galileo Time Solution
#define UBX_NAV_TIMEGLO  0x23   // GLO Time Solution
#define UBX_NAV_TIMEGPS  0x20   // GPS Time Solution
#define UBX_NAV_TIMELS  0x26    // Leap second event information
#define UBX_NAV_TIMEUTC  0x21   // UTC Time Solution
#define UBX_NAV_VELECEF  0x11   // Velocity Solution in ECEF
#define UBX_NAV_VELNED  0x12    // Velocity Solution in NED
#define UBX_NAV_AOPSTATUS  0x60 // AssistNow Autonomous status

// Class: RXM
// The following are used to configure the RXM UBX messages (receiver manager messages). Descriptions from UBX messages overview (ZED_F9P Interface Description Document page 36)
#define UBX_RXM_COR  0x34       // Differential correction input status
#define UBX_RXM_MEASX  0x14     // Satellite Measurements for RRLP
#define UBX_RXM_PMP  0x72       // PMP raw data (NEO-D9S) (two different versions) (packet size for version 0x01 is variable)
#define UBX_RXM_QZSSL6  0x73    // QZSSL6 data (NEO-D9C)
#define UBX_RXM_PMREQ  0x41     // Requests a Power Management task (two different packet sizes)
#define UBX_RXM_RAWX  0x15      // Multi-GNSS Raw Measurement Data
#define UBX_RXM_RLM  0x59       // Galileo SAR Short-RLM report (two different packet sizes)
#define UBX_RXM_RTCM  0x32      // RTCM input status
#define UBX_RXM_SFRBX  0x13     // Broadcast Navigation Data Subframe
#define UBX_RXM_SPARTN  0x33    // SPARTN input status
#define UBX_RXM_SPARTNKEY  0x36 // Poll/transfer dynamic SPARTN keys

// Class: SEC
// The following are used to configure the SEC UBX messages (security feature messages). Descriptions from UBX messages overview (ZED_F9P Interface Description Document page 36)
#define UBX_SEC_UNIQID  0x03 // Unique chip ID

// Class: TIM
// The following are used to configure the TIM UBX messages (timing messages). Descriptions from UBX messages overview (ZED_F9P Interface Description Document page 36)
#define UBX_TIM_TM2  0x03  // Time mark data
#define UBX_TIM_TP  0x01   // Time Pulse Timedata
#define UBX_TIM_VRFY  0x06 // Sourced Time Verification

// Class: UPD
// The following are used to configure the UPD UBX messages (firmware update messages). Descriptions from UBX messages overview (ZED-F9P Interface Description Document page 36)
#define UBX_UPD_SOS  0x14 // Poll Backup Fil Restore Status, Create Backup File in Flash, Clear Backup File in Flash, Backup File Creation Acknowledge, System Restored from Backup

// The following are used to enable RTCM messages
#define UBX_RTCM_MSB  0xF5    // All RTCM enable commands have 0xF5 as MSB
#define UBX_RTCM_1005  0x05   // Stationary RTK reference ARP
#define UBX_RTCM_1074  0x4A   // GPS MSM4
#define UBX_RTCM_1077  0x4D   // GPS MSM7
#define UBX_RTCM_1084  0x54   // GLONASS MSM4
#define UBX_RTCM_1087  0x57   // GLONASS MSM7
#define UBX_RTCM_1094  0x5E   // Galileo MSM4
#define UBX_RTCM_1097  0x61   // Galileo MSM7
#define UBX_RTCM_1124  0x7C   // BeiDou MSM4
#define UBX_RTCM_1127  0x7F   // BeiDou MSM7
#define UBX_RTCM_1230  0xE6   // GLONASS code-phase biases, set to once every 10 seconds
#define UBX_RTCM_4072_0  0xFE // Reference station PVT (ublox proprietary RTCM message)
#define UBX_RTCM_4072_1  0xFD // Additional reference station information (ublox proprietary RTCM message)

// Class: ACK
#define UBX_ACK_NACK  0x00
#define UBX_ACK_ACK  0x01
#define UBX_ACK_NONE  0x02 // Not a real value

// Class: ESF
//  The following constants are used to get External Sensor Measurements and Status
//  Information.
#define UBX_ESF_MEAS  0x02
#define UBX_ESF_RAW  0x03
#define UBX_ESF_STATUS  0x10
#define UBX_ESF_RESETALG  0x13
#define UBX_ESF_ALG  0x14
#define UBX_ESF_INS  0x15 // 36 bytes

// CFG-TMODE: Time mode configuration
#define SVIN_MODE_DISABLE  0x00
#define SVIN_MODE_ENABLE  0x01 // Survey-In
#define SVIN_MODE_FIXED  0x02

#define COM_TYPE_UBX  (1 << 0)
#define COM_TYPE_NMEA  (1 << 1)
#define COM_TYPE_RTCM3  (1 << 5)
#define COM_TYPE_SPARTN  (1 << 6)

// Odometer configuration - flags
#define UBX_CFG_ODO_USE_ODO  (1 << 0)
#define UBX_CFG_ODO_USE_COG  (1 << 1)
#define UBX_CFG_ODO_OUT_LP_VEL  (1 << 2)
#define UBX_CFG_ODO_OUT_LP_COG  (1 << 3)

// Odometer configuration - odoCfg
enum odoCfg_e
{
  UBX_CFG_ODO_RUN = 0,
  UBX_CFG_ODO_CYCLE,
  UBX_CFG_ODO_SWIM,
  UBX_CFG_ODO_CAR,
  UBX_CFG_ODO_CUSTOM,
};

// Configuration Sub-Section mask definitions for saveConfigSelective (UBX-CFG-CFG)
#define VAL_CFG_SUBSEC_IOPORT  0x00000001Lu   // ioPort - communications port settings (causes IO system reset!)
#define VAL_CFG_SUBSEC_MSGCONF  0x00000002Lu  // msgConf - message configuration
#define VAL_CFG_SUBSEC_INFMSG  0x00000004Lu   // infMsg - INF message configuration
#define VAL_CFG_SUBSEC_NAVCONF  0x00000008Lu  // navConf - navigation configuration
#define VAL_CFG_SUBSEC_RXMCONF  0x00000010Lu  // rxmConf - receiver manager configuration
#define VAL_CFG_SUBSEC_SENCONF  0x00000100Lu  // senConf - sensor interface configuration (requires protocol 19+)
#define VAL_CFG_SUBSEC_RINVCONF  0x00000200Lu // rinvConf - remove inventory configuration
#define VAL_CFG_SUBSEC_ANTCONF  0x00000400Lu  // antConf - antenna configuration
#define VAL_CFG_SUBSEC_LOGCONF  0x00000800Lu  // logConf - logging configuration
#define VAL_CFG_SUBSEC_FTSCONF  0x00001000Lu  // ftsConf - FTS configuration (FTS products only)

// Bitfield wakeupSources for UBX_RXM_PMREQ
#define VAL_RXM_PMREQ_WAKEUPSOURCE_UARTRX  0x00000008Lu  // uartrx
#define VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT0  0x00000020Lu // extint0
#define VAL_RXM_PMREQ_WAKEUPSOURCE_EXTINT1  0x00000040Lu // extint1
#define VAL_RXM_PMREQ_WAKEUPSOURCE_SPICS  0x00000080Lu   // spics

enum dynModel // Possible values for the dynamic platform model, which provide more accuract position output for the situation. Description extracted from ZED-F9P Integration Manual
{
  DYN_MODEL_PORTABLE = 0, // Applications with low acceleration, e.g. portable devices. Suitable for most situations.
  // 1 is not defined
  DYN_MODEL_STATIONARY = 2, // Used in timing applications (antenna must be stationary) or other stationary applications. Velocity restricted to 0 m/s. Zero dynamics assumed.
  DYN_MODEL_PEDESTRIAN,     // Applications with low acceleration and speed, e.g. how a pedestrian would move. Low acceleration assumed.
  DYN_MODEL_AUTOMOTIVE,     // Used for applications with equivalent dynamics to those of a passenger car. Low vertical acceleration assumed
  DYN_MODEL_SEA,            // Recommended for applications at sea, with zero vertical velocity. Zero vertical velocity assumed. Sea level assumed.
  DYN_MODEL_AIRBORNE1g,     // Airborne <1g acceleration. Used for applications with a higher dynamic range and greater vertical acceleration than a passenger car. No 2D position fixes supported.
  DYN_MODEL_AIRBORNE2g,     // Airborne <2g acceleration. Recommended for typical airborne environments. No 2D position fixes supported.
  DYN_MODEL_AIRBORNE4g,     // Airborne <4g acceleration. Only recommended for extremely dynamic environments. No 2D position fixes supported.
  DYN_MODEL_WRIST,          // Not supported in protocol versions less than 18. Only recommended for wrist worn applications. Receiver will filter out arm motion.
  DYN_MODEL_BIKE,           // Supported in protocol versions 19.2. (not available in all products)
  DYN_MODEL_MOWER,          // Added in HPS 1.21 (not available in all products)
  DYN_MODEL_ESCOOTER,       // Added in HPS 1.21 (not available in all products)
  DYN_MODEL_UNKNOWN = 255   // getDynamicModel will return 255 if sendCommand fails
};

// The GNSS identifiers - used by UBX-CFG-GNSS (0x06 0x3E) GNSS system configuration
enum sfe_ublox_gnss_ids_e
{
  SFE_UBLOX_GNSS_ID_GPS,
  SFE_UBLOX_GNSS_ID_SBAS,
  SFE_UBLOX_GNSS_ID_GALILEO,
  SFE_UBLOX_GNSS_ID_BEIDOU,
  SFE_UBLOX_GNSS_ID_IMES, // Note! IMES has no UBLOX_CFG_SIGNAL__ENA configuration key...
  SFE_UBLOX_GNSS_ID_QZSS,
  SFE_UBLOX_GNSS_ID_GLONASS,
  SFE_UBLOX_GNSS_ID_UNKNOWN
};

// The GNSS identifiers of leap second event info source - used by UBX-NAV-TIMELS
enum sfe_ublox_ls_src_e
{
  SFE_UBLOX_LS_SRC_DEFAULT,
  SFE_UBLOX_LS_SRC_GLONASS,
  SFE_UBLOX_LS_SRC_GPS,
  SFE_UBLOX_LS_SRC_SBAS,
  SFE_UBLOX_LS_SRC_BEIDOU,
  SFE_UBLOX_LS_SRC_GALILEO,
  SFE_UBLOX_LS_SRC_AIDED,
  SFE_UBLOX_LS_SRC_CONFIGURED,
  SFE_UBLOX_LS_SRC_UNKNOWN = 255
};

typedef enum
{
  SFE_UBLOX_MGA_ASSIST_ACK_NO,     // Do not expect UBX-MGA-ACK's. If the module outputs them, they will be ignored
  SFE_UBLOX_MGA_ASSIST_ACK_YES,    // Expect and check for UBX-MGA-ACK's
  SFE_UBLOX_MGA_ASSIST_ACK_ENQUIRE // Check UBX-CFG-NAVX5 ackAiding to determine if UBX-MGA-ACK's are expected
} sfe_ublox_mga_assist_ack_e;

// The infoCode byte included in UBX-MGA-ACK-DATA0
enum sfe_ublox_mga_ack_infocode_e
{
  SFE_UBLOX_MGA_ACK_INFOCODE_ACCEPTED,
  SFE_UBLOX_MGA_ACK_INFOCODE_NO_TIME,
  SFE_UBLOX_MGA_ACK_INFOCODE_NOT_SUPPORTED,
  SFE_UBLOX_MGA_ACK_INFOCODE_SIZE_MISMATCH,
  SFE_UBLOX_MGA_ACK_INFOCODE_NOT_STORED,
  SFE_UBLOX_MGA_ACK_INFOCODE_NOT_READY,
  SFE_UBLOX_MGA_ACK_INFOCODE_TYPE_UNKNOWN
};

// The mainTalkerId, set by UBX-CFG-NMEA setMainTalkerID
enum sfe_ublox_talker_ids_e
{
  SFE_UBLOX_MAIN_TALKER_ID_DEFAULT,
  SFE_UBLOX_MAIN_TALKER_ID_GP,
  SFE_UBLOX_MAIN_TALKER_ID_GL,
  SFE_UBLOX_MAIN_TALKER_ID_GN,
  SFE_UBLOX_MAIN_TALKER_ID_GA,
  SFE_UBLOX_MAIN_TALKER_ID_GB,
  SFE_UBLOX_MAIN_TALKER_ID_GQ
};

// The DGNSS differential mode
enum sfe_ublox_dgnss_mode_e
{
  SFE_UBLOX_DGNSS_MODE_FLOAT = 2, // No attempts are made to fix ambiguities
  SFE_UBLOX_DGNSS_MODE_FIXED      // Ambiguities are fixed whenever possible
};

// MON HW Antenna Status
enum sfe_ublox_antenna_status_e
{
  SFE_UBLOX_ANTENNA_STATUS_INIT,
  SFE_UBLOX_ANTENNA_STATUS_DONTKNOW,
  SFE_UBLOX_ANTENNA_STATUS_OK,
  SFE_UBLOX_ANTENNA_STATUS_SHORT,
  SFE_UBLOX_ANTENNA_STATUS_OPEN
};
