// DriverBridge.cpp
#include "DriverBridge.h"

const int DriverBridge::COMMAND_DELAYS[10] = { 75, 800, 1000, 900, 900, 900, 900, 900, 900, 900 };

DriverBridge::DriverBridge(Stream* motorSerial, Stream* debugSerial)
  : motor(motorSerial), debug(debugSerial),
    index(0), startTime(0), lastEndTime(0), isWaiting(false),
    messageCount(0), isNewSequence(false), lastFunction(0) {}

void DriverBridge::begin() {
  
}

bool DriverBridge::executeMotorCommand(int address) {
  String motorAddressFrame = generateModbusFrame(address);
  const String MOTOR_COMMAND_DATA = "010300000000000000000000000000000000D0E8";

  for (int i = 0; i < MOTOR_COMMAND_LENGTH; i++) {
    if (i == 0) {
      writeHexString(motorAddressFrame);
    } else {
      writeHexString(MOTOR_COMMAND_DATA);
    }
    delay(COMMAND_DELAYS[i]);
    receiveMotorResponse();
  }
  return true;
}

void DriverBridge::receiveMotorResponse() {
  if (index == 0 && !isWaiting) {
    startTime = millis();
    isWaiting = true;
  }

  if (motor->available()) {
    if (index == 0) {
      debug->print("| recv: ");
    }

    while (motor->available() && index < FRAME_SIZE) {
      receivedData[index] = motor->read();
      index++;
    }

    if (index == FRAME_SIZE) {
      for (int i = 0; i < FRAME_SIZE; i++) {
        if (receivedData[i] < 0x10) debug->print("0");
        debug->print(receivedData[i], HEX);
      }

      uint32_t currentTime = millis();
      uint32_t totalTime = currentTime - startTime;
      uint32_t timeSinceLastFrame = lastEndTime > 0 ? currentTime - lastEndTime : 0;

      decodeModbusData(receivedData);

      index = 0;
      isWaiting = false;
      lastEndTime = currentTime;
    }
  }
}

uint16_t DriverBridge::calculateCRC16(byte* data, int length) {
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

bool DriverBridge::hexStringToBytes(String hexString, byte* outputBuffer, int bufferSize, int* outputLength) {
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

String DriverBridge::generateModbusFrame(uint8_t address) {
  byte data[20];
  String result = "";
  data[0] = 0x01;                 // Device address
  data[1] = 0x05;                 // Function code
  data[2] = address;              // Parameter address
  data[3] = 0x03;                 // Operand 1
  data[4] = 0x01;                 // Operand 2
  for (int i = 5; i < 18; i++) {  // Padding 14 bytes
    data[i] = 0x00;
  }
  uint16_t crc = calculateCRC16(data, 18);
  data[19] = crc >> 8;    // High byte
  data[18] = crc & 0xFF;  // Low byte

  for (int i = 0; i < 20; i++) {
    if (data[i] < 0x10) {
      result += "0";
    }
    result += String(data[i], HEX);
  }
  return result;
}

bool DriverBridge::writeBytes(byte* data, int length) {
  if (data == nullptr || length <= 0 || !motor) return false;

  debug->print("| send: ");
  for (int i = 0; i < length; i++) {
    motor->write(data[i]);
    if (data[i] < 0x10) debug->print("0");
    debug->print(data[i], HEX);
  }
  debug->println("");
  return true;
}

bool DriverBridge::writeHexString(const String& hexString) {
  byte buffer[32];
  int length;
  if (!hexStringToBytes(hexString, buffer, sizeof(buffer), &length)) {
    debug->println("Error converting hex string");
    return false;
  }
  return writeBytes(buffer, length);
}

void DriverBridge::decodeModbusData(byte* data) {
  byte address = data[0];
  byte function = data[1];
  uint16_t crcReceived = (data[19] << 8) | data[18];

  if (function == 0x05) {
    messageCount = 1;
    isNewSequence = true;
    lastFunction = function;
    debug->println("");
    return;
  } else {
    messageCount = (lastFunction == 0x05) ? 1 : messageCount + 1;
    lastFunction = function;
  }

  uint16_t crcCalculated = calculateCRC16(data, 18);
  if (crcReceived != crcCalculated) {
    debug->println("| CRC Error: Data corrupted!");
    return;
  }

  uint16_t reg1 = (data[3] << 8) | data[4];
  uint16_t reg2 = (data[5] << 8) | data[6];
  uint16_t reg3 = (data[7] << 8) | data[8];
  uint16_t reg4 = (data[9] << 8) | data[10];

  if (messageCount >= 3) {
    if (reg2 == 0 && reg3 == 0 && reg4 == 0) {
      debug->print("| Status: Motor Tidak Berputar");
    } else if (isContainsSuffix(data)) {
      debug->print("| Status: Motor Berputar dengan Hambatan");
    } else if (reg2 > 0 && reg3 <= 0x3E) {
      debug->print("| Status: Motor Berputar Normal");
    }
  }

  debug->println("");

  if (messageCount >= 9) {
    messageCount = 0;
    isNewSequence = false;
  }
}

bool DriverBridge::isContainsSuffix(byte* data) {
  return data[11] > 0x00;
}