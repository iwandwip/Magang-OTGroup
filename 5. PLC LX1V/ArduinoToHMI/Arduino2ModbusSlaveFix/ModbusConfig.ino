#include "ModbusConfig.h"

void initModbusConfig() {
  slave.cbVector[CB_READ_COILS] = readCoils;
  slave.cbVector[CB_READ_DISCRETE_INPUTS] = readCoils;
  slave.cbVector[CB_WRITE_COILS] = writeCoils;
  slave.cbVector[CB_READ_INPUT_REGISTERS] = readInputRegister;
  slave.cbVector[CB_READ_HOLDING_REGISTERS] = readHoldingRegister;
  slave.cbVector[CB_WRITE_HOLDING_REGISTERS] = writeHoldingRegister;

  holdingRegister[CALIBRATION_REG] = eeprom.readFloat(200);  // 200 - 999
  // coilState[POWER_BUTTON] = eeprom.readInt(0);               // 0 - 199
  // coilState[PUMP_BUTTON] = eeprom.readInt(2);

  modbus.begin(9600);
  slave.begin(9600);
}

uint8_t readCoils(uint8_t fc, uint16_t address, uint16_t length, void *context) {
  for (int i = 0; i < length; i++) {
    slave.writeCoilToBuffer(i, coilState[i]);
  }
  return STATUS_OK;
}

uint8_t writeCoils(uint8_t fc, uint16_t address, uint16_t length, void *context) {
  if (length == 1) {
    if (address == POWER_BUTTON_ADDRESS) {
      coilState[POWER_BUTTON] = slave.readCoilFromBuffer(0);
      // eeprom.writeInt(0, coilState[POWER_BUTTON]);
    } else if (address == PUMP_BUTTON_ADDRESS) {
      coilState[PUMP_BUTTON] = slave.readCoilFromBuffer(0);
      // eeprom.writeInt(2, coilState[PUMP_BUTTON]);
    }
  }
  return STATUS_OK;
}

uint8_t readInputRegister(uint8_t fc, uint16_t address, uint16_t length, void *context) {
  return STATUS_OK;
}

uint8_t readHoldingRegister(uint8_t fc, uint16_t address, uint16_t length, void *context) {
  union FloatToRegister {
    float floatValue;
    uint16_t word[2];
  };

  FloatToRegister converter[HOLDING_REGISTER_LEN];
  for (int i = 0; i < HOLDING_REGISTER_LEN; i++) {
    converter[i].floatValue = holdingRegister[i];
  }

  for (int i = 0; i < length / 2; i++) {
    int offset = i * 2;
    slave.writeRegisterToBuffer(offset, converter[i].word[0]);
    slave.writeRegisterToBuffer(offset + 1, converter[i].word[1]);
  }
  return STATUS_OK;
}

uint8_t writeHoldingRegister(uint8_t fc, uint16_t address, uint16_t length, void *context) {
  auto registerToFloat = [](uint16_t lowWord, uint16_t highWord) -> float {
    union FloatToRegister {
      float floatValue;
      uint16_t word[2];
    };
    FloatToRegister converter;
    converter.word[0] = lowWord;
    converter.word[1] = highWord;

    return converter.floatValue;
  };

  if (address == CALIBRATION_ADDRESS) {
    uint16_t lowWord = slave.readRegisterFromBuffer(0);
    uint16_t highWord = slave.readRegisterFromBuffer(1);
    holdingRegister[CALIBRATION_REG] = registerToFloat(lowWord, highWord);
    eeprom.writeFloat(200, holdingRegister[CALIBRATION_REG]);
  }
  return STATUS_OK;
}