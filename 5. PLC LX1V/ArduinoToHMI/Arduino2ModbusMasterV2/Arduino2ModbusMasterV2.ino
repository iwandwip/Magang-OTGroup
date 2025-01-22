#define ENABLE_MODULE_EEPROM_LIB

#include "Kinematrix.h"

#include "ModbusMaster.h"
#include "SoftwareSerial.h"
#include "ModbusConfig.h"
#include "DOSensor.h"

#define PWM_OUTPUT_PIN 5

SoftwareSerial modbus(MODBUS_RO_PIN, MODBUS_DI_PIN);
ModbusMaster node;
EEPROMLib eeprom;

uint8_t readCoil[READ_COIL_LEN];
float writeHoldingRegister[WRITE_HOLDING_REGISTER_LEN];
float readHoldingRegister[READ_HOLDING_REGISTER_LEN];
uint16_t pwmOutput;

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PWM_OUTPUT_PIN, OUTPUT);
  initModbusConfig();
  initSensorDO();

  eeprom.init();
  EEPROMHealthReport report = eeprom.totalHealthCheck(eeprom.getWriteCount());

  writeHoldingRegister[EEPROM_WRITE_COUNT_REGISTER] = eeprom.getWriteCount();
  writeHoldingRegister[EEPROM_WRITE_LIMIT_REGISTER] = 100000;
  writeHoldingRegister[EEPROM_HEALTH_REGISTER] = report.healthPercentage;
  writeHoldingRegister[EEPROM_TOTAL_BAD_SECTOR_REGISTER] = report.badSectors;

  readHoldingRegister[CALIBRATION_REGISTER] = eeprom.readFloat(0);
  readHoldingRegister[IN_FREQUENCY_REGISTER] = eeprom.readFloat(4);
  readHoldingRegister[DO_THRESHOLD_REGISTER] = eeprom.readFloat(8);
  readHoldingRegister[ABOVE_THESHOLD_REGISTER] = eeprom.readFloat(12);
  readHoldingRegister[BELLOW_THESHOLD_REGISTER] = eeprom.readFloat(16);
  readHoldingRegister[TRANSITION_TIME_REGISTER] = eeprom.readFloat(20);

  writeMultipleFloatsToRegisters(readHoldingRegister, 101, READ_HOLDING_REGISTER_LEN);
}

void loop() {
  const uint32_t timerHoldingRegisterValue = 1000;
  static uint32_t timerHoldingRegister;
  if (millis() - timerHoldingRegister >= timerHoldingRegisterValue) {
    readSensorDO(
      &writeHoldingRegister[ADC_REGISTER],
      &writeHoldingRegister[VOLT_REGISTER],
      &writeHoldingRegister[DO_REGISTER]);
    writeHoldingRegister[DO_REGISTER] += readHoldingRegister[CALIBRATION_REGISTER];
    timerHoldingRegister = millis();
  }

  readCoil[AUTO_COIL] = readSingleCoil(2);

  readMultipleFloatsFromRegisters(readHoldingRegister, 101, READ_HOLDING_REGISTER_LEN);

  eeprom.writeFloat(0, readHoldingRegister[CALIBRATION_REGISTER]);
  eeprom.writeFloat(4, readHoldingRegister[IN_FREQUENCY_REGISTER]);
  eeprom.writeFloat(8, readHoldingRegister[DO_THRESHOLD_REGISTER]);
  eeprom.writeFloat(12, readHoldingRegister[ABOVE_THESHOLD_REGISTER]);
  eeprom.writeFloat(16, readHoldingRegister[BELLOW_THESHOLD_REGISTER]);
  eeprom.writeFloat(20, readHoldingRegister[TRANSITION_TIME_REGISTER]);

  if (readCoil[AUTO_COIL]) {
    // TODO
    pwmOutput = 0;
    writeHoldingRegister[OUT_FREQUENCY_REGISTER] = 0.0;
    writeHoldingRegister[PWM_OUT_REGISTER] = pwmOutput;
  } else {
    pwmOutput = map(readHoldingRegister[IN_FREQUENCY_REGISTER], 0, 50, 0, 255);
    writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[IN_FREQUENCY_REGISTER];
    writeHoldingRegister[PWM_OUT_REGISTER] = pwmOutput;
  }
  analogWrite(PWM_OUTPUT_PIN, pwmOutput);

  writeHoldingRegister[EEPROM_WRITE_COUNT_REGISTER] = eeprom.getWriteCount();
  writeMultipleFloatsToRegisters(writeHoldingRegister, 1, WRITE_HOLDING_REGISTER_LEN);

  // serialDebugging();
  digitalWrite(LED_BUILTIN, readCoil[AUTO_COIL]);
}