#pragma once
#include "Arduino.h"
#include "ModbusBase.h"

class ModbusDecoder : public ModbusBase {
public:
  ModbusDecoder(Stream* commStream, IDebugger* debugger);
  void begin();
  void receiveData();

private:
  static const int FRAME_SIZE = 20;
  byte receivedData[FRAME_SIZE];
  int index;
  uint32_t startTime;
  uint32_t lastEndTime;
  bool isWaiting;
  int messageCount;
  bool isNewSequence;
  byte lastFunction;

  void decodeModbusData(byte* data);
  bool isContainsSuffix(byte* data);
};