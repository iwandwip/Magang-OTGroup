#include "CommandProcessor.h"

CommandProcessor::CommandProcessor()
  : commandOutCallback(nullptr),
    stateCommandCallback(nullptr),
    currentCommand(CMD_NONE) {
}

void CommandProcessor::setCommandOutCallback(CommandOutCallback callback) {
  commandOutCallback = callback;
}

void CommandProcessor::setStateCommandCallback(StateCommandCallback callback) {
  stateCommandCallback = callback;
}

bool CommandProcessor::processCommand(const String& data) {
  String upperData = data;
  upperData.trim();
  upperData.toUpperCase();

  if (isSystemStateCommand(upperData)) {
    processSystemStateCommand(upperData);
    return true;
  }

  if (upperData == "ZERO") {
    processStandardCommand(upperData);
    return true;
  } else if (upperData.startsWith("SPEED;")) {
    processSpeedCommand(data);
    return true;
  } else if (upperData != "END_QUEUE") {
    processCoordinateData(data);
    return true;
  }

  return false;
}

void CommandProcessor::sendCommandToAllSlaves(Command cmd) {
  currentCommand = cmd;

  if (commandOutCallback) {
    const char* slaveIds[] = { "x", "y", "z", "t", "g" };
    for (int i = 0; i < 5; i++) {
      commandOutCallback(slaveIds[i], static_cast<int>(cmd), "");
    }
  }
}

void CommandProcessor::parseCoordinateData(const String& data) {
  if (!commandOutCallback) return;

  int pos = 0, endPos;
  while (pos < data.length()) {
    endPos = data.indexOf('(', pos);
    if (endPos == -1) break;

    String slaveId = data.substring(pos, endPos);
    slaveId.trim();
    slaveId.toLowerCase();

    int closePos = data.indexOf(')', endPos);
    if (closePos == -1) break;

    String paramsOrig = data.substring(endPos + 1, closePos);

    String params = "";
    for (int i = 0; i < paramsOrig.length(); i++) {
      params += (paramsOrig.charAt(i) == ',') ? ';' : paramsOrig.charAt(i);
    }

    commandOutCallback(slaveId, static_cast<int>(currentCommand), params);

    pos = data.indexOf(',', closePos);
    pos = (pos == -1) ? data.length() : pos + 1;
  }
}

CommandProcessor::Command CommandProcessor::getCurrentCommand() const {
  return currentCommand;
}

void CommandProcessor::processStandardCommand(const String& command) {
  if (command == "ZERO") {
    currentCommand = CMD_ZERO;
    sendCommandToAllSlaves(CMD_ZERO);
  }
}

void CommandProcessor::processSpeedCommand(const String& data) {
  currentCommand = CMD_SETSPEED;

  if (!commandOutCallback) return;

  String params = data.substring(6);
  int separatorPos = params.indexOf(';');

  if (separatorPos != -1) {
    String slaveId = params.substring(0, separatorPos);
    String speedValue = params.substring(separatorPos + 1);

    slaveId.toLowerCase();
    commandOutCallback(slaveId, static_cast<int>(CMD_SETSPEED), speedValue);
  } else {
    const char* slaveIds[] = { "x", "y", "z", "t", "g" };
    for (int i = 0; i < 5; i++) {
      commandOutCallback(slaveIds[i], static_cast<int>(CMD_SETSPEED), params);
    }
  }
}

void CommandProcessor::processCoordinateData(const String& data) {
  currentCommand = CMD_RUN;
  parseCoordinateData(data);
}

void CommandProcessor::processSystemStateCommand(const String& command) {
  if (stateCommandCallback) {
    stateCommandCallback(command);
  }
}

bool CommandProcessor::isSystemStateCommand(const String& command) const {
  return command == "IDLE" || command == "PLAY" || command == "PAUSE" || command == "STOP";
}