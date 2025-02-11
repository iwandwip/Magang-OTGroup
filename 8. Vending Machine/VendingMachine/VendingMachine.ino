#define ENABLE_MODULE_EEPROM_LIB

#include "Kinematrix.h"
#include "AltSoftSerial.h"

AltSoftSerial altSerial;
EEPROMLib eeprom;

String OTAddress;

void setup() {
  Serial.begin(9600);
  altSerial.begin(9600);
  eeprom.init();
  // eeprom.writeString(0, "OT0001");
  int writeCount = eeprom.getWriteCount();
  OTAddress = eeprom.readString(0);

  // Serial.print("| writeCount: ");
  // Serial.print(writeCount);
  // Serial.print("| OTAddress: ");
  // Serial.print(OTAddress);
  // Serial.println();
}

void loop() {
  if (Serial.available()) {
    char incomingChar = Serial.read();
    if (incomingChar == '\n' || incomingChar == '\r') return;
    if (incomingChar == 'x') Serial.println(OTAddress);
    int charDecimal = (int)incomingChar;
    if (charDecimal < 33 || charDecimal > 92) return;
    int address = charDecimal - 33;

    Serial.print("| incomingChar: ");
    Serial.print(incomingChar);
    Serial.print("| charDecimal: ");
    Serial.print(charDecimal);
    Serial.print("| address: ");
    Serial.print(address);
    // Serial.println();

    writeCommand(address);
    // delay(8000);
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

  Serial.print("| command 0x");
  Serial.print(addr < 0x10 ? "0" : "");
  Serial.print(addr, HEX);
  Serial.print(": ");

  for (int i = 0; i < 20; i++) {
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