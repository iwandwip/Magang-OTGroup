#define ENABLE_MODULE_EEPROM_LIB_ESP8266

#include "Kinematrix.h"

#include "ModbusMaster.h"
#include "SoftwareSerial.h"
#include "ModbusConfig.h"
#include "DOSensor.h"
#include "SensorFilter.h"

#define PWM_OUTPUT_PIN D4

SoftwareSerial modbus(MODBUS_RO_PIN, MODBUS_DI_PIN);
ModbusMaster node;
EEPROMLibESP8266 eeprom;

uint8_t readCoil[READ_COIL_LEN];
float writeHoldingRegister[WRITE_HOLDING_REGISTER_LEN];
float readHoldingRegister[READ_HOLDING_REGISTER_LEN];
uint16_t pwmOutput;

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PWM_OUTPUT_PIN, OUTPUT);
  analogWriteFreq(980);
  analogWriteRange(255);

  initModbusConfig();
  initSensorDO();

  eeprom.init();
  // EEPROMHealthReport report = eeprom.totalHealthCheck(eeprom.getWriteCount());

  writeHoldingRegister[EEPROM_WRITE_COUNT_REGISTER] = eeprom.getWriteCount();
  writeHoldingRegister[EEPROM_WRITE_LIMIT_REGISTER] = 100000;
  // writeHoldingRegister[EEPROM_HEALTH_REGISTER] = report.healthPercentage;
  // writeHoldingRegister[EEPROM_TOTAL_BAD_SECTOR_REGISTER] = report.badSectors;

  readHoldingRegister[CALIBRATION_REGISTER] = eeprom.readFloat(0);
  readHoldingRegister[IN_FREQUENCY_REGISTER] = eeprom.readFloat(4);
  readHoldingRegister[DO_THRESHOLD_REGISTER] = eeprom.readFloat(8);
  readHoldingRegister[ABOVE_THESHOLD_REGISTER] = eeprom.readFloat(12);
  readHoldingRegister[BELOW_THESHOLD_REGISTER] = eeprom.readFloat(16);
  readHoldingRegister[TRANSITION_TIME_REGISTER] = eeprom.readFloat(20);

  delay(1000);

  writeMultipleFloatsToRegisters(readHoldingRegister, 101, READ_HOLDING_REGISTER_LEN);
}

