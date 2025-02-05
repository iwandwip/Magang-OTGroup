#include "ModbusConfig.h"

void initModbusConfig() {
  modbus.begin(9600);
  node.begin(1, modbus);

  pinMode(MODBUS_RE_PIN, OUTPUT);
  pinMode(MODBUS_DE_PIN, OUTPUT);

  node.preTransmission([]() {
    digitalWrite(MODBUS_RE_PIN, HIGH);
    digitalWrite(MODBUS_DE_PIN, HIGH);
  });
  node.postTransmission([]() {
    digitalWrite(MODBUS_RE_PIN, LOW);
    digitalWrite(MODBUS_DE_PIN, LOW);
  });
}

float convertRegistersToFloat(uint16_t registerLow, uint16_t registerHigh) {
  union {
    float floatValue;
    uint16_t word[2];
  } converter;
  converter.word[0] = registerLow;
  converter.word[1] = registerHigh;
  return converter.floatValue;
}

float readSingleRegisterAsFloat(uint16_t address) {
  node.clearResponseBuffer();
  union {
    float floatValue;
    uint16_t word[2];
  } converter;
  uint8_t result = node.readHoldingRegisters(address, 2);
  if (result != node.ku8MBSuccess) return NAN;
  converter.word[0] = node.getResponseBuffer(0);
  converter.word[1] = node.getResponseBuffer(1);
  return converter.floatValue;
}

bool writeSingleRegisterAsFloat(uint16_t address, float value) {
  node.clearTransmitBuffer();
  union {
    float floatValue;
    uint16_t word[2];
  } converter;
  converter.floatValue = value;
  node.setTransmitBuffer(0, converter.word[0]);
  node.setTransmitBuffer(1, converter.word[1]);
  uint8_t result = node.writeMultipleRegisters(address, 2);
  return (result == node.ku8MBSuccess);
}

bool readMultipleFloatsFromRegisters(float* values, uint16_t startAddress, uint8_t count) {
  node.clearResponseBuffer();
  if (count == 0) return false;
  union {
    float floatValue;
    uint16_t word[2];
  } converter;
  uint8_t result = node.readHoldingRegisters(startAddress, count * 2);
  if (result != node.ku8MBSuccess) return false;
  for (uint8_t i = 0; i < count; i++) {
    converter.word[0] = node.getResponseBuffer(i * 2);
    converter.word[1] = node.getResponseBuffer(i * 2 + 1);
    values[i] = converter.floatValue;
  }
  return true;
}

bool writeMultipleFloatsToRegisters(float* values, uint16_t startAddress, uint8_t count) {
  node.clearTransmitBuffer();
  if (count == 0) return false;
  union {
    float floatValue;
    uint16_t word[2];
  } converter;
  uint8_t transmitIndex = 0;
  for (uint8_t i = 0; i < count; i++) {
    converter.floatValue = values[i];
    node.setTransmitBuffer(transmitIndex++, converter.word[0]);
    node.setTransmitBuffer(transmitIndex++, converter.word[1]);
  }
  uint8_t result = node.writeMultipleRegisters(startAddress, count * 2);
  return (result == node.ku8MBSuccess);
}


bool readSingleCoil(uint16_t address) {
  uint8_t result = node.readCoils(address, 1);
  if (result != node.ku8MBSuccess) return false;
  return node.getResponseBuffer(0) & 0x01;
}

bool writeSingleCoil(uint16_t address, bool value) {
  uint8_t result = node.writeSingleCoil(address, value ? 0xFF00 : 0x0000);
  return (result == node.ku8MBSuccess);
}

bool readMultipleCoils(uint16_t startAddress, bool* values, uint16_t count) {
  uint8_t result = node.readCoils(startAddress, count);
  if (result != node.ku8MBSuccess) return false;
  for (uint16_t i = 0; i < count; i++) {
    uint16_t byteIndex = i / 16;
    uint16_t bitIndex = i % 16;
    values[i] = (node.getResponseBuffer(byteIndex) >> bitIndex) & 0x01;
  }
  return true;
}

bool writeMultipleCoils(uint16_t startAddress, bool* values, uint16_t count) {
  uint16_t currentWord = 0;
  uint8_t transmitIndex = 0;
  for (uint16_t i = 0; i < count; i++) {
    if (values[i]) {
      currentWord |= (1 << (i % 16));
    }
    if ((i + 1) % 16 == 0 || i == count - 1) {
      node.setTransmitBuffer(transmitIndex++, currentWord);
      currentWord = 0;
    }
  }
  uint8_t result = node.writeMultipleCoils(startAddress, count);
  return (result == node.ku8MBSuccess);
}