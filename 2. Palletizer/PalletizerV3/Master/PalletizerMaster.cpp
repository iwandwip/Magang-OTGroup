#include "PalletizerMaster.h"

PalletizerMaster* PalletizerMaster::instance = nullptr;

PalletizerMaster::PalletizerMaster(int rxPin, int txPin)
  : slaveCommSerial(rxPin, txPin) {
  instance = this;
}

void PalletizerMaster::begin() {
  Serial.begin(9600);
  slaveCommSerial.begin(9600);

  bluetoothSerial.begin(&Serial);
  slaveSerial.begin(&slaveCommSerial);
  debugSerial.begin(&Serial);

  bluetoothSerial.setDataCallback(onBluetoothDataWrapper);
  slaveSerial.setDataCallback(onSlaveDataWrapper);

  debugSerial.println("MASTER: System initialized");
}

void PalletizerMaster::update() {
  bluetoothSerial.checkCallback();
  slaveSerial.checkCallback();
}

void PalletizerMaster::onBluetoothDataWrapper(const String& data) {
  if (instance) {
    instance->onBluetoothData(data);
  }
}

void PalletizerMaster::onSlaveDataWrapper(const String& data) {
  if (instance) {
    instance->onSlaveData(data);
  }
}

void PalletizerMaster::onBluetoothData(const String& data) {
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
  } else if (upperData.startsWith("SPEED;")) {
    // Handle the speed command format: SPEED;slaveID;value OR SPEED;value
    String params = data.substring(6);  // Remove "SPEED;"

    // Check if we have a specific slave ID or should send to all
    int separatorPos = params.indexOf(';');
    if (separatorPos != -1) {
      // Format: SPEED;slaveID;value - send to specific slave
      String slaveId = params.substring(0, separatorPos);
      String speedValue = params.substring(separatorPos + 1);

      slaveId.toLowerCase();  // Convert to lowercase for consistency
      String command = slaveId + ";" + String(CMD_SETSPEED) + ";" + speedValue;

      slaveSerial.println(command);
      debugSerial.println("MASTER→SLAVE: " + command);
      bluetoothSerial.println("[FEEDBACK] SPEED COMMAND SENT TO " + slaveId);
    } else {
      // Format: SPEED;value - send to all slaves
      const char* slaveIds[] = { "x", "y", "z", "t", "g" };
      for (int i = 0; i < 5; i++) {
        String command = String(slaveIds[i]) + ";" + String(CMD_SETSPEED) + ";" + params;
        slaveSerial.println(command);
        debugSerial.println("MASTER→SLAVE: " + command);
      }
      bluetoothSerial.println("[FEEDBACK] SPEED COMMAND SENT TO ALL SLAVES");
    }
  } else if (currentCommand == CMD_START) {
    debugSerial.println("MASTER: Processing coordinates");
    parseCoordinateData(data);
    bluetoothSerial.println("[FEEDBACK] DONE");
  }
}

void PalletizerMaster::onSlaveData(const String& data) {
  debugSerial.println("SLAVE→MASTER: " + data);
  bluetoothSerial.println("[SLAVE] " + data);
}

void PalletizerMaster::sendCommandToAllSlaves(Command cmd) {
  const char* slaveIds[] = { "x", "y", "z", "t", "g" };
  for (int i = 0; i < 5; i++) {
    String command = String(slaveIds[i]) + ";" + String(cmd);
    slaveSerial.println(command);
    debugSerial.println("MASTER→SLAVE: " + command);
  }
}

void PalletizerMaster::parseCoordinateData(const String& data) {
  int pos = 0, endPos;
  while (pos < data.length()) {
    endPos = data.indexOf('(', pos);
    if (endPos == -1) break;
    String slaveId = data.substring(pos, endPos);
    slaveId.trim();
    slaveId.toLowerCase();
    int closePos = data.indexOf(')', endPos);
    if (closePos == -1) break;

    // Get the parameters within the parentheses
    String paramsOrig = data.substring(endPos + 1, closePos);

    // Convert any commas to semicolons in the parameters
    String params = "";
    for (int i = 0; i < paramsOrig.length(); i++) {
      if (paramsOrig.charAt(i) == ',') {
        params += ';';
      } else {
        params += paramsOrig.charAt(i);
      }
    }

    String command = slaveId + ";" + String(currentCommand) + ";" + params;
    slaveSerial.println(command);
    debugSerial.println("MASTER→SLAVE: " + command);

    pos = data.indexOf(',', closePos);
    pos = (pos == -1) ? data.length() : pos + 1;
  }
}