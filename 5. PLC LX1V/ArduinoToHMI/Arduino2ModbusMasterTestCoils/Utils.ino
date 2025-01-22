
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