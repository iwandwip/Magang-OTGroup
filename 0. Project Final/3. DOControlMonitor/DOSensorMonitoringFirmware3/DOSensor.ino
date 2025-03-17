#include "DOSensor.h"

MovingAverageFilter filterAverageADC(350);  // 70, 350
DynamicLerpFilter filterLerpADC(0.002, 0.2, 0.01);

void initSensorDO() {
  pinMode(DO_SENSOR_PIN, INPUT);
}

unsigned long lowReadingStartTime = 0;
bool isLowReading = false;

void readSensorDO(float* adcRawRead, float* adcVoltage, float* doSensorValue) {
  uint8_t temperature = (uint8_t)DO_READ_TEMPERATURE;
  int rawAdcReading = analogRead(DO_SENSOR_PIN);
  if (rawAdcReading < 15) {
    if (!isLowReading) {
      lowReadingStartTime = millis();
      isLowReading = true;
    }
    if (millis() - lowReadingStartTime > 5000) {
      *adcRawRead = 0;
      *adcVoltage = 0;
      *doSensorValue = 0;
      return;
    }
  } else {
    isLowReading = false;
    filterAverageADC.addMeasurement(rawAdcReading);
  }
  *adcRawRead = filterAverageADC.getFilteredValue();
  filterLerpADC.update(*adcRawRead);
  *adcRawRead = filterLerpADC.getValue();

  *adcVoltage = *adcRawRead * (ARDUINO_REFERENCE_VOLTAGE / ARDUINO_ADC_RESOLUTION);
  *doSensorValue = *adcVoltage * (DO_RANGE_MAX / ARDUINO_REFERENCE_VOLTAGE);
  *doSensorValue = *doSensorValue * calibrationFactor;

  *doSensorValue = *doSensorValue < 0.0 ? 0.0 : *doSensorValue;
  *doSensorValue = *doSensorValue > 20.0 ? 20.0 : *doSensorValue;

  // *adcVoltage = uint32_t(DO_SENSOR_VREF_MV) * *adcRawRead / DO_SENSOR_ADC_RES;
  // *doSensorValue = convertVoltageToDO(*adcVoltage, temperature) / 1000.f;
  // *adcVoltage /= 1000.f;
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

void calibrationDO() {
  writeHoldingRegister[VOLT_REGISTER] = writeHoldingRegister[ADC_REGISTER] * (ARDUINO_REFERENCE_VOLTAGE / ARDUINO_ADC_RESOLUTION);
  writeHoldingRegister[DO_REGISTER] = writeHoldingRegister[VOLT_REGISTER] * (DO_RANGE_MAX / ARDUINO_REFERENCE_VOLTAGE);

  calibrationFactor = readHoldingRegister[PARAM_DO_QC_INPUT_REGISTER] / writeHoldingRegister[DO_REGISTER];
  calibrationFactor = (isnan(calibrationFactor) || isinf(calibrationFactor)) ? 1.0 : calibrationFactor;
}