#pragma once
#include "Arduino.h"

class IDebugger {
public:
  virtual ~IDebugger() = default;
  virtual void print(const char* message) = 0;
  virtual void print(String message) = 0;
  virtual void println(const char* message) = 0;
  virtual void printHex(byte value) = 0;
};

class StreamDebugger : public IDebugger {
public:
  explicit StreamDebugger(Stream* debugStream);
  void print(const char* message) override;
  void print(String message) override;
  void println(const char* message) override;
  void printHex(byte value) override;

private:
  Stream* debug;
};