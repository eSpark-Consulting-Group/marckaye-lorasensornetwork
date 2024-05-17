/*
 * \brief I2C EEPROMs data writer/reader (implementation)
 *
 * \author Quentin Comte-Gaz <quentin@comte-gaz.com>
 * \date 15 January 2023
 * \license MIT License (contact me if too restrictive)
 * \copyright Copyright (c) 2023 Quentin Comte-Gaz
 * \version 1.3
 */

#include "I2CEEPROM.h"
#include "i2c1.h"

    /*!
     * \brief read Combine I2C address with high byte of address for 512-2048 byte devices, return regular address for 16-bit addressed devices.
     * \param address (uint16_t) EEPROM internal address
     * \return (uint8_t) Read Byte at EEPROM internal address \p address (returns 0xFF if an error occurred)
     */
static    uint8_t I2CEEPROM_generate_I2C_address(uint32_t address);

static     int _i2c_device_address;
static     int _addressing_mode;
static     int _i2c_address_mask;

void I2CEEPROM(uint8_t i2c_device_address, enum address_mode addressing_mode)
{
  _i2c_device_address = i2c_device_address;
  _addressing_mode = addressing_mode;

  // Set the address mask (as defined in enum) to properly set I2C address when writing/reading
  if (addressing_mode == ADDRESS_MODE_16BIT)
  {
    _i2c_address_mask = 0xFF;
  }
  else
  {
    _i2c_address_mask = (0xFF << addressing_mode);
  }

}

void I2CEEPROM_write(uint32_t address, uint8_t data)
{
  // Generate the proper I2C address based on device mask and memory address
  uint8_t dev_address = I2CEEPROM_generate_I2C_address(address);
  uint8_t buffer[3];

	// In classic device mode, send the 8 MSB of the memory address.
	// This is already part of the device addr in CAT24CXX
	if (_addressing_mode == ADDRESS_MODE_16BIT)
	{
		buffer[0]=(uint8_t)(address >> 8);   // MSB
		buffer[1]=(uint8_t)(address & 0xFF); // LSB
		buffer[2]=data;

		if(!i2c1_writeNBytes(dev_address, buffer, 3))
			return;		
	}		
	else
	{
		buffer[0]=(uint8_t)(address & 0xFF);
		buffer[1]=data;
	
		if(!i2c1_writeNBytes(dev_address, buffer, 2))
			return;
	}

}

uint8_t I2CEEPROM_read(uint32_t address)
{
	// Generate the proper I2C address based on device mask and memory address
	uint8_t dev_address = I2CEEPROM_generate_I2C_address(address);
	uint8_t read_data = 0xFF;
	uint8_t buffer[2];

	if (_addressing_mode == ADDRESS_MODE_16BIT)
	{
	     buffer[0]=(uint8_t)(address >> 8);   // MSB
	     buffer[1]=(uint8_t)(address & 0xFF); // LSB
		 
		 if(!i2c1_writeNBytes(dev_address, buffer, 2))
			return -1;
	}
	else
	{
		buffer[0]=(uint8_t)(address & 0xFF); 
			     
		if(!i2c1_writeNBytes(dev_address, buffer, 1))
			     return -1;

	}

	if(!i2c1_readNBytes(dev_address, &read_data, 1))
		return -1;

	return read_data;

}

void I2CEEPROM_update(uint32_t address, uint8_t data)
{
  if (I2CEEPROM_read(address) != data)
  {
    I2CEEPROM_write(address, data);
  }
  else
  {
    // Data to write is the same as data already written
    // => No need to write !
  }
}

static uint8_t I2CEEPROM_generate_I2C_address(uint32_t address)
{
  uint8_t dev_address = _i2c_device_address;

  if (_addressing_mode != ADDRESS_MODE_16BIT)
  {
    // Set the N MSB of the address to the N LSB of the high-byte of the address
    dev_address = (_i2c_device_address & _i2c_address_mask) | ((address >> 8) & (~_i2c_address_mask));
  }
  else
  {
	  // higher order for cascading devices
	  dev_address += ((uint8_t)(address >> 16)) & 7;
  }
  return dev_address;
}
