#define ENABLE_MODULE_SERIAL_ENHANCED
#include "Kinematrix.h"
#include "SoftwareSerial.h"

SoftwareSerial baseCommSlave(8, 9);

EnhancedSerial bluetooth;
EnhancedSerial slave;
EnhancedSerial debug;

enum Command {
  CMD_NONE,
  CMD_START,
  CMD_ZERO,
  CMD_PAUSE,
  CMD_RESUME,
  CMD_RESET
};

Command currentCommand = CMD_NONE;

void setup() {
  Serial.begin(9600);
  baseCommSlave.begin(9600);

  bluetooth.begin(&Serial);
  slave.begin(&Serial);
  debug.begin(&Serial);

  bluetooth.setDataCallback(onReceiveData);
}

void loop() {
  bluetooth.checkCallback();
}

void sendCommandToAllSlaves(Command cmd) {
  static const char* slaveIds[] = { "x", "y", "z", "t", "g" };
  static const int SLAVE_COUNT = 5;
  for (int i = 0; i < SLAVE_COUNT; i++) {
    debug.println(String(slaveIds[i]) + ";" + String(cmd));
  }
}

void parseCoordinateData(const String& data) {
  int pos = 0, endPos;
  while (pos < data.length()) {
    endPos = data.indexOf('(', pos);
    if (endPos == -1) break;

    String slaveId = data.substring(pos, endPos);
    slaveId.trim();
    slaveId.toLowerCase();

    int closePos = data.indexOf(')', endPos);
    if (closePos == -1) break;

    String params = data.substring(endPos + 1, closePos);
    String cmd = slaveId + ";" + String(currentCommand);

    int start = 0, comma;
    do {
      comma = params.indexOf(',', start);
      cmd += ";" + (comma == -1 ? params.substring(start) : params.substring(start, comma));
      start = comma + 1;
    } while (comma != -1 && start < params.length());

    debug.println(cmd);

    pos = data.indexOf(',', closePos);
    pos = (pos == -1) ? data.length() : pos + 1;
  }
}

void onReceiveData(const String& data) {
  String upperData = data;
  upperData.trim();
  upperData.toUpperCase();

  if (upperData == "START") {
    currentCommand = CMD_START;
    debug.println("Command: START");
    bluetooth.println("[FEEDBACK] START DONE");
    return;
  } else if (upperData == "ZERO") {
    currentCommand = CMD_ZERO;
    debug.println("Command: ZERO");
    sendCommandToAllSlaves(CMD_ZERO);
    bluetooth.println("[FEEDBACK] ZERO DONE");
    return;
  } else if (upperData == "PAUSE") {
    currentCommand = CMD_PAUSE;
    debug.println("Command: PAUSE");
    sendCommandToAllSlaves(CMD_PAUSE);
    bluetooth.println("[FEEDBACK] PAUSE DONE");
    return;
  } else if (upperData == "RESUME") {
    currentCommand = CMD_RESUME;
    debug.println("Command: RESUME");
    sendCommandToAllSlaves(CMD_RESUME);
    bluetooth.println("[FEEDBACK] RESUME DONE");
    return;
  } else if (upperData == "RESET") {
    currentCommand = CMD_RESET;
    debug.println("Command: RESET");
    sendCommandToAllSlaves(CMD_RESET);
    currentCommand = CMD_NONE;
    bluetooth.println("[FEEDBACK] RESET DONE");
    return;
  }

  if (currentCommand == CMD_START) {
    parseCoordinateData(data);
    bluetooth.println("[FEEDBACK] DONE");
  }
}