// AndroidBridge.cpp
#include "AndroidBridge.h"

AndroidBridge::AndroidBridge(Stream* androidSerial, Stream* debugSerial)
  : android(androidSerial), debug(debugSerial), lastCommand(-1) {}

void AndroidBridge::begin() {
}

void AndroidBridge::handleCommands() {
  if (android && android->available()) {
    char incomingChar = android->read();
    if (incomingChar == '\n' || incomingChar == '\r') return;

    int charDecimal = (int)incomingChar;
    if (charDecimal < 33 || charDecimal > 92) return;

    lastCommand = charDecimal - 33;
    if (debug) {
      debug->print("Received command: ");
      debug->println(lastCommand);
    }
  }
}

void AndroidBridge::sendResponse(const String& response) {
  if (android) android->println(response);
  if (debug) {
    debug->print("Sent response: ");
    debug->println(response);
  }
}

int AndroidBridge::getReceivedCommand() {
  int command = lastCommand;
  lastCommand = -1;
  return command;
}