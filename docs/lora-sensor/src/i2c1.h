
#ifndef I2C1_H
#define I2C1_H

#include "asf.h"

bool i2c1_init(void);

bool i2c1_writeNBytes(U8 addr, U8 *buf, U8 len);

bool i2c1_readNBytes(U8 addr, U8 *buf, U8 len);

// ADDRESSES

#define SGP41_I2C_ADDRESS 0x59

#define SSD1306_SA 0x3C


#endif
