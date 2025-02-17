// DriverBridge.h
#pragma once
#include "Arduino.h"

class DriverBridge {
public:
  DriverBridge(Stream* motorSerial, Stream* debugSerial);
  void begin();
  bool executeMotorCommand(int address);
  void receiveMotorResponse();

private:
  Stream* motor;
  Stream* debug;

  static const uint8_t MOTOR_COMMAND_LENGTH = 10;
  static const int FRAME_SIZE = 20;
  static const int COMMAND_DELAYS[10];

  byte receivedData[20];
  int index;
  uint32_t startTime;
  uint32_t lastEndTime;
  bool isWaiting;
  int messageCount;
  bool isNewSequence;
  byte lastFunction;

  uint16_t calculateCRC16(byte* data, int length);
  bool hexStringToBytes(String hexString, byte* outputBuffer, int bufferSize, int* outputLength);
  String generateModbusFrame(uint8_t address);
  bool writeHexString(const String& hexString);
  bool writeBytes(byte* data, int length);
  void decodeModbusData(byte* data);
  bool isContainsSuffix(byte* data);
};