#pragma once
#include "Arduino.h"
#include "Debugger.h"

class ModbusBase {
public:
  ModbusBase(Stream* commStream, IDebugger* debugger);
  virtual ~ModbusBase() = default;
  bool isCommValid() const;
  bool isDebugValid() const;

protected:
  Stream* comm;
  IDebugger* debug;
};

class ModbusUtils {
public:
  static uint16_t calculateCRC16(byte* data, int length);
  static bool hexStringToBytes(String hexString, byte* outputBuffer, int bufferSize, int* outputLength);
};