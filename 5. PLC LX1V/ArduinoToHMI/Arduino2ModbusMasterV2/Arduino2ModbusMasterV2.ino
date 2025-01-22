#define ENABLE_MODULE_EEPROM_LIB

#include "Kinematrix.h"

#include "ModbusMaster.h"
#include "SoftwareSerial.h"
#include "ModbusConfig.h"
#include "DOSensor.h"

SoftwareSerial modbus(MODBUS_RO_PIN, MODBUS_DI_PIN);
ModbusMaster node;
EEPROMLib eeprom;

uint8_t readCoil[READ_COIL_LEN];
float writeHoldingRegister[WRITE_HOLDING_REGISTER_LEN];
float readHoldingRegister[READ_HOLDING_REGISTER_LEN];

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  initModbusConfig();
  initSensorDO();
}

void loop() {
  const uint32_t timerHoldingRegisterValue = 1000;
  static uint32_t timerHoldingRegister;
  if (millis() - timerHoldingRegister >= timerHoldingRegisterValue) {
    readSensorDO(
      &writeHoldingRegister[ADC_REGISTER],
      &writeHoldingRegister[VOLT_REGISTER],
      &writeHoldingRegister[DO_REGISTER]);
    timerHoldingRegister = millis();
  }

  readCoil[AUTO_COIL] = readSingleCoil(2);
  readHoldingRegister[CALIBRATION_REGISTER] = readSingleRegisterAsFloat(101);
  readHoldingRegister[IN_FREQUENCY_REGISTER] = readSingleRegisterAsFloat(103);

  if (readCoil[AUTO_COIL]) {
    /*
    TODO: Implement Fuzzy Logic Control for Frequency Adjustment  
    1. Design a fuzzy logic algorithm to control the output frequency.  
    2. Use real-time Dissolved Oxygen (DO) readings as the input for the control system.  
    3. Dynamically adjust the frequency output to maintain optimal oxygen levels in the system.  
    */
    writeHoldingRegister[OUT_FREQUENCY_REGISTER] = 0.0;
  } else {
    writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[IN_FREQUENCY_REGISTER];
  }
  writeMultipleFloatsToRegisters(writeHoldingRegister, 1, WRITE_HOLDING_REGISTER_LEN);

  // serialDebugging();
  digitalWrite(LED_BUILTIN, readCoil[AUTO_COIL]);
}