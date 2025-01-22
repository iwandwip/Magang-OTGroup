#pragma once
#include "Arduino.h"

#define DO_SENSOR_PIN A0
#define DO_SENSOR_VREF_MV 5000
#define DO_SENSOR_ADC_RES 1024
#define TWO_POINT_CALIBRATION 0
#define DO_READ_TEMPERATURE (25)

// Single point calibration needs to be filled CAL1_VOLTAGE and CAL1_TEMPERATURE
#define CAL1_VOLTAGE (1235)    // mv
#define CAL1_TEMPERATURE (25)  // ℃
// Two-point calibration needs to be filled CAL2_VOLTAGE and CAL2_TEMPERATURE
// CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_VOLTAGE (1300)    // mv
#define CAL2_TEMPERATURE (15)  // ℃

const uint16_t DO_SENSOR_TABLE[41] = {
  14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
  11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
  9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
  7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};

void initSensorDO();
void readSensorDO(float* _adcRawRead, float* _adcVoltage, float* _doSensorValue);
int16_t convertDOToPercentage(float doValue, float maxDO);