
#include "i2c2.h"
	
#define Delay5us() delay_us(5)
	
#define PIN_I2C PIN_PA14
#define PIN_I2D PIN_PA15
	
#define GET_DATA() port_pin_get_input_level(PIN_I2D)

static inline void TCLK0(void)
{
    port_pin_set_output_level(PIN_I2C,0); 
	delay_us(10);
}

static inline void TCLK1(void)
{
	port_pin_set_output_level(PIN_I2C,1);
	delay_us(10);
}

static inline void TDATA0(void)
{

	struct port_config pin_conf;

	port_pin_set_output_level(PIN_I2D,0);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as outputs*/
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	pin_conf.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(PIN_I2D, &pin_conf);

	delay_us(10);
}

static inline void TDATA1(void)
{

	struct port_config pin_conf;

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as input */
	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	pin_conf.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(PIN_I2D, &pin_conf);
	
	delay_us(10);	
}

static bool TI2C_Ack;

static void TI2C_Start(void)
{
		TCLK1();
		TDATA0();
		TCLK0();
}

static void TI2C_Stop(void)
{
		TDATA0();
		TCLK1();
		TDATA1();
}

//
// Send W to I2C
//
static bool TI2C_Out(U8 w)
{
	U8 count;

	for(count=0;count!=8;count++)
	{
		TCLK0();

		if(w & 128)
		TDATA1();
		else
		TDATA0();

		w<<=1;

		TCLK1();
	}

	TCLK0();
	TDATA1();
	TCLK1();

	TI2C_Ack=0;

	if(GET_DATA()==0u)
		TI2C_Ack=1;
		
	TCLK0();

	return TI2C_Ack;
}

	
//
// Read from I2C
//
static U8 TI2C_In(void)
{
		U8 count,w=0;

		TDATA1();

		for(count=0;count!=8;count++)
		{

			TCLK1();

			w<<=1;

			if(GET_DATA())
			  w|=1;

			TCLK0();
		}

		if(!TI2C_Ack)
		TDATA0();

		TCLK1();
		TCLK0();
		TDATA1();

		return w;
}

bool i2c2_readNBytes(U8 i2c_addr, U8 *i2c_buf, U8 i2c_buf_len)
{
	U8 i;

	// Form the read header

	TI2C_Start();
	TI2C_Out(1|(i2c_addr<<1));

	for(i=0;i!=i2c_buf_len;i++)
	{
		if(i == (i2c_buf_len-1))
		TI2C_Ack=1; // NAK the last byte
		else
		TI2C_Ack=0; // ACK other bytes

		i2c_buf[i]=TI2C_In();
		TI2C_Ack=0;
	}

	TI2C_Stop();

	return true;
}

bool i2c2_writeNBytes(U8 i2c_addr, U8 *i2c_buf, U8 i2c_buf_len)
{
	U8 i;

	// Form the write header

	TI2C_Start();
	TI2C_Out(i2c_addr<<1);

	for(i=0;i!=i2c_buf_len;i++)
	{
		if(!TI2C_Out(i2c_buf[i]))
			break;
	}

	TI2C_Stop();

	if(i == i2c_buf_len)
		return true;
	else
		return false;
}

bool i2c2_init(void)
{
	struct port_config pin_conf;

	port_pin_set_output_level(PIN_I2C,1);
	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as output */
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_I2C, &pin_conf);

	port_pin_set_output_level(PIN_I2D,0);
	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as input */
	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	port_pin_set_config(PIN_I2D, &pin_conf);

	return true;
}
