
// sfe_bus.c

#include "asf.h"
#include "sfe_bus.h"
#include "i2c1.h"

static    uint8_t _address;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // I2C init()
  //
  // Methods to init/setup this device.
  // The caller can provide a Wire Port, or this class will use the default.
  // Always update the address in case the user has changed the I2C address - see Example9
  bool SfeI2C_init(uint8_t address)
  {

    _address = address;

    return true;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // ping()
  //
  // Is a device connected?
  bool SfeI2C_ping(void)
  {
	uint8_t buffer[1];

	if(!i2c1_writeNBytes(_address, buffer, 0))
	  	return false;
	else
		return true;

  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // available()
  //
  // Checks how many bytes are waiting in the GNSS's I2C buffer
  // It does this by reading registers 0xFD and 0xFE
  //
  // From the u-blox integration manual:
  // "There are two forms of DDC read transfer. The "random access" form includes a peripheral register
  //  address and thus allows any register to be read. The second "current address" form omits the
  //  register address. If this second form is used, then an address pointer in the receiver is used to
  //  determine which register to read. This address pointer will increment after each read unless it
  //  is already pointing at register 0xFF, the highest addressable register, in which case it remains
  //  unaltered."

  uint16_t SfeI2C_available(void)
  {
	uint8_t buffer[2];
	uint16_t bytesAvailable = 0;

	buffer[0]=0xFD;

	if(!i2c1_writeNBytes(_address, buffer, 1))
	  	return 0;

	if(!i2c1_readNBytes(_address, buffer, 2))
	  	return 0;

    bytesAvailable = (uint16_t)buffer[0] << 8 | buffer[1];

    return (bytesAvailable);

	#if 0

   // Get the number of bytes available from the module
    uint16_t bytesAvailable = 0;
    _i2cPort->beginTransmission(_address);
    _i2cPort->write(0xFD);                               // 0xFD (MSB) and 0xFE (LSB) are the registers that contain number of bytes available
    uint8_t i2cError = _i2cPort->endTransmission(false); // Always send a restart command. Do not release the bus. ESP32 supports this.
    if (i2cError != 0)
    {
      return (0); // Sensor did not ACK
    }

    // Forcing requestFrom to use a restart would be unwise. If bytesAvailable is zero, we want to surrender the bus.
    uint16_t bytesReturned = _i2cPort->requestFrom(_address, static_cast<uint8_t>(2));
    if (bytesReturned != 2)
    {
      return (0); // Sensor did not return 2 bytes
    }
    else // if (_i2cPort->available())
    {
      uint8_t msb = _i2cPort->read();
      uint8_t lsb = _i2cPort->read();
      bytesAvailable = (uint16_t)msb << 8 | lsb;
    }

    return (bytesAvailable);

    #endif

  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // writeBytes()

  uint8_t SfeI2C_writeBytes(uint8_t *dataToWrite, uint8_t length)
  {

    if (length == 0)
      return 0;

#if 0
    _i2cPort->beginTransmission(_address);
    uint8_t written = _i2cPort->write((const uint8_t *)dataToWrite, length);
    if (_i2cPort->endTransmission() == 0)
      return written;
#endif

// not 100% accurate with finding out how many bytes got written

	if(!i2c1_writeNBytes(_address, dataToWrite, length))
	  	return 0;

    return length;
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // readBytes()

  uint8_t SfeI2C_readBytes(uint8_t *data, uint8_t length)
  {

    if (length == 0)
      return 0;

	if(!i2c1_readNBytes(_address, data, length))
	  	return 0;

// not 100% accurate with finding out how many bytes got read

	return length;

	#if 0
    uint8_t bytesReturned = _i2cPort->requestFrom(_address, length);

    for (uint8_t i = 0; i < bytesReturned; i++)
      *data++ = _i2cPort->read();

    return bytesReturned;
    #endif

  }
