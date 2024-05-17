
#include "sensors.h"
#include "i2c1.h"
#include "i2c2.h"
#include "ADS1115.h"
#include "miwi_api.h"

// ms to wait for adc result before skipping
#define ADC_TIMEOUT 100

#include <sio2host.h>

// SGP41 interface

static uint8_t CalcCrc(uint8_t data0, uint8_t data1)
{
	uint8_t crc = 0xFF;
	uint8_t b;

	crc ^= data0;
	for(b = 8; b != 0; --b)
	{
		if(crc & 0x80)
		{
			crc = (crc << 1) ^ 0x31u;
		}
		else
		{
			crc = (crc << 1);
		}
	}

	crc ^= data1;
	for( b = 8; b != 0; --b)
	{
		if(crc & 0x80)
		{
			crc = (crc << 1) ^ 0x31u;
		}
		else
		{
			crc = (crc << 1);
		}
	}
	return crc;
}

int sgp41_getSerial(uint8_t *sno)
{
	uint8_t buffer[9];

	buffer[0]=0x36;
	buffer[1]=0x82;

	#if 0
	printf("TX:%x,%x",
	(int)buffer[0],
	(int)buffer[1]
	);
	#endif

	if(!i2c1_writeNBytes(SGP41_I2C_ADDRESS, buffer, 2))
	return -1;

	delay_ms(10);

	buffer[0]=0x55;
	buffer[1]=0x55;
	buffer[2]=0x55;
	buffer[3]=0x55;
	buffer[4]=0x55;
	buffer[5]=0x55;
	buffer[6]=0x55;
	buffer[7]=0x55;
	buffer[8]=0x55;

	if(!i2c1_readNBytes(SGP41_I2C_ADDRESS, buffer, 9))
		return -3;

	#if 0
	printf("RX:%x,%x,%x,%x,%x,%x,%x,%x,%x",

	(int)buffer[0],
	(int)buffer[1],
	(int)buffer[2],
	(int)buffer[3],
	(int)buffer[4],
	(int)buffer[5],
	(int)buffer[6],
	(int)buffer[7],
	(int)buffer[8]

	);
	#endif


	// printf("CRC0:%x",CalcCrc(buffer[0],buffer[1]));

	//printf("CRC1:%x",(int)CalcCrc(buffer[3],buffer[4]));

	//printf("CRC2:%x",(int)CalcCrc(buffer[6],buffer[7]));

	sno[0] = buffer[0];
	sno[1] = buffer[1];

	sno[2] = buffer[3];
	sno[3] = buffer[4];

	sno[4] = buffer[6];
	sno[5] = buffer[7];

	if(buffer[2] != CalcCrc(buffer[0],buffer[1]))
	return -1;

	if(buffer[5] != CalcCrc(buffer[3],buffer[4]))
	return -2;

	if(buffer[8] != CalcCrc(buffer[6],buffer[7]))
	return -3;


	return 0;
}

int sgp41_executeConditioning(void)
{
	uint8_t buffer[8];

	buffer[0]=0x26;
	buffer[1]=0x12;

	buffer[2]=0x80;
	buffer[3]=0x00;

	buffer[4]=0xa2;

	buffer[5]=0x66;
	buffer[6]=0x66;

	buffer[7]=0x93;

	#if 0
	printf("TX:%x,%x,%x,%x,%x,%x,%x,%x",
	(int)buffer[0],
	(int)buffer[1],
	(int)buffer[2],
	(int)buffer[3],
	(int)buffer[4],
	(int)buffer[5],
	(int)buffer[6],
	(int)buffer[7]);
	#endif

	if(!i2c1_writeNBytes(SGP41_I2C_ADDRESS, buffer, 8))
	return -1;

	delay_ms(55);

	buffer[0]=0x55;
	buffer[1]=0x55;
	buffer[2]=0x55;

	if(!i2c1_readNBytes(SGP41_I2C_ADDRESS, buffer, 3))
		return -2;

	#if 0
	printf("RX:%x,%x,%x",

	(int)buffer[0],
	(int)buffer[1],
	(int)buffer[2]
	);
	#endif

	//  printf("CRC:%x",(int)CalcCrc(buffer[0],buffer[1]));

	if(buffer[2] != CalcCrc(buffer[0],buffer[1]))
	return -1;

	return 0;
}

