#pragma once
#include "Arduino.h"
#include "ModbusBase.h"

class VendingControllerBridge : public ModbusBase {
public:
  VendingControllerBridge(Stream* commStream, IDebugger* debugger);
  bool writeBytes(byte* data, int length);
  bool writeHexString(String hexString);
  String generateModbusFrame(uint8_t address);
};