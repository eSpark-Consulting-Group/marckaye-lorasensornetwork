
#ifndef I2C2_H
#define I2C2_H

#include "asf.h"

bool i2c2_init(void);

bool i2c2_writeNBytes(U8 addr, U8 *buf, U8 len);

bool i2c2_readNBytes(U8 addr, U8 *buf, U8 len);

// ADDRESSES

#define SGP40_I2C_ADDRESS 0x59



#endif
