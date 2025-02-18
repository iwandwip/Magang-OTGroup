#include "ModbusBase.h"

ModbusBase::ModbusBase(Stream* commStream, IDebugger* debugger)
  : comm(commStream), debug(debugger) {}

bool ModbusBase::isCommValid() const {
  return comm != nullptr;
}

bool ModbusBase::isDebugValid() const {
  return debug != nullptr;
}

uint16_t ModbusUtils::calculateCRC16(byte* data, int length) {
  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < length; pos++) {
    crc ^= (uint16_t)data[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool ModbusUtils::hexStringToBytes(String hexString, byte* outputBuffer, int bufferSize, int* outputLength) {
  if (hexString.length() % 2 != 0) {
    *outputLength = 0;
    return false;
  }
  *outputLength = hexString.length() / 2;
  if (*outputLength > bufferSize) {
    *outputLength = 0;
    return false;
  }
  for (int i = 0; i < hexString.length(); i += 2) {
    String byteStr = hexString.substring(i, i + 2);
    outputBuffer[i / 2] = (byte)strtol(byteStr.c_str(), NULL, 16);
  }
  return true;
}