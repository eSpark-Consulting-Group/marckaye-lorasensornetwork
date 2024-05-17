
#ifndef CRC_H
#define CRC_H

#include "asf.h"

void	crc_init(uint16_t *crc);

void	crc_update(uint16_t *crc,uint8_t b);

#endif