int sgp41_measureRawSignals(int32_t *srawVoc, int32_t *srawNox)
{
	uint8_t buffer[8];

	#if 0

	bool e;

	e=sgp41_getSerial(buffer);

	printf("Serial:%d:%x,%x,%x,%x,%x,%x\n",
	(int)e,
	(int)buffer[0],
	(int)buffer[1],
	(int)buffer[2],
	(int)buffer[3],
	(int)buffer[4],
	(int)buffer[5]
	);

	#endif

	buffer[0]=0x26;
	buffer[1]=0x19; // SGP40=F SGP41=19

	buffer[2]=0x80;
	buffer[3]=0x00;
	buffer[4]=0xa2;

	buffer[5]=0x66;
	buffer[6]=0x66;
	buffer[7]=0x93;

	#if 0
	printf("TX:%x,%x,%x,%x,%x,%x,%x,%x",
	(int)buffer[0],
	(int)buffer[1],
	(int)buffer[2],
	(int)buffer[3],
	(int)buffer[4],
	(int)buffer[5],
	(int)buffer[6],
	(int)buffer[7]);
	#endif

	if(!i2c1_writeNBytes(SGP41_I2C_ADDRESS, buffer, 8))
		return -1;

	delay_ms(55);

	buffer[0]=0x55;
	buffer[1]=0x55;
	buffer[2]=0x55;
	buffer[3]=0x55;
	buffer[4]=0x55;
	buffer[5]=0x55;

	if(!i2c1_readNBytes(SGP41_I2C_ADDRESS, buffer, 6))
		return -3;

	#if 0
	printf("RX:%x,%x,%x,%x,%x,%x",

	(int)buffer[0],
	(int)buffer[1],
	(int)buffer[2],
	(int)buffer[3],
	(int)buffer[4],
	(int)buffer[5]


	);
	#endif

	//printf("CRC0:%x", (int)CalcCrc(buffer[0],buffer[1]));

	if(buffer[2] != CalcCrc(buffer[0],buffer[1]))
	return -2;

	*srawVoc = (unsigned char)buffer[0];
	*srawVoc *= 256;
	*srawVoc += (unsigned char)buffer[1];

	// printf("CRC1:%x", (int)CalcCrc(buffer[3],buffer[4]));

	if(buffer[5] != CalcCrc(buffer[3],buffer[4]))
	return -2;

	*srawNox = (unsigned char)buffer[3];
	*srawNox *= 256;
	*srawNox += (unsigned char)buffer[4];

	//printf("VOC:%d NOX:%d", (int)*srawVoc, (int)*srawNox);


	return 0;
}

int sgp41_turnHeaterOff(void)
{
	uint8_t buffer[2];

	buffer[0]=0x36;
	buffer[1]=0x15;

	if(!i2c1_writeNBytes(SGP41_I2C_ADDRESS, buffer, 2))
	return -1;

	delay_ms(1);

	return 0;
}

bool SENSORS_getSGP41(int32_t *Voc, int32_t *Nox)
{
	int error;

	static  int32_t srawVoc = -1;
	static  int32_t srawNox = -1;

	// time in seconds needed for NOx conditioning
	static uint16_t conditioning_s = 10;

	if (conditioning_s > 0) {
		// During NOx conditioning (10s) SRAW NOx will remain 0
		error = sgp41_executeConditioning();
		conditioning_s--;
		} else {
		// Read Measurement
		error = sgp41_measureRawSignals(&srawVoc, &srawNox);
	}

	if ((error) || (conditioning_s > 0))
	return false;
	else {

		if(Nox!=NULL) *Nox=srawNox;
		if(Voc!=NULL) *Voc=srawVoc;

		//printf("VOC2:%d NOX2:%d\n", (int)srawVoc, (int)srawNox);

		return true;

	}

}

