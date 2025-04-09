#define ENABLE_MODULE_SERIAL_ENHANCED
#include "Kinematrix.h"
#include "SoftwareSerial.h"

SoftwareSerial slaveCommSerial(8, 9);  // RX, TX for slave communication

EnhancedSerial bluetoothSerial;  // For Android-Master communication
EnhancedSerial slaveSerial;      // For Master-Slave communication
EnhancedSerial debugSerial;      // For debugging

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
  Serial.begin(9600);           // Hardware serial for Bluetooth
  slaveCommSerial.begin(9600);  // Software serial for slave communication

  bluetoothSerial.begin(&Serial);
  slaveSerial.begin(&slaveCommSerial);
  debugSerial.begin(&Serial);

  bluetoothSerial.setDataCallback(onBluetoothData);
  slaveSerial.setDataCallback(onSlaveData);

  debugSerial.println("MASTER: System initialized");
}

void loop() {
  bluetoothSerial.checkCallback();
  slaveSerial.checkCallback();
}

void sendCommandToAllSlaves(Command cmd) {
  const char* slaveIds[] = { "x", "y", "z", "t", "g" };
  for (int i = 0; i < 5; i++) {
    String command = String(slaveIds[i]) + ";" + String(cmd);
    slaveSerial.println(command);
    debugSerial.println("MASTER→SLAVE: " + command);
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
    String command = slaveId + ";" + String(currentCommand) + ";" + params;

    slaveSerial.println(command);
    debugSerial.println("MASTER→SLAVE: " + command);

    pos = data.indexOf(',', closePos);
    pos = (pos == -1) ? data.length() : pos + 1;
  }
}

void onBluetoothData(const String& data) {
  debugSerial.println("ANDROID→MASTER: " + data);

  String upperData = data;
  upperData.trim();
  upperData.toUpperCase();

  if (upperData == "START") {
    currentCommand = CMD_START;
    debugSerial.println("MASTER: Command set to START");
    bluetoothSerial.println("[FEEDBACK] START DONE");
  } else if (upperData == "ZERO") {
    currentCommand = CMD_ZERO;
    debugSerial.println("MASTER: Command set to ZERO");
    sendCommandToAllSlaves(CMD_ZERO);
    bluetoothSerial.println("[FEEDBACK] ZERO DONE");
  } else if (upperData == "PAUSE") {
    currentCommand = CMD_PAUSE;
    debugSerial.println("MASTER: Command set to PAUSE");
    sendCommandToAllSlaves(CMD_PAUSE);
    bluetoothSerial.println("[FEEDBACK] PAUSE DONE");
  } else if (upperData == "RESUME") {
    currentCommand = CMD_RESUME;
    debugSerial.println("MASTER: Command set to RESUME");
    sendCommandToAllSlaves(CMD_RESUME);
    bluetoothSerial.println("[FEEDBACK] RESUME DONE");
  } else if (upperData == "RESET") {
    currentCommand = CMD_RESET;
    debugSerial.println("MASTER: Command set to RESET");
    sendCommandToAllSlaves(CMD_RESET);
    currentCommand = CMD_NONE;
    bluetoothSerial.println("[FEEDBACK] RESET DONE");
  } else if (currentCommand == CMD_START) {
    debugSerial.println("MASTER: Processing coordinates");
    parseCoordinateData(data);
    bluetoothSerial.println("[FEEDBACK] DONE");
  }
}

void onSlaveData(const String& data) {
  debugSerial.println("SLAVE→MASTER: " + data);
  bluetoothSerial.println("[SLAVE] " + data);
}