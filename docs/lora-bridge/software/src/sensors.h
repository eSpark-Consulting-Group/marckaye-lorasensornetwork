
#ifndef SENSORS_H
#define SENSORS_H

#include "asf.h"

#define PIN_5V_CNTL PIN_PA22
#define PIN_MUXSEL PIN_PA18

bool SENSORS_init(void);

void SENSORS_5V_CTRL(bool state);

void SENSORS_MUXSEL(bool state);

#endif
