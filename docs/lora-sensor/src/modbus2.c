/*!

\defgroup	Modbus Modbus LoRa Slave Interface
\author		Lindsay Meek

*/

#include <string.h>

#include "modbus.h"
#include "crc.h"
#include "miwi_api.h"
#include "p2p_demo.h"

/// Return from GETB() for nodata
#define GETB_NODATA  -1
/// Return from GETB() for timeout
#define GETB_TIMEOUT -2

static uint8_t	device_id;			// the current device ID
static uint8_t	state;				// state machine variable
static uint16_t	_index;				// index for packet assembly/disassembly
static uint32_t	wait_timer;		// timeout timer
static int16_t	data;				// data temp
static uint16_t	address;			// address temp
static uint16_t  length;				// length of transmission/reception
static uint16_t  mb_crc;				// transmitted / received crc
static uint8_t	header[MB_HEADER_LEN+2+256];

static uint8_t tbuf[256],rd_i,wr_i;

void rx_lora_data(uint16_t len, uint8_t *x);

void rx_lora_data(uint16_t len, uint8_t *x)
{
	uint16_t i;
	uint8_t wr_n;
	
	i=0;
	while(len--)
	{
		wr_n = wr_i + 1;
		
		if(wr_n != rd_i)
		{
			tbuf[wr_i] = x[i++];
			wr_i=wr_n;
		}
	}
}

static bool rx_empty(void)
{
	if(rd_i == wr_i)
	return true;
	else
	return false;
}


static uint8_t rx_getc(void)
{
	if(rx_empty())
	return 0;
	
	return tbuf[rd_i++];
}

///
/// Modbus slave FSM states
//
typedef enum  {
	MB_IDLE = 0,
	MB_GET_HEADER,
	MB_PUT_HEADER,
	MB_PUT_CRCHI,
	MB_PUT_CRCLO,
	MB_PUT_DATA,
	MB_PUT_DATAHI,
	MB_PUT_DATALO,
	MB_PUT_ERROR
	
 } states;

///
/// Receive a uint8_t, updating CRC
/// \return >=0 for data if successful, otherwise -ve number indicates error
/// \sa Timer_elapsed_msec, Timer_read, getch2, crc_update, kbhit2
///
static int16_t	getb(void)
	{
		uint8_t t;

		// any data?
		if(rx_empty())
		{
			// no, process timeout
			if(MiWi_TickGetDiff((MIWI_TICK)MiWi_TickGet(),(MIWI_TICK)wait_timer) >  MODBUS_TIMEOUT)
				return GETB_TIMEOUT;
			else
				return GETB_NODATA;
		}
		else
		{
			// reset timer
			wait_timer=MiWi_TickGet();

			t=rx_getc();
			
			// update crc
			crc_update(&mb_crc,t);

			// return data
			return t;
		}
	} // checked

///
/// Illegal Function called routine
///
static void	exception_response(uint8_t code)
	{

	// format and broadcast error message containing device number and function
	// code, with MSB set

	header[1] |= 0x80;
	header[2] = code;

	// reset preamble timer
	wait_timer = MiWi_TickGet();

	state = MB_PUT_ERROR;

	} // checked


///
/// Set state machine to idle (waiting for packet)
/// \sa crc_init, enable_tx2
///
static void	Modbus_Idle(void)
{
	// reset state machine
	state=MB_IDLE;
	// initialise CRC
	crc_init(&mb_crc);

	// reset timeout timer
	wait_timer = MiWi_TickGet();


} // checked


///
/// Initialise the modbus task
/// \sa open2, Modbus_Idle
/// \param device The modbus device ID
///
void	Modbus2_initialise(uint8_t device)
{
	// Set device ID
	if(device)
		device_id = device;
	else
		device_id = 1;	// don't allow device zero - choose one

	// Protocol goes to idle
	Modbus_Idle();

} // checked


