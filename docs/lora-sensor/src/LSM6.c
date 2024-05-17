
#include "asf.h"
#include "i2c1.h"
#include "LSM6.h"

static enum deviceType _device; // chip type
static uint8_t address;


// Defines ////////////////////////////////////////////////////////////////

// The Arduino two-wire interface uses a 7-bit number for the address,
// and sets the last bit correctly based on reads and writes
#define DS33_SA0_HIGH_ADDRESS 0b1101011
#define DS33_SA0_LOW_ADDRESS  0b1101010
#define DSO_SA0_HIGH_ADDRESS  0b1101011
#define DSO_SA0_LOW_ADDRESS   0b1101010

#define TEST_REG_ERROR -1

#define DS33_WHO_ID    0x69
#define DSO_WHO_ID    0x6C

#define TEST_REG_ERROR -1

static int16_t LSM6_testReg(uint8_t addr, enum regAddr reg)
{
	uint8_t buffer[1];

	buffer[0]=reg;

	if(!i2c1_writeNBytes(addr, buffer, 1))
		return TEST_REG_ERROR;

	if(!i2c1_readNBytes(addr, buffer, 1))
		return TEST_REG_ERROR;

	return buffer[0];
	
#if 0
	bus->beginTransmission(address);
	bus->write((uint8_t)reg);
	if (bus->endTransmission() != 0)
	{
		return TEST_REG_ERROR;
	}

	bus->requestFrom(address, (uint8_t)1);
	if (bus->available())
	{
		return bus->read();
	}
	else
	{
		return TEST_REG_ERROR;
	}
#endif

}

// Public Methods //////////////////////////////////////////////////////////////

bool LSM6_init(enum deviceType device, enum sa0State sa0)
{
  // perform auto-detection unless device type and SA0 state were both specified
  if (device == device_auto || sa0 == sa0_auto)
  {
    // check for LSM6DS33 if device is unidentified or was specified to be this type
    if (device == device_auto || device == device_DS33)
    {
      // check SA0 high address unless SA0 was specified to be low
      if (sa0 != sa0_low && LSM6_testReg(DS33_SA0_HIGH_ADDRESS, WHO_AM_I) == DS33_WHO_ID)
      {
        sa0 = sa0_high;
        device = device_DS33;
      }
      // check SA0 low address unless SA0 was specified to be high
      else if (sa0 != sa0_high && LSM6_testReg(DS33_SA0_LOW_ADDRESS, WHO_AM_I) == DS33_WHO_ID)
      {
        sa0 = sa0_low;
        device = device_DS33;
      }
    }

    // check for LSM6DSO if device is unidentified or was specified to be this type
    if (device == device_auto || device == device_DSO)
    {
      // check SA0 high address unless SA0 was specified to be low
      if (sa0 != sa0_low && LSM6_testReg(DSO_SA0_HIGH_ADDRESS, WHO_AM_I) == DSO_WHO_ID)
      {
        sa0 = sa0_high;
        device = device_DSO;
      }
      // check SA0 low address unless SA0 was specified to be high
      else if (sa0 != sa0_high && LSM6_testReg(DSO_SA0_LOW_ADDRESS, WHO_AM_I) == DSO_WHO_ID)
      {
        sa0 = sa0_low;
        device = device_DSO;
      }
    }

    // make sure device and SA0 were successfully detected; otherwise, indicate failure
    if (device == device_auto || sa0 == sa0_auto)
    {
      return false;
    }
  }

  _device = device;

  switch (device)
  {
    case device_DS33:
      address = (sa0 == sa0_high) ? DS33_SA0_HIGH_ADDRESS : DS33_SA0_LOW_ADDRESS;
      break;

    case device_DSO:
      address = (sa0 == sa0_high) ? DSO_SA0_HIGH_ADDRESS : DSO_SA0_LOW_ADDRESS;
      break;

    default:
      return false; // this should not ever happen
  }

  return true;
}

enum deviceType LSM6_getDeviceType(void)
    { return _device; }

/*
Enables the LSM6's accelerometer and gyro. Also:
- Sets sensor full scales (gain) to default power-on values, which are
  +/- 2 g for accelerometer and 245 dps for gyro
- Selects 1.66 kHz (high performance) ODR (output data rate) for accelerometer
  and 1.66 kHz (high performance) ODR for gyro. (These are the ODR settings for
  which the electrical characteristics are specified in the LSM6DS33 datasheet.)
- Enables automatic increment of register address during multiple byte access
Note that this function will also reset other settings controlled by
the registers it writes to.
*/
void LSM6_enableDefault(void)
{
  if (_device == device_DS33 || _device == device_DSO)
  {
    // Accelerometer

    // 0x80 = 0b10000000
    // ODR = 1000 (1.66 kHz (high performance)); FS_XL = 00 (+/-2 g full scale)
    LSM6_writeReg(CTRL1_XL, 0x80);

    // Gyro

    // 0x80 = 0b010000000
    // ODR = 1000 (1.66 kHz (high performance)); FS_G = 00 (245 dps for DS33, 250 dps for DSO)
    LSM6_writeReg(CTRL2_G, 0x80);

    // Common

    // 0x04 = 0b00000100
    // IF_INC = 1 (automatically increment register address)
    LSM6_writeReg(CTRL3_C, 0x04);
  }
}