// SGP40 interface

int sgp40_getSerial(uint8_t *sno)
{
	uint8_t buffer[9];

	buffer[0]=0x36;
	buffer[1]=0x82;

	#if 0
	printf("TX:%x,%x",
	(int)buffer[0],
	(int)buffer[1]
	);
	#endif

i2c2_writeNBytes(SGP40_I2C_ADDRESS, buffer, 2);


	//if(!i2c2_writeNBytes(SGP40_I2C_ADDRESS, buffer, 2))
	//return -1;

	delay_ms(10);

	buffer[0]=0x55;
	buffer[1]=0x55;
	buffer[2]=0x55;
	buffer[3]=0x55;
	buffer[4]=0x55;
	buffer[5]=0x55;
	buffer[6]=0x55;
	buffer[7]=0x55;
	buffer[8]=0x55;

	if(!i2c2_readNBytes(SGP40_I2C_ADDRESS, buffer, 9))
		return -4;

	#if 0
	printf("RX:%x,%x,%x,%x,%x,%x,%x,%x,%x",

	(int)buffer[0],
	(int)buffer[1],
	(int)buffer[2],
	(int)buffer[3],
	(int)buffer[4],
	(int)buffer[5],
	(int)buffer[6],
	(int)buffer[7],
	(int)buffer[8]

	);
	#endif


	// printf("CRC0:%x",CalcCrc(buffer[0],buffer[1]));

	//printf("CRC1:%x",(int)CalcCrc(buffer[3],buffer[4]));

	//printf("CRC2:%x",(int)CalcCrc(buffer[6],buffer[7]));

	sno[0] = buffer[0];
	sno[1] = buffer[1];

	sno[2] = buffer[3];
	sno[3] = buffer[4];

	sno[4] = buffer[6];
	sno[5] = buffer[7];

	if(buffer[2] != CalcCrc(buffer[0],buffer[1]))
	return -1;

	if(buffer[5] != CalcCrc(buffer[3],buffer[4]))
	return -2;

	if(buffer[8] != CalcCrc(buffer[6],buffer[7]))
	return -3;


	return 0;
}

int sgp40_measureRawSignal(int32_t *srawVoc)
{
	uint8_t buffer[8];

	buffer[0]=0x26;
	buffer[1]=0x0F; 
	 
	buffer[2]=0x80;
	buffer[3]=0x00;
	
	buffer[4]=0xa2;
	buffer[5]=0x66;
	
	buffer[6]=0x66;
	buffer[7]=0x93;

	if(!i2c2_writeNBytes(SGP40_I2C_ADDRESS, buffer, 8))
	  return -1;

	delay_ms(55);

	buffer[0]=0x55;
	buffer[1]=0x55;
	buffer[2]=0x55;
	
	if(!i2c2_readNBytes(SGP40_I2C_ADDRESS, buffer, 3))
		return -2;

	#if 0
	printf("RX:%x,%x,%x",

	(int)buffer[0],
	(int)buffer[1],
	(int)buffer[2]


	);
	#endif

	//printf("CRC0:%x", (int)CalcCrc(buffer[0],buffer[1]));

	//if(buffer[2] != CalcCrc(buffer[0],buffer[1]))
	//return -3;

	*srawVoc = (unsigned char)buffer[0];
	*srawVoc <<= 8;
	*srawVoc += (unsigned char)buffer[1];

	//printf("VOC:%d", (int)*srawVoc);

	return 0;
}

int sgp40_turnHeaterOff(void)
{
	uint8_t buffer[2];

	buffer[0]=0x36;
	buffer[1]=0x15;

	if(!i2c2_writeNBytes(SGP40_I2C_ADDRESS, buffer, 2))
	return -1;

	delay_ms(1);

	return 0;
}

bool SENSORS_getSGP40(int32_t *Voc)
{
	int error;

	static  int32_t srawVoc = -1;

	// Read Measurement
	error = sgp40_measureRawSignal(&srawVoc);

	if (error)
	{
		
		return false;
	}
	else
	{

		if(Voc!=NULL) *Voc=srawVoc;

		return true;
	}

}

