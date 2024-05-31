/*!

\defgroup	Modbus Modbus RTU Slave Interface
\author		Lindsay Meek

*/

#include <string.h>
#include "modbus.h"
#include "crc.h"
#include "udi_cdc.h"
#include "miwi_api.h"

#include "p2p_demo.h"



/// Return from GETB() for nodata
#define GETB_NODATA  -1
/// Return from GETB() for timeout
#define GETB_TIMEOUT -2

static uint8_t	device_id_lo,device_id_hi;			// range of valid device IDs
static uint8_t	state;				// state machine variable
static uint8_t	_index;				// index for packet assembly/disassembly
static uint32_t	wait_timer;		// timeout timer
static int16_t	data;				// data temp

static uint16_t  mb_crc;				// transmitted / received crc
static uint8_t	header[MB_HEADER_LEN+2];

///
/// Modbus slave FSM states
//
typedef enum  {
	MB_IDLE = 0,
	MB_GET_HEADER,

	MB_PUT_CRCHI,
	MB_PUT_CRCLO,
	
	MB_PUT_ERROR
 } states;


extern void FireLED(void);


///
/// Transmit a uint8_t, updating CRC
/// \param b value to transmit
/// \return 1 if successful
/// \sa putc2, crc_update
///
static uint8_t putb(uint8_t b)
	{
		if(!udi_cdc_is_tx_ready())
			return 0;

		if(udi_cdc_putc(b))
		{
			//DumpNum(b);

		 	crc_update(&mb_crc,b);
		 	return 1;
		}
		else
			return 0;
	} // checked


///
/// Receive a uint8_t, updating CRC
/// \return >=0 for data if successful, otherwise -ve number indicates error
/// \sa Timer_elapsed_msec, Timer_read, getch2, crc_update, kbhit2
///
static int16_t	getb(int16_t byte)
	{
		uint8_t t;

		// any data?
		if(0)//!udi_cdc_is_rx_ready()
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

			// read data
			t=byte;

			// update crc
			crc_update(&mb_crc,t);

			// return data
			printf("Value from t: %d\r\n",t);
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
/// \sa Modbus_Idle
/// \param device_lo,device_hi The modbus device ID range
///
void	Modbus_initialise(uint8_t device_lo, uint8_t device_hi)
{
	device_id_lo = device_lo;
	device_id_hi = device_hi;
	
	// Protocol goes to idle
	Modbus_Idle();

} // checked


///
/// Poll the modbus slave interface
/// \sa Modbus_Idle, enable_tx2,getb,Setpoint_read8,exception_response
/// \sa Read_register, Write_register, putc2, Setpoint_read
///
void 	Modbus(bool test, int16_t byte)
{
	bool cont=true;
	//while(cont)
	//{
		cont=false;
		
		switch(state) 
		{
			default:
			case MB_IDLE:	// scanning for device code

					if(test)
					{

						udi_cdc_putc('*');
					}
					else
					{

						data=getb(byte);
						if((uint8_t)data >= device_id_lo && (uint8_t)data <= device_id_hi)
						{

							_index=0;
							header[_index++]=(uint8_t)data;
							state=MB_GET_HEADER;
							cont=true;
						}
						//else
							//Modbus_Idle();
					}

					break;

			case MB_GET_HEADER: // reading in header

					data=getb(byte);
					if(data >= 0)
					{
						cont=true;
						header[_index++] = (uint8_t)data;
						if(_index == MB_HEADER_LEN+2)
						{

							if(!mb_crc)
							{
								switch(header[1])
								{
									default:
//
										exception_response(ILLEGAL_FUNCTION);
										break;

									case READ_HOLDING_REGISTERS:
									case READ_INPUT_REGISTERS:
									case PRESET_SINGLE_REGISTER:
									
										 //forward modbus packet to LoRa channel
										printf("Header: ");
										for(int j=0;j<MB_HEADER_LEN+2;j++)
											printf("%x \r\n",header[j]);
										p2p_demo_send_data(0, 0, header, MB_HEADER_LEN+2) ;

										// wait for next one
										
										Modbus_Idle();
										return;

								}
								crc_init(&mb_crc);
								_index=0;

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
					
			case MB_PUT_ERROR: // sending error response

			//		FireLED();

				 	//if(putb(header[_index]))
				 	//{
					 //cont=true;
					 //
					 //if(++_index >= 3u)
						//state = MB_PUT_CRCHI;
					//}

				break;


			case MB_PUT_CRCHI:

			//		FireLED();

					 // sending CRC code MSB
					 //if(!udi_cdc_is_tx_ready())
						 //break;
//
					 //if(udi_cdc_putc(mb_crc >> 8))
					 //{
						 //cont=true;
					 	//state = MB_PUT_CRCLO;
					 //}
					break;

			case MB_PUT_CRCLO:

			//		FireLED();

					 // sending CRC code MSB
					 //if(!udi_cdc_is_tx_ready())
						//break;
					 
					 //if(udi_cdc_putc(mb_crc & 0xFF))
					 //{
							//Modbus_Idle();
					 //}
					break;


		}
	//}
}
