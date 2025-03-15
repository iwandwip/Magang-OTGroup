#pragma once
#include "Arduino.h"

enum WriteHoldingRegisterIndex {
  ADC_REGISTER,                      // 41
  VOLT_REGISTER,                     // 43
  DO_REGISTER,                       // 45
  OUT_FREQUENCY_REGISTER,            // 47
  PWM_OUT_REGISTER,                  // 49
  EEPROM_WRITE_COUNT_REGISTER,       // 411
  EEPROM_WRITE_LIMIT_REGISTER,       // 413
  EEPROM_HEALTH_REGISTER,            // 415
  EEPROM_TOTAL_BAD_SECTOR_REGISTER,  // 417
  WRITE_HOLDING_REGISTER_LEN
};

enum ReadHoldingRegisterIndex {
  CALIBRATION_REGISTER,      // 4101
  IN_FREQUENCY_REGISTER,     // 4103
  DO_THRESHOLD_REGISTER,     // 4105
  ABOVE_THESHOLD_REGISTER,   // 4107
  BELOW_THESHOLD_REGISTER,   // 4109
  TRANSITION_TIME_REGISTER,  // 4111
  PARAM_KP_PID_REGISTER,     // 4113
  PARAM_KI_PID_REGISTER,     // 4115
  PARAM_KD_PID_REGISTER,     // 4117
  PARAM_SP_PID_REGISTER,     // 4119
  PARAM_DO_QC_INPUT,         // 4121
  READ_HOLDING_REGISTER_LEN
};

enum ReadCoils {
  POWER_COIL,  // 0
  AUTO_COIL,   // 1
  MODE_COIL,   // 2
  READ_COIL_LEN
};

void initModbusConfig();

float convertRegistersToFloat(uint16_t registerLow, uint16_t registerHigh);
float readSingleRegisterAsFloat(uint16_t address);

bool writeSingleRegisterAsFloat(uint16_t address, float value);
bool readMultipleFloatsFromRegisters(float* values, uint16_t startAddress, uint8_t count);
bool writeMultipleFloatsToRegisters(float* values, uint16_t startAddress, uint8_t count);
bool readSingleCoil(uint16_t address);
bool writeSingleCoil(uint16_t address, bool value);
bool readMultipleCoils(uint16_t startAddress, bool* values, uint16_t count);
bool writeMultipleCoils(uint16_t startAddress, bool* values, uint16_t count);