#define SHT45_ADDR 0x44

bool SENSORS_getSHT45(uint16_t *temp, uint16_t *humidity, bool activate_heater)
{
	uint8_t buffer[6];
	//uint8_t checksum_t,checksum_rh;
	//float _t,_h;

	if(activate_heater)
		buffer[0]=0x32;		// 200mW for 0.1s
	else
		buffer[0]=0xFD;

	if(!i2c1_writeNBytes(SHT45_ADDR, buffer, 1))
		return false;

	if(activate_heater)
		delay_ms(110);
	else
		delay_ms(10);

	if(!i2c1_readNBytes(SHT45_ADDR, buffer, 6))
		return false;

	uint16_t t_ticks = ((uint16_t)buffer[0] << 8) | buffer[1];
	//checksum_t = buffer[2];
	uint16_t rh_ticks = ((uint16_t)buffer[3] << 8) | buffer[4];
	//checksum_rh = buffer[5];
	
#if 0
	_t = -45.0f + (175.0f * ((float)t_ticks / 65535.0f));
	_h = -6.0f + (125.0f * ((float)rh_ticks / 65535.0f));

	if (_h > 100.0f)
		_h = 100.0f;
	else if (_h < 0.0f)
		_h = 0.0f;
		
	if(temp!=NULL) *temp=_t;
	if(humidity!=NULL) *humidity=_h;

#else

	if(temp!=NULL) *temp=t_ticks;
	if(humidity!=NULL) *humidity=rh_ticks;
	
#endif
	
	return true;

}

// LMP91000 interface

#define LMP91000_I2C_Address                           (0x48)                  // Device Address
#define TI_LMP91000_STATUS_REG                         (0x00)                  /* Read only status register */
#define TI_LMP91000_LOCK_REG                           (0x01)                  /* Protection Register */
#define TI_LMP91000_TIACN_REG                          (0x10)                  /* TIA Control Register */
#define TI_LMP91000_REFCN_REG                          (0x11)                  /* Reference Control Register*/
#define TI_LMP91000_MODECN_REG                         (0x12)                  /* Mode Control Register */
#define TI_LMP91000_WRITE_LOCK                         (0x01)
#define TI_LMP91000_WRITE_UNLOCK                       (0x00)
#define TI_LMP91000_READY                              (0x01)
#define TI_LMP91000_NOT_READY                          (0x00)

static const uint8_t TI_LMP91000_TIACN_REG_VALUE[2] = {
    (0x0F),              /* TIA_GAIN = 7K, RLOAD = 100ohm  */
    (0x0C)               /* TIA_GAIN = 7K, RLOAD = 10ohm  */
};

static const uint8_t  TI_LMP91000_REFCN_REG_VALUE[2] = {
                    (0x20),                  /* Internal Ref, INT_Z 50%, 0% bias  */
                    (0x20)                  /* Internal Ref, INT_Z 50%, 0% bias  */
};

static const uint8_t  TI_LMP91000_MODECN_REG_VALUE [2] = {
                  (0x03),                 /* 3lead  */
                  (0x03)                  /* 3lead  */
};

static uint8_t LMP91000_I2CReadReg(uint8_t reg)
{
    uint8_t data[1];

    i2c1_writeNBytes(LMP91000_I2C_Address, &reg, 1);

    i2c1_readNBytes(LMP91000_I2C_Address, data, 1);

    return data[0];

}

static void LMP91000_I2CWriteReg(uint8_t reg,uint8_t val)
{
      uint8_t data[2];

      data[0]=reg;
      data[1]=val;

    i2c1_writeNBytes(LMP91000_I2C_Address, data, 2);

}

