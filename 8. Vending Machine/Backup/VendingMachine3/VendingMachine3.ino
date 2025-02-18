#define ENABLE_MODULE_EEPROM_LIB

#include "Kinematrix.h"
#include "AltSoftSerial.h"
#include "Config.h"
#include "Vending.h"
#include "ModbusDecoder.h"

AltSoftSerial altSerial;
EEPROMLib eeprom;
String OTAddress;

Stream* serialComm = &altSerial;
Stream* debugSerial = &Serial;

StreamDebugger debugger(debugSerial);
VendingControllerBridge vending(serialComm, &debugger);
ModbusDecoder decoder(serialComm, &debugger);

void setup() {
  Serial.begin(9600);
  altSerial.begin(9600);
  decoder.begin();
  eeprom.init();
  eeprom.writeString(0, "OT0001");
  int writeCount = eeprom.getWriteCount();
  OTAddress = eeprom.readString(0);
}

void loop() {
  if (Serial.available()) {
    char incomingChar = Serial.read();
    if (incomingChar == '\n' || incomingChar == '\r') return;
    if (incomingChar == 'z') {
      // openDoorLock();
      return;
    }
    if (incomingChar == 'x') {
      Serial.println(OTAddress);
      return;
    }
    int charDecimal = (int)incomingChar;
    if (charDecimal < 33 || charDecimal > 92) return;
    int address = charDecimal - 33;

    String motorAddressFrame = vending.generateModbusFrame(address);
    for (int i = 0; i < VendingConfig::MOTOR_COMMAND_LENGTH; i++) {
      if (i == 0) vending.writeHexString(motorAddressFrame);
      else vending.writeHexString(VendingConfig::MOTOR_COMMAND_DATA);
      delay(VendingConfig::COMMAND_DELAYS[i]);
      decoder.receiveData();
    }
  }

  // receiveData();
}