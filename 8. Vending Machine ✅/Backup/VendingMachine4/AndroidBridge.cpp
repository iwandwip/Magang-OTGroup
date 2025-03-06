// AndroidBridge.cpp
#include "AndroidBridge.h"

AndroidBridge::AndroidBridge(Stream* androidSerial, Stream* debugSerial)
  : android(androidSerial), debug(debugSerial), lastCommand(-1) {
  char buffer[OT_ADDRESS_LENGTH + 1];  // +1 for null terminator
  for (int i = 0; i < OT_ADDRESS_LENGTH; i++) {
    buffer[i] = EEPROM.read(OT_ADDRESS_LOCATION + i);
  }
  buffer[OT_ADDRESS_LENGTH] = '\0';
  OTAddress = String(buffer);
}

void AndroidBridge::begin() {
}

void AndroidBridge::handleCommands() {
  if (android && android->available()) {
    char incomingChar = android->read();

    if (incomingChar == 'y') {
      unlockRequested = true;
      if (debug) {
        debug->println("Unlock command received");
      }
      return;
    }

    if (incomingChar == 'x') {
      if (debug) {
        debug->println("OT Address request received");
      }
      sendResponse(OTAddress);
      return;
    }

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

bool AndroidBridge::isUnlocking() {
  if (unlockRequested) {
    unlockRequested = false;
    return true;
  }
  return false;
}

String AndroidBridge::getOTAddress() {
  return OTAddress;
}

void AndroidBridge::writeOTAddress(const String& address) {
  // Write address to EEPROM
  for (int i = 0; i < OT_ADDRESS_LENGTH && i < address.length(); i++) {
    EEPROM.write(OT_ADDRESS_LOCATION + i, address[i]);
  }
  OTAddress = address;

  if (debug) {
    debug->print("New OT Address written: ");
    debug->println(address);
  }
}