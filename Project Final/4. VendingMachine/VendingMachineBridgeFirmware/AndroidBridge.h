#pragma once

#include "Arduino.h"
#include "EEPROM.h"

class AndroidBridge {
public:
  AndroidBridge(Stream* androidSerial, Stream* debugSerial);
  void begin();
  void handleCommands();
  void sendResponse(const String& response);
  int getReceivedCommand();
  bool isUnlocking();
  String getOTAddress();
  void writeOTAddress(const String& address);
  void flush();

private:
  Stream* android;
  Stream* debug;
  int lastCommand;
  bool unlockRequested;
  String OTAddress;
  static const int OT_ADDRESS_LOCATION = 0;  
  static const int OT_ADDRESS_LENGTH = 6;
};