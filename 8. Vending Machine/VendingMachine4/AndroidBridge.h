// AndroidBridge.h
#pragma once
#include "Arduino.h"

class AndroidBridge {
public:
  AndroidBridge(Stream* androidSerial, Stream* debugSerial);
  void begin();
  void handleCommands();
  void sendResponse(const String& response);
  int getReceivedCommand();

private:
  Stream* android;
  Stream* debug;
  int lastCommand;
};