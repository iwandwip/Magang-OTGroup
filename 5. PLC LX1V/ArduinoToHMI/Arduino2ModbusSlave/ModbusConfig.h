#pragma once
#include "Arduino.h"

#define MODBUS_RO_PIN 10  // RX
#define MODBUS_RE_PIN 11  // EN
#define MODBUS_DE_PIN 11  // EN
#define MODBUS_DI_PIN 12  // TX

enum HoldingRegisterIndex {
  ADC_REG,
  VOLT_REG,
  DO_REG,
  PERCENT_REG,
  CALIBRATION_REG,
  HOLDING_REGISTER_LEN  // Length
};

constexpr uint16_t ADDRESS_OFFSET = 1;
enum HoldingRegisterAddress {
  ADC_ADDRESS = ADC_REG * 2 + ADDRESS_OFFSET,
  VOLT_ADDRESS = VOLT_REG * 2 + ADDRESS_OFFSET,
  DO_ADDRESS = DO_REG * 2 + ADDRESS_OFFSET,
  PERCENT_ADDRESS = PERCENT_REG * 2 + ADDRESS_OFFSET,
  CALIBRATION_ADDRESS = CALIBRATION_REG * 2 + ADDRESS_OFFSET,
  HOLDING_REGISTER_ADDRESS_LEN = HOLDING_REGISTER_LEN * 2 + ADDRESS_OFFSET
};

enum CoinStateDataIndex {
  POWER_BUTTON,   // 0
  PUMP_BUTTON,    // 1
  COIL_STATE_LEN  // Length
};

constexpr uint16_t STATE_OFFSET = 1;
enum CoinStateDataAddress {
  POWER_BUTTON_ADDRESS = POWER_BUTTON + STATE_OFFSET,
  PUMP_BUTTON_ADDRESS = PUMP_BUTTON + STATE_OFFSET,
  COIL_STATE_ADDRESS_LEN = COIL_STATE_LEN + STATE_OFFSET
};

void initModbusConfig();
uint8_t readCoils(uint8_t fc, uint16_t address, uint16_t length, void *context);
uint8_t writeCoils(uint8_t fc, uint16_t address, uint16_t length, void *context);
uint8_t readInputRegister(uint8_t fc, uint16_t address, uint16_t length, void *context);
uint8_t readHoldingRegister(uint8_t fc, uint16_t address, uint16_t length, void *context);
uint8_t writeHoldingRegister(uint8_t fc, uint16_t address, uint16_t length, void *context);