bool SENSORS_setLMPValue(void)
{
	static uint8_t state=0;
	uint8_t status;
	uint8_t read_val[4];

	switch(state)
	{
		default:
		case 0:

		state =1;

		// fall thru

		case 1:

		status = LMP91000_I2CReadReg(TI_LMP91000_STATUS_REG);                       // Read device ready status

		if (status == TI_LMP91000_NOT_READY)
		{

			return false;
		}

		read_val[0] = LMP91000_I2CReadReg(TI_LMP91000_LOCK_REG);                     // Read from lock register default value
		read_val[1] = LMP91000_I2CReadReg(TI_LMP91000_TIACN_REG);                    // Read TIA control register default value
		read_val[2] = LMP91000_I2CReadReg(TI_LMP91000_REFCN_REG);                    // Read Reference control register default value
		read_val[3] = LMP91000_I2CReadReg(TI_LMP91000_MODECN_REG);                   // Read Mode control register default value

		LMP91000_I2CWriteReg(TI_LMP91000_LOCK_REG, TI_LMP91000_WRITE_UNLOCK);        // unlock the registers for write

		LMP91000_I2CWriteReg(TI_LMP91000_TIACN_REG, TI_LMP91000_TIACN_REG_VALUE[0]);    // Modify TIA control register
		LMP91000_I2CWriteReg(TI_LMP91000_REFCN_REG, TI_LMP91000_REFCN_REG_VALUE[0]);    // Modify REF control register
		LMP91000_I2CWriteReg(TI_LMP91000_MODECN_REG, TI_LMP91000_MODECN_REG_VALUE[0]);  // 3-lead amperometric cell

		read_val[0]   = LMP91000_I2CReadReg(TI_LMP91000_TIACN_REG);                  // Read to confirm register is modified
		read_val[1]   = LMP91000_I2CReadReg(TI_LMP91000_REFCN_REG);                  // Read to confirm register is modified
		read_val[2]   = LMP91000_I2CReadReg(TI_LMP91000_MODECN_REG);                 // Read to confirm register is modified

		// test if write/read values match
		if ((read_val[0] != TI_LMP91000_TIACN_REG_VALUE[0]) ||
		(read_val[1] != TI_LMP91000_REFCN_REG_VALUE[0]) ||
		(read_val[2] != TI_LMP91000_MODECN_REG_VALUE[0])
		)                            // test values took effect
		{

			return false;
		}

		state=2;

		// fall thru
		case 2:

		// AFE initialised .. nothing further to do
		break;
	}

	return true;
	
}

static uint32_t adc_timer;

void ADC_StartConversion(uint8_t channel)
{
	switch(channel)
	{
		default:
		break;

		case ADC_CHAN_302:
			ADS1115(ADS1115_I2C_ADDR_VDD);	// Top ADC
			ADS1115_setMultiplexer(ADS1115_MUX_AIN0_GND);
			break;

		case ADC_CHAN_402:
			ADS1115(ADS1115_I2C_ADDR_VDD);	// Top ADC
			ADS1115_setMultiplexer(ADS1115_MUX_AIN1_GND);
			break;

	case ADC_CHAN_502:
			ADS1115(ADS1115_I2C_ADDR_VDD);	// Top ADC
			ADS1115_setMultiplexer(ADS1115_MUX_AIN2_GND);
			break;

	case ADC_CHAN_602:
			ADS1115(ADS1115_I2C_ADDR_VDD);	// Top ADC
			ADS1115_setMultiplexer(ADS1115_MUX_AIN3_GND);
			break;

	case ADC_CHAN_702:
			ADS1115(ADS1115_I2C_ADDR_SDA);	// Bottom ADC
			ADS1115_setMultiplexer(ADS1115_MUX_AIN2_GND);
			break;

	case ADC_CHAN_802:
			ADS1115(ADS1115_I2C_ADDR_SDA);	// Bottom ADC
			ADS1115_setMultiplexer(ADS1115_MUX_AIN3_GND);
			break;

	case ADC_CHAN_O2:
			ADS1115(ADS1115_I2C_ADDR_SDA);	// Bottom ADC
			ADS1115_setMultiplexer(ADS1115_MUX_AIN0_GND);
			break;

	case ADC_CHAN_NDIR:
			ADS1115(ADS1115_I2C_ADDR_SDA);	// Bottom ADC
			ADS1115_setMultiplexer(ADS1115_MUX_AIN1_GND);
			break;

	}

	delay_ms(1);

	ADS1115_startSingleConvertion();
	
	adc_timer=MiWi_TickGet();
}