///
/// Poll the modbus slave interface
/// \sa Modbus_Idle, enable_tx2,getb,Setpoint_read8,exception_response
/// \sa Read_register, Write_register, putc2, Setpoint_read
///
void 	Modbus2(bool test)
{
		uint16_t arg,i;
		uint8_t c;
		
		switch(state) {
			default:
			case MB_IDLE:	// scanning for device code

					if(test)
					{
						c='*';
						p2p_demo_send_data(0, 0, &c, 1) ;
						
					}
					else
					{

						data=getb();
						if((uint8_t)data == device_id)
						{

							_index=0;
							header[_index++]=(uint8_t)data;
							state=MB_GET_HEADER;

						}
						else
							Modbus_Idle();
					}

					break;

			case MB_GET_HEADER: // reading in header

					data=getb();
					if(data >= 0)
					{

						header[_index++] = (uint8_t)data;
						if(_index == MB_HEADER_LEN+2)
						{

							if(!mb_crc)
							{
								switch(header[1])
								{
									default:

										exception_response(ILLEGAL_FUNCTION);
										break;

									case READ_HOLDING_REGISTERS:
									case READ_INPUT_REGISTERS:

										length = header[4];
										length <<= 8;
										length += header[5];

										address = header[2];
										address <<= 8;
										address += header[3];

										// reset preamble timer
										wait_timer = MiWi_TickGet();

										if(length <= 127u)
										{
											
											header[2] = length << 1;
											state = MB_PUT_DATA;
										}
										else
											exception_response(ILLEGAL_DATA_ADDRESS);
										
										break;

									case PRESET_SINGLE_REGISTER:

										arg = header[4];
										arg <<= 8;
										arg += header[5];

										address = header[2];
										address <<= 8;
										address += header[3];

										if(!Write_register(address, arg))
										{
											// generate error if processing failed, else proceed to response state
											exception_response(ILLEGAL_DATA_ADDRESS);
										}
										else
										{
											// reset preamble timer
											wait_timer = MiWi_TickGet();

											state = MB_PUT_HEADER;
										}

										break;
								}
								crc_init(&mb_crc);
								

							}
							else
								Modbus_Idle();
						}
					}
					else
					{
						if(data == GETB_TIMEOUT)
							Modbus_Idle();
					}
					break;


			case MB_PUT_HEADER: // sending header

					crc_init(&mb_crc);
					for(i=0;i!=MB_HEADER_LEN;i++) 
						crc_update(&mb_crc,header[i]);
					header[i++]=(uint8_t)(mb_crc >> 8);
					header[i++]=(uint8_t)(mb_crc);
					
				 	if(p2p_demo_send_data(0, 0, header, i))
				 	{
						Modbus_Idle();
					}

				break;


			case MB_PUT_ERROR: // sending error response

					crc_init(&mb_crc);
					for(i=0;i!=3;i++) 
						crc_update(&mb_crc,header[i]);
					header[i++]=(uint8_t)(mb_crc >> 8);
					header[i++]=(uint8_t)(mb_crc);
					 
				 	if(p2p_demo_send_data(0, 0, header, i))
				 	{
						Modbus_Idle();
					}

				break;

			case MB_PUT_DATA:	// sending data response header

					for(i=0; i < header[2]; i+=2)
					{
						if ( header[1] == READ_HOLDING_REGISTERS )
							data = Read_Holding_Register(address+(i>>1));
						else
							data = Read_Input_Register(address+(i>>1));
							
						header[3+i]=(uint8_t)(data >> 8);
						header[4+i]=(uint8_t)data;
					}
					
					crc_init(&mb_crc);				
					for(i=0;i< 3+header[2];i++) 
						crc_update(&mb_crc, header[i]);		
					header[i++]=(uint8_t)(mb_crc >> 8);
					header[i++]=(uint8_t)(mb_crc);
	
					if(p2p_demo_send_data(0, 0, header, i))
					{
						Modbus_Idle();
					}
					break;
		}
}

/// Change the device ID
void	Modbus2_device(uint8_t device)
{
	if(device)
		device_id = device;
	else
		device_id = 1;	// don't allow device zero - choose one

}
