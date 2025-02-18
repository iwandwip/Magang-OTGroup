#pragma once
#include "Arduino.h"

class DriverBridge {
public:
  DriverBridge(Stream* motorSerial, Stream* debugSerial);
  void begin();
  bool executeMotorCommand(int address);
  bool openLock();

private:
  Stream* motor;
  Stream* debug;

  static const uint8_t MOTOR_COMMAND_LENGTH = 10;
  static const int FRAME_SIZE = 20;
  static const int COMMAND_DELAYS[10];
  static const String DOOR_LOCKS[2];

  byte receivedData[20];
  byte lastReceivedData[20];
  int index;
  uint32_t startTime;
  uint32_t lastEndTime;
  bool isWaiting;
  int messageCount;
  bool isNewSequence;
  byte lastFunction;
  bool lastDataHasSuffix;

  uint16_t calculateCRC16(byte* data, int length);
  bool hexStringToBytes(String hexString, byte* outputBuffer, int bufferSize, int* outputLength);
  String generateModbusFrame(uint8_t address);
  bool writeHexString(const String& hexString);
  bool writeBytes(byte* data, int length);
  void receiveMotorResponse();
  bool isContainsSuffix(byte* data);
};