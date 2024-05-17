/*
 * \brief I2C EEPROMs data writer/reader library
 *
 * \author Quentin Comte-Gaz <quentin@comte-gaz.com>
 * \date 15 January 2023
 * \license MIT License (contact me if too restrictive)
 * \copyright Copyright (c) 2023 Quentin Comte-Gaz
 * \version 1.3
 *
 * \history
 *  - v1.0 Main design of the library
 *  - v1.1 Add Arduino < 1.0 compatibility
 *  - v1.2 Add CAT24Cxx compatibility
 *  - v1.3 Add update() method
 */

#ifndef I2CEEPROM_h
#define I2CEEPROM_h

#include "asf.h"

// Internal enum to set the number of bits used in I2C address for the memory address for CAT24CXX devices.
// The "Classic" device sends the address and then 16-bits for the address.
// The CAT24CXX devices include the 'n' MSB of the MEMORY address as the 'n' LSB of the I2C address.
//    Where n is 3 for the 16 kb, 2 for the 8 kb, 1 for the 4 kb, and 0 for the 2kb
enum address_mode
{
    ADDRESS_MODE_16BIT,
    ADDRESS_MODE_8BIT = 0,
    ADDRESS_MODE_9BIT = 1,
    ADDRESS_MODE_10BIT = 2,
    ADDRESS_MODE_11BIT = 3,
};

// Map device names to Addressing mode to make it easier to use
#define EEPROM_DEVICE_CLASSIC  ADDRESS_MODE_16BIT
#define EEPROM_DEVICE_CAT24C01 ADDRESS_MODE_8BIT
#define EEPROM_DEVICE_CAT24C02 ADDRESS_MODE_8BIT
#define EEPROM_DEVICE_CAT24C04 ADDRESS_MODE_9BIT
#define EEPROM_DEVICE_CAT24C08 ADDRESS_MODE_10BIT
#define EEPROM_DEVICE_CAT24C16 ADDRESS_MODE_11BIT
#define EEPROM_DEVICE_CAT24C512 ADDRESS_MODE_16BIT

#define CAT24_ADDR1 0x50
#define CAT24_ADDR2 0x51

    /*!
     * \brief I2CEEPROM Initialize I2C EEPROM instance
     * \param device_address (int) I2C address  of the EEPROM device (cascading for bigger devices)
     * \param addressing_mode (enum) Selects the device in use
     */
    void I2CEEPROM(uint8_t i2c_device_address , enum address_mode mode);

    /*!
     * \brief write Write one byte \p data in EEPROM device at EEPROM internal address \p address
     * \param address (unsigned int) EEPROM internal address (most of the time, first address is 0x00)
     * \param data (byte) Byte to write at EEPROM internal address \p address
     */
    void I2CEEPROM_write(uint32_t address, uint8_t data) ;

    /*!
     * \brief read Read one byte in EEPROM device at EEPROM internal address \p address
     * \param address (unsigned int) EEPROM internal address (most of the time, first address is 0x00)
     * \return (byte) Read Byte at EEPROM internal address \p address (returns 0xFF if an error occurred)
     */
    uint8_t I2CEEPROM_read(uint32_t address) ;

    /*!
     * \brief update Update one byte \p data in EEPROM device at EEPROM internal address \p address if not already the correct one (will read then write if not identical)
     * \param address (unsigned int) EEPROM internal address (most of the time, first address is 0x00)
     * \param data (byte) Byte to write at EEPROM internal address \p address
     */
    void I2CEEPROM_update(uint32_t address, uint8_t data) ;


#endif //I2CEEPROM_h

