#define ENABLE_MODULE_EEPROM_LIB

#include "Kinematrix.h"
#include "AltSoftSerial.h"
#include "Config.h"

AltSoftSerial altSerial;
EEPROMLib eeprom;

String OTAddress;

void setup() {
  Serial.begin(9600);
  altSerial.begin(9600);
  eeprom.init();
  eeprom.writeString(0, "OT0001");
  int writeCount = eeprom.getWriteCount();
  OTAddress = eeprom.readString(0);

  // Serial.print("| writeCount: ");
  // Serial.print(writeCount);
  // Serial.print("| OTAddress: ");
  // Serial.print(OTAddress);
  // Serial.println();

  // for (int i = 0; i < 60; i++) {
  //   writeCommand(i);
  // }
}

void loop() {
  if (Serial.available()) {
    char incomingChar = Serial.read();
    if (incomingChar == '\n' || incomingChar == '\r') return;
    if (incomingChar == 'z') {
      openLock();
      return;
    }
    if (incomingChar == 'x') {
      Serial.println(OTAddress);
      return;
    }
    int charDecimal = (int)incomingChar;
    if (charDecimal < 33 || charDecimal > 92) return;
    int address = charDecimal - 33;
    writeCommand(address);
  }
  receiveData();
}

void receiveData() {
  static byte receivedData[20];
  static int index = 0;

  if (altSerial.available()) {
    if (index == 0) Serial.print("| recv: ");
    while (altSerial.available() && index < 20) {
      receivedData[index] = altSerial.read();
      index++;
    }
    if (index == 20) {
      for (int i = 0; i < 20; i++) {
        if (receivedData[i] < 0x10) Serial.print("0");
        Serial.print(receivedData[i], HEX);
      }
      // Serial.println();
      index = 0;

      decodeModbusData(receivedData);
    }
  }
}

void openLock() {
  for (int i = 0; i < 2; i++) {
    Serial.print("| open: ");
    for (int j = 0; j < 20; j++) {
      if (lockDataArr[i][j] < 0x10) Serial.print("0");
      Serial.print(lockDataArr[i][j], HEX);
      altSerial.write(lockDataArr[i][j]);
    }
    Serial.println();
    delay(250);
  }
}

void prepareCommand(uint8_t address, byte* data) {
  data[0] = 0x01;                 // Device address
  data[1] = 0x05;                 // Function code
  data[2] = address;              // Parameter address
  data[3] = 0x03;                 // Operand 1
  data[4] = 0x01;                 // Operand 2
  for (int i = 5; i < 18; i++) {  // Padding 14 bytes
    data[i] = 0x00;
  }
  uint16_t crc = calculateCRC16Modbus(data, 18);  // Hitung CRC untuk 18 byte pertama
  data[19] = crc >> 8;                            // High byte
  data[18] = crc & 0xFF;                          // Low byte
}

void writeCommand(uint8_t addr) {
  byte command[20];
  prepareCommand(addr, command);

  Serial.print("| comm: ");
  for (int i = 0; i < 20; i++) {
    if (command[i] < 0x10) Serial.print("0");
    Serial.print(command[i], HEX);
    altSerial.write(command[i]);
  }
  Serial.println();
}

uint16_t calculateCRC16Modbus(byte* data, int length) {
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