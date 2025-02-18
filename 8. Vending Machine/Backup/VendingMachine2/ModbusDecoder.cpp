#include "ModbusDecoder.h"

ModbusDecoder::ModbusDecoder(Stream* commStream, IDebugger* debugger)
  : ModbusBase(commStream, debugger),
    index(0),
    startTime(0),
    lastEndTime(0),
    isWaiting(false),
    messageCount(0),
    isNewSequence(false),
    lastFunction(0) {}

void ModbusDecoder::begin() {
  index = 0;
  startTime = 0;
  lastEndTime = 0;
  isWaiting = false;
  messageCount = 0;
  isNewSequence = false;
  lastFunction = 0;
}

void ModbusDecoder::receiveData() {
  if (index == 0 && !isWaiting) {
    startTime = millis();
    isWaiting = true;
  }

  if (comm->available()) {
    if (index == 0) {
      debug->print("| recv: ");
    }

    while (comm->available() && index < FRAME_SIZE) {
      receivedData[index] = comm->read();
      index++;
    }

    if (index == FRAME_SIZE) {
      for (int i = 0; i < FRAME_SIZE; i++) {
        debug->printHex(receivedData[i]);
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

void ModbusDecoder::decodeModbusData(byte* data) {
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

  uint16_t crcCalculated = ModbusUtils::calculateCRC16(data, 18);
  if (crcReceived != crcCalculated) {
    debug->println("| CRC Error: Data corrupted!");
    return;
  }

  uint16_t reg1 = (data[3] << 8) | data[4];   // Bytes 3-4
  uint16_t reg2 = (data[5] << 8) | data[6];   // Bytes 5-6  (Counter/ID)
  uint16_t reg3 = (data[7] << 8) | data[8];   // Bytes 7-8  (Data 1)
  uint16_t reg4 = (data[9] << 8) | data[10];  // Bytes 9-10 (Data 2)

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

bool ModbusDecoder::isContainsSuffix(byte* data) {
  return data[11] > 0x00;
}