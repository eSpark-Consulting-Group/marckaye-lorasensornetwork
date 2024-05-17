
#ifndef SENSORS_H
#define SENSORS_H

#include "asf.h"
#include "ADS1115.h"

#define PIN_3V3_SENS PIN_PA22
#define PIN_5V_CNTL PIN_PA28
#define PIN_BUTTON PIN_PA31
#define PIN_MUXSEL PIN_PA30
#define PIN_ACC_INT PIN_PB03
#define PIN_SERVO PIN_PA18
#define PIN_GPS_RESET PIN_PA23

bool SENSORS_init(void);

// SGP41 interface
int sgp41_getSerial(uint8_t *sno);
int sgp41_executeConditioning(void);
int sgp41_measureRawSignals(int32_t *srawVoc, int32_t *srawNox);
int sgp41_turnHeaterOff(void);
bool SENSORS_getSGP41(int32_t *Voc, int32_t *Nox);

// SGP40 interface
int sgp40_getSerial(uint8_t *sno);
int sgp40_executeConditioning(void);
int sgp40_measureRawSignal(int32_t *srawVoc);
int sgp40_turnHeaterOff(void);
bool SENSORS_getSGP40(int32_t *Voc);

// SHT45 interface
bool SENSORS_getSHT45(uint16_t *temp, uint16_t *humidity, bool heater);

// LMP91000 interface
bool SENSORS_setLMPValue(void);

bool ADC_init(void);

/// ADC channel assignments
#define ADC_CHAN_302 0
#define ADC_CHAN_402 1
#define ADC_CHAN_502 2
#define ADC_CHAN_602 3
#define ADC_CHAN_702 4
#define ADC_CHAN_802 5
#define ADC_CHAN_O2  6
#define ADC_CHAN_NDIR 7

void ADC_StartConversion(uint8_t channel);
bool ADC_WaitConversion(void);
reg_val_t ADC_ReadConversion(void);

reg_val_t ADC_GetConversion(uint8_t channel);

void SENSORS_3V3_SENS(bool state);
void SENSORS_5V_CTRL(bool state);
bool SENSORS_getButton(void);
void SENSORS_MUXSEL(bool state);
void SENSORS_SERVO(bool state);
bool SENSORS_getAccInt(void);

uint16_t SENSORS_getLight(void);

#endif
