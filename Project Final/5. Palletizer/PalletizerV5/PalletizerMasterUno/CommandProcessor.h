#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>

class CommandProcessor {
public:
  enum Command {
    CMD_NONE = 0,
    CMD_RUN = 1,
    CMD_ZERO = 2,
    CMD_SETSPEED = 6
  };

  typedef void (*CommandOutCallback)(const String& slaveId, int command, const String& params);
  typedef void (*StateCommandCallback)(const String& command);

  CommandProcessor();

  void setCommandOutCallback(CommandOutCallback callback);
  void setStateCommandCallback(StateCommandCallback callback);

  bool processCommand(const String& data);

  void sendCommandToAllSlaves(Command cmd);
  void parseCoordinateData(const String& data);

  Command getCurrentCommand() const;

private:
  CommandOutCallback commandOutCallback;
  StateCommandCallback stateCommandCallback;
  Command currentCommand;

  void processStandardCommand(const String& command);
  void processSpeedCommand(const String& data);
  void processCoordinateData(const String& data);
  void processSystemStateCommand(const String& command);

  bool isSystemStateCommand(const String& command) const;
};

#endif