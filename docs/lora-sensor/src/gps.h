
#ifndef GPS_H
#define GPS_H

#include "asf.h"

void GPS_init(void);

void GPS_poll(void);

extern uint8_t gps_state;
extern int32_t gps_lat;
extern int32_t gps_lon;

#endif

