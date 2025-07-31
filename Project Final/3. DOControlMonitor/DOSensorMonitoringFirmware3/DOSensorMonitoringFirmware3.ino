#include "PinsAndConfig.h"
#include "Kinematrix.h"

#include "ModbusMaster.h"
#include "SoftwareSerial.h"
#include "ModbusConfig.h"
#include "DOSensor.h"
#include "SensorFilter.h"

SoftwareSerial modbus(MODBUS_RO_PIN, MODBUS_DI_PIN);
ModbusMaster node;
PID pid;

#if IS_ARDUINO_BOARD
EEPROMLib eeprom;
#else
#if ENABLE_FIREBASE
FirebaseModule firebase;
FirebaseAuthentication auth;
#endif
EEPROMLibESP8266 eeprom;
#endif

uint8_t readCoil[READ_COIL_LEN];
float writeHoldingRegister[WRITE_HOLDING_REGISTER_LEN];
float readHoldingRegister[READ_HOLDING_REGISTER_LEN];
float calibrationFactor = 1.0;
float calibrationInputQC = 0.0;
float calibrationInputQCBefore = 0.0;

void setup() {
  // ESP.wdtDisable();
  // ESP.wdtEnable(0);
  Serial.begin(9600);

  // pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PWM_OUTPUT_PIN, OUTPUT);
#if IS_ARDUINO_BOARD
#else
  analogWriteFreq(980);
  analogWriteRange(255);
#endif
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
  readHoldingRegister[PARAM_KP_PID_REGISTER] = eeprom.readFloat(24);
  readHoldingRegister[PARAM_KI_PID_REGISTER] = eeprom.readFloat(28);
  readHoldingRegister[PARAM_KD_PID_REGISTER] = eeprom.readFloat(32);
  readHoldingRegister[PARAM_SP_PID_REGISTER] = eeprom.readFloat(36);
  readHoldingRegister[PARAM_DO_QC_INPUT_REGISTER] = eeprom.readFloat(40);
  readHoldingRegister[PARAM_DO_CAL_FACTOR_REGISTER] = eeprom.readFloat(44);

  bool isAnyNaN =
    isnan(readHoldingRegister[CALIBRATION_REGISTER])
    || isnan(readHoldingRegister[IN_FREQUENCY_REGISTER])
    || isnan(readHoldingRegister[DO_THRESHOLD_REGISTER])
    || isnan(readHoldingRegister[ABOVE_THESHOLD_REGISTER])
    || isnan(readHoldingRegister[BELOW_THESHOLD_REGISTER])
    || isnan(readHoldingRegister[TRANSITION_TIME_REGISTER])
    || isnan(readHoldingRegister[PARAM_KP_PID_REGISTER])
    || isnan(readHoldingRegister[PARAM_KI_PID_REGISTER])
    || isnan(readHoldingRegister[PARAM_KD_PID_REGISTER])
    || isnan(readHoldingRegister[PARAM_SP_PID_REGISTER])
    || isnan(readHoldingRegister[PARAM_DO_QC_INPUT_REGISTER])
    || isnan(readHoldingRegister[PARAM_DO_CAL_FACTOR_REGISTER]);

  if (isAnyNaN) {
    eeprom.writeFloat(0, 1.0);    // CALIBRATION_REGISTER
    eeprom.writeFloat(4, 0.0);    // IN_FREQUENCY_REGISTER
    eeprom.writeFloat(8, 2.0);    // DO_THRESHOLD_REGISTER
    eeprom.writeFloat(12, 25.0);  // ABOVE_THESHOLD_REGISTER
    eeprom.writeFloat(16, 35.0);  // BELOW_THESHOLD_REGISTER
    eeprom.writeFloat(20, 1.0);   // TRANSITION_TIME_REGISTER
    eeprom.writeFloat(24, 0.8);   // PARAM_KP_PID_REGISTER
    eeprom.writeFloat(28, 0.3);   // PARAM_KI_PID_REGISTER
    eeprom.writeFloat(32, 0.2);   // PARAM_KD_PID_REGISTER
    eeprom.writeFloat(36, 2.5);   // PARAM_SP_PID_REGISTER
    eeprom.writeFloat(40, 2.5);   // PARAM_DO_QC_INPUT_REGISTER
    eeprom.writeFloat(44, 1.0);   // PARAM_DO_CAL_FACTOR_REGISTER
  }

  delay(1000);

  writeMultipleFloatsToRegisters(readHoldingRegister, 101, READ_HOLDING_REGISTER_LEN);
#if !IS_ARDUINO_BOARD && ENABLE_FIREBASE
  wifiTaskInit();
#endif

  pid.setConstants(readHoldingRegister[PARAM_KP_PID_REGISTER], readHoldingRegister[PARAM_KI_PID_REGISTER], readHoldingRegister[PARAM_KD_PID_REGISTER], readHoldingRegister[PARAM_SP_PID_REGISTER]);
  pid.setOutputRange(0, 50);
  pid.reset();

  calibrationDO();
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

    timerHoldingRegister = millis();
  }

  readCoil[AUTO_COIL] = readSingleCoil(2);
  readMultipleFloatsFromRegisters(readHoldingRegister, 101, READ_HOLDING_REGISTER_LEN);
  calibrationInputQC = readHoldingRegister[PARAM_DO_QC_INPUT_REGISTER];
  if (calibrationInputQC != calibrationInputQCBefore) {
    calibrationInputQCBefore = calibrationInputQC;
    calibrationDO();
  }

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
    readHoldingRegister[PARAM_KP_PID_REGISTER] = eeprom.readFloat(24);
    readHoldingRegister[PARAM_KI_PID_REGISTER] = eeprom.readFloat(28);
    readHoldingRegister[PARAM_KD_PID_REGISTER] = eeprom.readFloat(32);
    readHoldingRegister[PARAM_SP_PID_REGISTER] = eeprom.readFloat(36);
    readHoldingRegister[PARAM_DO_QC_INPUT_REGISTER] = eeprom.readFloat(40);
    readHoldingRegister[PARAM_DO_CAL_FACTOR_REGISTER] = eeprom.readFloat(44);

    writeMultipleFloatsToRegisters(readHoldingRegister, 101, READ_HOLDING_REGISTER_LEN);
    Serial.println("| Wait");
  } else {
    if (eepromEnableWrite) {
      isSystemInitialize = true;
      eeprom.writeFloat(0, readHoldingRegister[CALIBRATION_REGISTER]);
      eeprom.writeFloat(4, readHoldingRegister[IN_FREQUENCY_REGISTER]);
      eeprom.writeFloat(8, readHoldingRegister[DO_THRESHOLD_REGISTER]);
      eeprom.writeFloat(12, readHoldingRegister[ABOVE_THESHOLD_REGISTER]);
      eeprom.writeFloat(16, readHoldingRegister[BELOW_THESHOLD_REGISTER]);
      eeprom.writeFloat(20, readHoldingRegister[TRANSITION_TIME_REGISTER]);
      eeprom.writeFloat(24, readHoldingRegister[PARAM_KP_PID_REGISTER]);
      eeprom.writeFloat(28, readHoldingRegister[PARAM_KI_PID_REGISTER]);
      eeprom.writeFloat(32, readHoldingRegister[PARAM_KD_PID_REGISTER]);
      eeprom.writeFloat(36, readHoldingRegister[PARAM_SP_PID_REGISTER]);
      eeprom.writeFloat(40, readHoldingRegister[PARAM_DO_QC_INPUT_REGISTER]);
      eeprom.writeFloat(44, readHoldingRegister[PARAM_DO_CAL_FACTOR_REGISTER]);
      serialDebugging();
    }
  }

  if (isSystemInitialize) {
    static uint32_t systemTransitionTimer;
    if (millis() - systemTransitionTimer >= readHoldingRegister[TRANSITION_TIME_REGISTER] * 1000) {
      systemTransitionTimer = millis();
      pid.calculate(readHoldingRegister[PARAM_SP_PID_REGISTER] * 1000, writeHoldingRegister[DO_REGISTER] * 1000);
      writeHoldingRegister[OUT_FREQUENCY_REGISTER] = pid.getOutput();
      if (writeHoldingRegister[OUT_FREQUENCY_REGISTER] < 20) writeHoldingRegister[OUT_FREQUENCY_REGISTER] = 20;
      else if (writeHoldingRegister[OUT_FREQUENCY_REGISTER] > 40) writeHoldingRegister[OUT_FREQUENCY_REGISTER] = 40;
      writeHoldingRegister[PWM_OUT_REGISTER] = map(writeHoldingRegister[OUT_FREQUENCY_REGISTER], 0, 50, 0, 255);
      analogWrite(PWM_OUTPUT_PIN, writeHoldingRegister[PWM_OUT_REGISTER]);
    }

    // static uint32_t systemTransitionTimer;
    // if (millis() - systemTransitionTimer >= readHoldingRegister[TRANSITION_TIME_REGISTER] * 1000) {
    //   systemTransitionTimer = millis();
    //   if (writeHoldingRegister[DO_REGISTER] > readHoldingRegister[DO_THRESHOLD_REGISTER]) {
    //     pwmOutput = map(readHoldingRegister[ABOVE_THESHOLD_REGISTER], 0, 50, 0, 255);
    //     writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[ABOVE_THESHOLD_REGISTER];
    //   } else {
    //     pwmOutput = map(readHoldingRegister[BELOW_THESHOLD_REGISTER], 0, 50, 0, 255);
    //     writeHoldingRegister[OUT_FREQUENCY_REGISTER] = readHoldingRegister[BELOW_THESHOLD_REGISTER];
    //   }
    //   writeHoldingRegister[PWM_OUT_REGISTER] = pwmOutput;
    //   analogWrite(PWM_OUTPUT_PIN, pwmOutput);
    // }
  }

  writeHoldingRegister[EEPROM_WRITE_COUNT_REGISTER] = eeprom.getWriteCount();
  writeMultipleFloatsToRegisters(writeHoldingRegister, 1, WRITE_HOLDING_REGISTER_LEN);
#if !IS_ARDUINO_BOARD && ENABLE_FIREBASE
  wifiTaskLoop();
#endif
}