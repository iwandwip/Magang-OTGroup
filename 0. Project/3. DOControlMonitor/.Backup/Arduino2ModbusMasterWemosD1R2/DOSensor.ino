#include "DOSensor.h"

void initSensorDO() {
  pinMode(DO_SENSOR_PIN, INPUT);
}

void readSensorDO(float* _adcRawRead, float* _adcVoltage, float* _doSensorValue) {
  uint8_t temperature = (uint8_t)DO_READ_TEMPERATURE;
  *_adcRawRead = analogRead(DO_SENSOR_PIN);
  *_adcVoltage = uint32_t(DO_SENSOR_VREF_MV) * *_adcRawRead / DO_SENSOR_ADC_RES;
  *_doSensorValue = convertVoltageToDO(*_adcVoltage, temperature) / 1000.f;
  *_adcVoltage /= 1000.f;
}

int16_t convertDOToPercentage(float doValue, float maxDO) {
  return constrain((doValue / maxDO) * 100.0, 0.f, 100.f);
}

int16_t convertVoltageToDO(uint32_t voltageReadMv, uint8_t tempCelcius) {
  tempCelcius = constrain(tempCelcius, 0, 40);
#if TWO_POINT_CALIBRATION == 0
  uint16_t vSaturation = (uint32_t)CAL1_VOLTAGE + (uint32_t)35 * tempCelcius - (uint32_t)CAL1_TEMPERATURE * 35;
  return (voltageReadMv * DO_SENSOR_TABLE[tempCelcius] / vSaturation);
#else
  uint16_t vSaturation = (int16_t)((int8_t)tempCelcius - CAL2_TEMPERATURE) * ((uint16_t)CAL1_VOLTAGE - CAL2_VOLTAGE) / ((uint8_t)CAL1_TEMPERATURE - CAL2_TEMPERATURE) + CAL2_VOLTAGE;
  return (voltageReadMv * DO_SENSOR_TABLE[tempCelcius] / vSaturation);
#endif
}