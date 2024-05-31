
#ifndef	MODBUS_H
#define	MODBUS_H

#include "asf.h"


// How long to wait for a character before giving up on the packet
#define MODBUS_TIMEOUT 2000

/// Modbus function code for 4xxxx register read
#define	READ_HOLDING_REGISTERS		3u
/// Modbus function code for 3xxxx register read
#define	READ_INPUT_REGISTERS		4u
/// Modbus function code for 4xxxx register single write
#define	PRESET_SINGLE_REGISTER		6u

/// Modbus exception codes for illegal function code
#define	ILLEGAL_FUNCTION			1u
/// Modbus exception codes for illegal data address
#define	ILLEGAL_DATA_ADDRESS		2u

/// length of modbus header
#define MB_HEADER_LEN 6u

//
// initialise the modbus task
// \param device_id The modbus device ID
//
void	Modbus_initialise(uint8_t device_lo,uint8_t device_hi);
void	Modbus2_initialise(uint8_t device_lo,uint8_t device_hi);

//
// Poll the modbus slave interface
//
void	Modbus(bool test, int16_t byte);
void	Modbus2(bool test);

bool usb_port_open(void);

///
/// Handle write to modbus register
/// \param address The modbus address - 40001
/// \param data The data to write
/// \return TRUE if success
bool	Write_register(uint16_t address,uint16_t data);

///
/// Read data from a holding register
/// \param address The modbus address - 40001
/// \return The data from the register
///
uint16_t Read_Holding_Register(uint16_t address);

///
/// Read data from an input register
/// \param address The modbus address - 30001
/// \return The data from the register
///
uint16_t Read_Input_Register(uint16_t address);

#endif