void LSM6_writeReg(uint8_t reg, uint8_t value)
{
	uint8_t buffer[2];

	buffer[0]=reg;
	buffer[1]=value;

	i2c1_writeNBytes(address, buffer, 2);

 #if 0
  bus->beginTransmission(address);
  bus->write(reg);
  bus->write(value);
  last_status = bus->endTransmission();
  #endif

}

uint8_t LSM6_readReg(uint8_t reg)
{
	uint8_t buffer[1];

	buffer[0]=reg;

	if(!i2c1_writeNBytes(address, buffer, 1))
			return -1;

	if(!i2c1_readNBytes(address, buffer, 1))
			return -1;

  return buffer[0];

#if 0
  uint8_t value;

  bus->beginTransmission(address);
  bus->write(reg);
  last_status = bus->endTransmission();

  bus->requestFrom(address, (uint8_t)1);
  value = bus->read();

  return value;
#endif

}

// Reads the 3 accelerometer channels and stores them in vector a
bool LSM6_readAcc(void)
{
	uint8_t buffer[6];

	LSM6_last_a[0] = LSM6_a[0];
	LSM6_last_a[1] = LSM6_a[1];
	LSM6_last_a[2] = LSM6_a[2];

	buffer[0]=OUTX_L_XL;

	if(!i2c1_writeNBytes(address, buffer, 1))
			return false;

	if(!i2c1_readNBytes(address, buffer, 6))
			return false;

	LSM6_a[0] = ((int16_t)buffer[1] << 8) | (int16_t)buffer[0];
	LSM6_a[1] = ((int16_t)buffer[3] << 8) | (int16_t)buffer[2];
	LSM6_a[2] = ((int16_t)buffer[5] << 8) | (int16_t)buffer[4];

	LSM6_da[0] = LSM6_a[0] - LSM6_last_a[0];
	LSM6_da[1] = LSM6_a[1] - LSM6_last_a[1];
	LSM6_da[2] = LSM6_a[2] - LSM6_last_a[2];

	return true;

#if 0
  bus->beginTransmission(address);
  // automatic increment of register address is enabled by default (IF_INC in CTRL3_C)
  bus->write(OUTX_L_XL);
  bus->endTransmission();

  bus->requestFrom(address, (uint8_t)6);
  uint8_t xla = bus->read();
  uint8_t xha = bus->read();
  uint8_t yla = bus->read();
  uint8_t yha = bus->read();
  uint8_t zla = bus->read();
  uint8_t zha = bus->read();

  // combine high and low bytes
  a.x = (int16_t)(xha << 8 | xla);
  a.y = (int16_t)(yha << 8 | yla);
  a.z = (int16_t)(zha << 8 | zla);
  #endif

}

// Reads the 3 gyro channels and stores them in vector g
bool LSM6_readGyro(void)
{
	uint8_t buffer[6];

	LSM6_last_g[0] = LSM6_g[0];
	LSM6_last_g[1] = LSM6_g[1];
	LSM6_last_g[2] = LSM6_g[2];
	
	buffer[0]=OUTX_L_G;

	if(!i2c1_writeNBytes(address, buffer, 1))
			return false;

	if(!i2c1_readNBytes(address, buffer, 6))
			return false;

	LSM6_g[0] = ((int16_t)buffer[1] << 8) | (int16_t)buffer[0];
	LSM6_g[1] = ((int16_t)buffer[3] << 8) | (int16_t)buffer[2];
	LSM6_g[2] = ((int16_t)buffer[5] << 8) | (int16_t)buffer[4];

	LSM6_dg[0] = LSM6_g[0] - LSM6_last_g[0];
	LSM6_dg[1] = LSM6_g[1] - LSM6_last_g[1];
	LSM6_dg[2] = LSM6_g[2] - LSM6_last_g[2];
	
	
	
	return true;

#if 0
  bus->beginTransmission(address);
  // automatic increment of register address is enabled by default (IF_INC in CTRL3_C)
  bus->write(OUTX_L_G);
  bus->endTransmission();

  bus->requestFrom(address, (uint8_t)6);
  uint8_t xlg = bus->read();
  uint8_t xhg = bus->read();
  uint8_t ylg = bus->read();
  uint8_t yhg = bus->read();
  uint8_t zlg = bus->read();
  uint8_t zhg = bus->read();

  // combine high and low bytes
  g.x = (int16_t)(xhg << 8 | xlg);
  g.y = (int16_t)(yhg << 8 | ylg);
  g.z = (int16_t)(zhg << 8 | zlg);
 #endif

}

// Reads all 6 channels of the LSM6 and stores them in the object variables
bool LSM6_read(void)
{
	bool rc1,rc2;

	rc1=LSM6_readAcc();
  	rc2=LSM6_readGyro();

  	return(rc1==true && rc2==true);
}
