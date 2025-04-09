#pragma once
#include "Arduino.h"

class DriverBridge {
public:
  DriverBridge(Stream* motorSerial, Stream* debugSerial);
  void begin();
  bool executeMotorCommand(int address);
  bool openLock();

  void receiveMotorResponse();
  float readSensorVoltage();
  String generateModbusFrame(uint8_t address);

  uint16_t calculateCRC16(byte* data, int length);
  bool hexStringToBytes(String hexString, byte* outputBuffer, int bufferSize, int* outputLength);
  bool writeHexString(const String& hexString);
  bool writeBytes(byte* data, int length);
  bool isContainsSuffix(byte* data);
private:
  Stream* motor;
  Stream* debug;

  static const uint8_t MOTOR_COMMAND_LENGTH = 10;
  static const int FRAME_SIZE = 20;
  static const int COMMAND_DELAYS[10];
  static const String DOOR_LOCKS[2];
  static const int SENSOR_PIN = A0;
  static const float VOLTAGE_THRESH = 2.0;

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
};