bool ADC_WaitConversion(void)
{
	if(ADS1115_getOperationalStatus() == ADS1115_OS_NO_CONV)
		return true;
	
	//if(MiWi_TickGetDiff((MIWI_TICK)MiWi_TickGet(),(MIWI_TICK)adc_timer) >  ADC_TIMEOUT)
	//	return true;
		
	return false;
}

reg_val_t ADC_ReadConversion(void)
{
	return ADS1115_readRawValue();	
}

reg_val_t ADC_GetConversion(uint8_t channel)
{
	
	ADC_StartConversion(channel);
	
	while(!ADC_WaitConversion()) ;
	
	return ADC_ReadConversion();
}

bool ADC_init(void)
{
	bool rc1,rc2;

		ADS1115(ADS1115_I2C_ADDR_SDA);	// Bottom ADC

		rc2=ADS1115_reset();

		if(rc2)
		{
			ADS1115_setDataRate(ADS1115_DR_8_SPS);
			ADS1115_setDeviceMode(ADS1115_MODE_SINGLE);
			ADS1115_setPga(ADS1115_PGA_1_024_MULT);
		}


	ADS1115(ADS1115_I2C_ADDR_VDD);	// Top ADC

	rc1=ADS1115_reset();

	if(rc1)
	{
		ADS1115_setDataRate(ADS1115_DR_8_SPS);
		ADS1115_setDeviceMode(ADS1115_MODE_SINGLE);
		ADS1115_setPga(ADS1115_PGA_1_024_MULT);
	}


	return (rc1 && rc2);
}

void SENSORS_5V_CTRL(bool state)
{
	port_pin_set_output_level(PIN_5V_CNTL,state);
}

void SENSORS_3V3_SENS(bool state)
{
	port_pin_set_output_level(PIN_3V3_SENS,state);
}

void SENSORS_MUXSEL(bool state)
{
	port_pin_set_output_level(PIN_MUXSEL,state);
}

void SENSORS_SERVO(bool state)
{
	port_pin_set_output_level(PIN_SERVO,state);
}

bool SENSORS_getButton(void)
{
	return port_pin_get_input_level(PIN_BUTTON);
}

bool SENSORS_getAccInt(void)
{
	return port_pin_get_input_level(PIN_ACC_INT);
}


static struct adc_module adc_instance;

uint16_t SENSORS_getLight(void)
{
	static uint16_t result=0;

	adc_read(&adc_instance, &result);

	return result;
}

bool SENSORS_init(void)
{
	struct adc_config config_adc;
	struct port_config pin_conf;

	adc_get_config_defaults(&config_adc);
	config_adc.reference=ADC_REFERENCE_INTVCC2;
	config_adc.positive_input=ADC_INPUTCTRL_MUXPOS_AIN6; // light
	config_adc.freerunning=true;
	adc_init(&adc_instance, ADC, &config_adc);
	adc_enable(&adc_instance);

	SENSORS_5V_CTRL(0);
	SENSORS_3V3_SENS(0);
	SENSORS_MUXSEL(0);
	SENSORS_SERVO(0);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as input */
	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	port_pin_set_config(PIN_BUTTON, &pin_conf);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as input */
	pin_conf.direction  = PORT_PIN_DIR_INPUT;
	port_pin_set_config(PIN_ACC_INT, &pin_conf);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as outputs*/
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_MUXSEL, &pin_conf);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as outputs*/
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_3V3_SENS, &pin_conf);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as outputs*/
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_5V_CNTL, &pin_conf);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as outputs*/
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_SERVO, &pin_conf);

	// Set it to a nominal level

	port_pin_set_output_level(PIN_GPS_RESET,1);

	port_get_config_defaults(&pin_conf);
	/* Configure the GPIO pin as outputs*/
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_GPS_RESET, &pin_conf);

	// External ADCs
	return ADC_init();

}
