
#include "i2c1.h"

/* master sercom pinmux setting */
#define CONF_I2C_MASTER_MODULE    SERCOM3
#define CONF_MASTER_SDA_PINMUX    PINMUX_PA16D_SERCOM3_PAD0
#define CONF_MASTER_SCK_PINMUX    PINMUX_PA17D_SERCOM3_PAD1
static struct i2c_master_module i2c_master_instance;

bool i2c1_init(void)
{
	enum status_code status;
	struct i2c_master_config config_i2c_master;

	/* master init testing */
	i2c_master_get_config_defaults(&config_i2c_master);
	config_i2c_master.buffer_timeout = 10000;
	config_i2c_master.pinmux_pad0    = CONF_MASTER_SDA_PINMUX;
	config_i2c_master.pinmux_pad1    = CONF_MASTER_SCK_PINMUX;
	status = i2c_master_init(&i2c_master_instance, CONF_I2C_MASTER_MODULE, &config_i2c_master);

	if(status == STATUS_OK)
	{
		
		i2c_master_enable(&i2c_master_instance);
		
		return true;
	}
	else
	return false;

}

bool i2c1_writeNBytes(U8 addr, U8 *buf, U8 len)
{
	bool rc;
	
	struct i2c_master_packet master_packet = {
		.address     = addr,
		.data_length = len,
		.data        = buf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};
	
	rc = i2c_master_write_packet_wait(&i2c_master_instance, &master_packet);
	
	return (rc == STATUS_OK);
}

bool i2c1_readNBytes(U8 addr, U8 *buf, U8 len)
{
	bool rc;
	
	struct i2c_master_packet master_packet = {
		.address     = addr,
		.data_length = len,
		.data        = buf,
		.ten_bit_address = false,
		.high_speed      = false,
		.hs_master_code  = 0x0,
	};
	
	rc=i2c_master_read_packet_wait(&i2c_master_instance, &master_packet);
	
	return (rc == STATUS_OK);
}
