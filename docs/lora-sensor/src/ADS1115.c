
#include "ADS1115.h"
#include "i2c1.h"

static uint8_t currentPGA = 255;

static uint8_t i2cAddress;

static flag_val_t ADS1115_readFlag(reg_addr_t dataAddress, flag_pos_t flagPos);
static void ADS1115_writeFlag(reg_addr_t dataAddress, flag_pos_t flagPos, flag_val_t flagValue);

static flag_val_t ADS1115_getFlag(reg_val_t registerValue, flag_pos_t flagPos);
static flag_val_t ADS1115_getFlag2(reg_val_t registerValue, flag_pos_t flagPos, flag_val_t customVal);
static reg_val_t ADS1115_setFlag(reg_val_t registerValue, flag_pos_t flagPos, flag_val_t flagVal);

void ADS1115(i2c_addr_t i2cAddr)
{
	i2cAddress = i2cAddr;

}

/**
 * Public methods
 */

bool ADS1115_reset(void)
{
	if(!ADS1115_writeRegister(ADS1115_CONFIG_REG_ADDR, ADS1115_CONFIG_REG_DEF))
		return false;
		
	if(!ADS1115_writeRegister(ADS1115_LOW_TRESH_REG_ADDR, ADS1115_LOW_TRESH_REG_DEF))
		return false;
		
	if(!ADS1115_writeRegister(ADS1115_HIGH_TRESH_REG_ADDR, ADS1115_HIGH_TRESH_REG_DEF))
		return false;
		
	return true;
}

reg_val_t ADS1115_readRegister(reg_addr_t dataAddress)
{
	uint8_t buffer[2];
	
	buffer[0]=dataAddress;
	
	if(!i2c1_writeNBytes(i2cAddress, buffer, 1))
		return -1;
	
	if(!i2c1_readNBytes(i2cAddress, buffer, 2))
		return -1;
	
	return (buffer[0] << 8)+buffer[1];
	
	#if 0
	Wire.beginTransmission(i2cAddress);
	Wire.write(dataAddress);
	Wire.endTransmission();
	Wire.requestFrom(i2cAddress, (uint8_t) 2);

	if (Wire.available()) {
		uint8_t firstByte = Wire.read();
		uint8_t secondByte = Wire.read();

		return (firstByte << 8) + secondByte;
	}

	return -1;
	#endif
}

bool ADS1115_writeRegister(reg_addr_t dataAddress, uint16_t data) 
{
	uint8_t buffer[3];
	
	buffer[0]=dataAddress;
	buffer[1]=(uint8_t)(data >> 8);
	buffer[2]=(uint8_t)data;
	
	if(!i2c1_writeNBytes(i2cAddress, buffer, 3))
		return false;
	else
		return true;
		
	#if 0
	uint8_t hByte = data >> 8;
	uint8_t lByte = data & 255;

	Wire.beginTransmission(i2cAddress);
	Wire.write(dataAddress);
	Wire.write(hByte);
	Wire.write(lByte);
	Wire.endTransmission();
	#endif
}

reg_val_t ADS1115_readRawValue(void) {
	return ADS1115_readRegister(ADS1115_CONVERSION_REG_ADDR);
}

float ADS1115_readConvertedValue(void) {
	float rawValue = (float) ADS1115_readRawValue();

	if (currentPGA == 255) {
		currentPGA = ADS1115_getPga();
	}

	float multiplier;

	switch (currentPGA) {
		case ADS1115_PGA_0_256:
			multiplier = ADS1115_PGA_0_256_MULT;
			break;
		case ADS1115_PGA_0_512:
			multiplier = ADS1115_PGA_0_512_MULT;
			break;
		case ADS1115_PGA_1_024:
			multiplier = ADS1115_PGA_1_024_MULT;
			break;
		case ADS1115_PGA_2_048:
			multiplier = ADS1115_PGA_2_048_MULT;
			break;
		case ADS1115_PGA_4_096:
			multiplier = ADS1115_PGA_4_096_MULT;
			break;
		case ADS1115_PGA_6_144:
			multiplier = ADS1115_PGA_6_144_MULT;
			break;
		default:
			multiplier = 1.0;
			break;
	}

	return multiplier * rawValue;
}

void ADS1115_startSingleConvertion(void) {
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_OS_FLAG_POS, 1);
}

uint8_t ADS1115_getOperationalStatus(void) {
	return ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_OS_FLAG_POS);
}

void ADS1115_setMultiplexer(uint8_t mux) {
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_MUX2_DAT_POS, ADS1115_getFlag(mux, 2));
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_MUX1_DAT_POS, ADS1115_getFlag(mux, 1));
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_MUX0_DAT_POS, ADS1115_getFlag(mux, 0));
}

