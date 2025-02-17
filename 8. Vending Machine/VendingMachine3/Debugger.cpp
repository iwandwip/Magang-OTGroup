#include "Debugger.h"

StreamDebugger::StreamDebugger(Stream* debugStream)
  : debug(debugStream) {}

void StreamDebugger::print(const char* message) {
  if (debug != nullptr) debug->print(message);
}

void StreamDebugger::print(String message) {
  if (debug != nullptr) debug->print(message);
}

void StreamDebugger::println(const char* message) {
  if (debug != nullptr) debug->println(message);
}

void StreamDebugger::printHex(byte value) {
  if (debug != nullptr) {
    if (value < 0x10) debug->print("0");
    debug->print(value, HEX);
  }
}