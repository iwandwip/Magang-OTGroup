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

int16_t readDOValue(uint32_t voltageReadMv, uint8_t tempCelcius) {
  tempCelcius = constrain(tempCelcius, 0, 40);
#if TWO_POINT_CALIBRATION == 0
  uint16_t vSaturation = (uint32_t)CAL1_VOLTAGE + (uint32_t)35 * tempCelcius - (uint32_t)CAL1_TEMPERATURE * 35;
  return (voltageReadMv * DO_SENSOR_TABLE[tempCelcius] / vSaturation);
#else
  uint16_t vSaturation = (int16_t)((int8_t)tempCelcius - CAL2_TEMPERATURE) * ((uint16_t)CAL1_VOLTAGE - CAL2_VOLTAGE) / ((uint8_t)CAL1_TEMPERATURE - CAL2_TEMPERATURE) + CAL2_VOLTAGE;
  return (voltageReadMv * DO_SENSOR_TABLE[tempCelcius] / vSaturation);
#endif
}

void setup() {
  Serial.begin(9600);
  pinMode(DO_SENSOR_PIN, INPUT);
}

void loop() {
  uint8_t temperature = (uint8_t)DO_READ_TEMPERATURE;
  uint16_t adcRawRead = analogRead(DO_SENSOR_PIN);
  uint16_t adcVoltage = uint32_t(DO_SENSOR_VREF_MV) * adcRawRead / DO_SENSOR_ADC_RES;

  Serial.print("temperature:\t" + String(temperature) + "\t");
  Serial.print("ADC RAW:\t" + String(adcRawRead) + "\t");
  Serial.print("ADC Voltage:\t" + String(adcVoltage) + "\t");
  Serial.println("DO:\t" + String(readDOValue(adcVoltage, temperature) / 1000.f) + "\t");

  delay(100);
}