void loop() {
  const uint32_t timerHoldingRegisterValue = 30;
  static uint32_t timerHoldingRegister = 0;
  static uint32_t eepromInitializeTimer = 0;
  static bool isSystemInitialize = false;

  if (millis() - timerHoldingRegister >= timerHoldingRegisterValue) {
    float rawADCSensor, rawVoltSensor, rawDOSensor;
    readSensorDO(
      &rawADCSensor,
      &rawVoltSensor,
      &rawDOSensor);

    writeHoldingRegister[ADC_REGISTER] = rawADCSensor;
    writeHoldingRegister[VOLT_REGISTER] = rawVoltSensor;
    writeHoldingRegister[DO_REGISTER] = rawDOSensor;
    writeHoldingRegister[DO_REGISTER] += readHoldingRegister[CALIBRATION_REGISTER];
    timerHoldingRegister = millis();
  }

  readCoil[AUTO_COIL] = readSingleCoil(2);

  readMultipleFloatsFromRegisters(readHoldingRegister, 101, READ_HOLDING_REGISTER_LEN);

  bool eepromEnableWrite = false;
  for (int i = 0; i < READ_HOLDING_REGISTER_LEN; i++) {
    if (readHoldingRegister[i] != 0.0) {
      eepromEnableWrite = true;
      break;
    }
  }

  if (millis() - eepromInitializeTimer <= 15000) {
    isSystemInitialize = false;
    readHoldingRegister[CALIBRATION_REGISTER] = eeprom.readFloat(0);
    readHoldingRegister[IN_FREQUENCY_REGISTER] = eeprom.readFloat(4);
    readHoldingRegister[DO_THRESHOLD_REGISTER] = eeprom.readFloat(8);
    readHoldingRegister[ABOVE_THESHOLD_REGISTER] = eeprom.readFloat(12);
    readHoldingRegister[BELOW_THESHOLD_REGISTER] = eeprom.readFloat(16);
    readHoldingRegister[TRANSITION_TIME_REGISTER] = eeprom.readFloat(20);

    writeMultipleFloatsToRegisters(readHoldingRegister, 101, READ_HOLDING_REGISTER_LEN);
  } else {
    if (eepromEnableWrite) {
      isSystemInitialize = true;
      eeprom.writeFloat(0, readHoldingRegister[CALIBRATION_REGISTER]);
      eeprom.writeFloat(4, readHoldingRegister[IN_FREQUENCY_REGISTER]);
      eeprom.writeFloat(8, readHoldingRegister[DO_THRESHOLD_REGISTER]);
      eeprom.writeFloat(12, readHoldingRegister[ABOVE_THESHOLD_REGISTER]);
      eeprom.writeFloat(16, readHoldingRegister[BELOW_THESHOLD_REGISTER]);
      eeprom.writeFloat(20, readHoldingRegister[TRANSITION_TIME_REGISTER]);
    }
  }

  if (isSystemInitialize) {
    static uint32_t systemTransitionTimer;
    if (millis() - systemTransitionTimer >= readHoldingRegister[TRANSITION_TIME_REGISTER] * 1000) {
      systemTransitionTimer = millis();
      if (writeHoldingRegister[DO_REGISTER] > readHoldingRegister[DO_THRESHOLD_REGISTER]) {
        digitalWrite(LED_BUILTIN, HIGH);
        pwmOutput = map(readHoldingRegister[ABOVE_THESHOLD_REGISTER], 0, 50, 0, 255);
        writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[ABOVE_THESHOLD_REGISTER];
      } else {
        digitalWrite(LED_BUILTIN, LOW);
        pwmOutput = map(readHoldingRegister[BELOW_THESHOLD_REGISTER], 0, 50, 0, 255);
        writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[BELOW_THESHOLD_REGISTER];
      }
      writeHoldingRegister[PWM_OUT_REGISTER] = pwmOutput;
      analogWrite(PWM_OUTPUT_PIN, pwmOutput);
    }
  }

  writeHoldingRegister[EEPROM_WRITE_COUNT_REGISTER] = eeprom.getWriteCount();
  writeMultipleFloatsToRegisters(writeHoldingRegister, 1, WRITE_HOLDING_REGISTER_LEN);

  // serialDebugging();
  // digitalWrite(LED_BUILTIN, readCoil[AUTO_COIL]);
}

// void loop() {
//   // NOTES: DO MONITOR & CONTROL V1
//   if (readCoil[AUTO_COIL]) {
//     static uint32_t systemTransitionTimer;
//     if (millis() - systemTransitionTimer >= readHoldingRegister[TRANSITION_TIME_REGISTER] * 1000) {
//       systemTransitionTimer = millis();
//       if (writeHoldingRegister[DO_REGISTER] > readHoldingRegister[DO_THRESHOLD_REGISTER]) {
//         pwmOutput = map(readHoldingRegister[ABOVE_THESHOLD_REGISTER], 0, 50, 0, 255);
//         writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[ABOVE_THESHOLD_REGISTER];
//       } else {
//         pwmOutput = map(readHoldingRegister[BELOW_THESHOLD_REGISTER], 0, 50, 0, 255);
//         writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[BELOW_THESHOLD_REGISTER];
//       }
//       writeHoldingRegister[PWM_OUT_REGISTER] = pwmOutput;
//       analogWrite(PWM_OUTPUT_PIN, pwmOutput);
//     }
//   } else {
//     pwmOutput = map(readHoldingRegister[IN_FREQUENCY_REGISTER], 0, 50, 0, 255);
//     writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[IN_FREQUENCY_REGISTER];
//     writeHoldingRegister[PWM_OUT_REGISTER] = pwmOutput;
//     analogWrite(PWM_OUTPUT_PIN, pwmOutput);
//   }
// }