#define ENABLE_MODULE_EEPROM_LIB

#include "Kinematrix.h"

#include "ModbusSlave.h"
#include "SoftwareSerial.h"
#include "ModbusConfig.h"
#include "DOSensor.h"

SoftwareSerial modbus(MODBUS_RO_PIN, MODBUS_DI_PIN);
Modbus slave(modbus, 1, MODBUS_RE_PIN);
EEPROMLib eeprom;

uint8_t coilState[COIL_STATE_LEN];
float holdingRegister[HOLDING_REGISTER_LEN];

uint16_t temperature;
uint16_t adcRawRead;
uint16_t adcVoltage;
uint16_t doPercentage;
float doSensorValue;
float doSensorCalibration;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  initModbusConfig();
  initSensorDO();
}

void loop() {
  readSensorDO(&adcRawRead, &adcVoltage, &doSensorValue);

  static uint32_t timerHoldingRegister;
  if (millis() - timerHoldingRegister >= 1000) {
    timerHoldingRegister = millis();

    holdingRegister[ADC_REG] = adcRawRead;
    holdingRegister[VOLT_REG] = adcVoltage / 1000.f;
    holdingRegister[DO_REG] = doSensorValue + doSensorCalibration;
    holdingRegister[PERCENT_REG] = convertDOToPercentage(doSensorValue, 20.0);
    doSensorCalibration = holdingRegister[CALIBRATION_REG];
  }

  digitalWrite(LED_BUILTIN, coilState[POWER_BUTTON]);
  slave.poll();
}