uint8_t ADS1115_getMultiplexer(void) {
	uint8_t mux = ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_MUX0_DAT_POS);

	mux |= ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_MUX1_DAT_POS) << 1;
	mux |= ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_MUX2_DAT_POS) << 2;

	return mux;
}

void ADS1115_setPga(uint8_t pga) {
	currentPGA = pga;

	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_PGA2_DAT_POS, ADS1115_getFlag(pga, 2));
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_PGA1_DAT_POS, ADS1115_getFlag(pga, 1));
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_PGA0_DAT_POS, ADS1115_getFlag(pga, 0));
}

uint8_t ADS1115_getPga(void) {
	uint8_t pga = ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_PGA0_DAT_POS);

	pga |= ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_PGA1_DAT_POS) << 1;
	pga |= ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_PGA2_DAT_POS) << 2;

	return pga;
}

void ADS1115_setDeviceMode(uint8_t mode) {
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_MODE_FLAG_POS, mode);
}

uint8_t ADS1115_getDeviceMode(void) {
	return ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_MODE_FLAG_POS);
}

void ADS1115_setDataRate(uint8_t dataRate) {
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_DR2_DAT_POS, ADS1115_getFlag(dataRate, 2));
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_DR1_DAT_POS, ADS1115_getFlag(dataRate, 1));
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_DR0_DAT_POS, ADS1115_getFlag(dataRate, 0));
}

uint8_t ADS1115_getDataRate(void) {
	uint8_t dataRate = ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_DR0_DAT_POS);

	dataRate |= ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_DR1_DAT_POS) << 1;
	dataRate |= ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_DR2_DAT_POS) << 2;

	return dataRate;
}

void ADS1115_setComparatorMode(uint8_t mode) {
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_MODE_FLAG_POS, mode);
}

uint8_t ADS1115_getComparatorMode(void) {
	return ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_MODE_FLAG_POS);
}

void ADS1115_setComparatorPolarity(uint8_t polarity) {
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_POL_FLAG_POS, polarity);
}

uint8_t ADS1115_getComparatorPolarity(void) {
	return ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_POL_FLAG_POS);
}

void ADS1115_setLatching(bool latching) {
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_LAT_FLAG_POS, latching);
}

bool ADS1115_isLatching(void) {
	return ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_LAT_FLAG_POS);
}

void ADS1115_setComparatorQueue(uint8_t queue) {
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_QUE1_DAT_POS, ADS1115_getFlag(queue, 1));
	ADS1115_writeFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_QUE0_DAT_POS, ADS1115_getFlag(queue, 0));
}

uint8_t ADS1115_getComparatorQueue(void) {
	uint8_t queue = ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_QUE0_DAT_POS);

	queue |= ADS1115_readFlag(ADS1115_CONFIG_REG_ADDR, ADS1115_COMP_QUE1_DAT_POS) << 1;

	return queue;
}

void ADS1115_setLowTreshold(reg_val_t lowTreshold) {
	ADS1115_writeRegister(ADS1115_LOW_TRESH_REG_ADDR, lowTreshold);
}

reg_val_t ADS1115_getLowTreshold(void) {
	return ADS1115_readRegister(ADS1115_LOW_TRESH_REG_ADDR);
}

void ADS1115_setHighTreshold(reg_val_t highTreshold) {
	ADS1115_writeRegister(ADS1115_HIGH_TRESH_REG_ADDR, highTreshold);
}

reg_val_t ADS1115_getHighTreshold(void) {
	return ADS1115_readRegister(ADS1115_HIGH_TRESH_REG_ADDR);
}

/**
 * Private methods
 */

static flag_val_t ADS1115_readFlag(reg_addr_t dataAddress, flag_pos_t flagPos) {
	reg_val_t registerValue = ADS1115_readRegister(dataAddress);

	return ADS1115_getFlag(registerValue, flagPos);
}

static void ADS1115_writeFlag(reg_addr_t dataAddress, flag_pos_t flagPos, flag_val_t flagValue) {
	reg_val_t registerValue = ADS1115_readRegister(dataAddress);
	registerValue = ADS1115_setFlag(registerValue, flagPos, flagValue);
	ADS1115_writeRegister(dataAddress, registerValue);
}

static flag_val_t ADS1115_getFlag(reg_val_t registerValue, flag_pos_t flagPos) {
	return ADS1115_getFlag2(registerValue, flagPos, 1);
}

static flag_val_t ADS1115_getFlag2(reg_val_t registerValue, flag_pos_t flagPos, flag_val_t customVal) {
	return ((registerValue >> flagPos) & 1) == 1 ? customVal : 0;
}

static reg_val_t ADS1115_setFlag(reg_val_t registerValue, flag_pos_t flagPos, flag_val_t flagVal) {
	if (flagVal == 1) {
		return registerValue | (1 << flagPos);
	}

	return registerValue & (~(1 << flagPos));
}