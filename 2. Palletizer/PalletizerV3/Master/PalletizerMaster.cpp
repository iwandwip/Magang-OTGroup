#include "PalletizerMaster.h"

PalletizerMaster* PalletizerMaster::instance = nullptr;

PalletizerMaster::PalletizerMaster(int rxPin, int txPin, int indicatorPin)
  : slaveCommSerial(rxPin, txPin), indicatorPin(indicatorPin) {
  instance = this;
  indicatorEnabled = (indicatorPin != -1);
}

void PalletizerMaster::begin() {
  Serial.begin(9600);
  slaveCommSerial.begin(9600);

  bluetoothSerial.begin(&Serial);
  slaveSerial.begin(&slaveCommSerial);
  debugSerial.begin(&Serial);

  bluetoothSerial.setDataCallback(onBluetoothDataWrapper);
  slaveSerial.setDataCallback(onSlaveDataWrapper);

  if (indicatorEnabled) {
    pinMode(indicatorPin, INPUT_PULLUP);
    debugSerial.println("MASTER: Indicator pin enabled on pin " + String(indicatorPin));
  } else {
    debugSerial.println("MASTER: Indicator pin disabled");
  }

  debugSerial.println("MASTER: System initialized");
}

void PalletizerMaster::update() {
  bluetoothSerial.checkCallback();
  slaveSerial.checkCallback();

  if (indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (millis() - lastCheckTime > 50) {
      lastCheckTime = millis();

      if (checkAllSlavesCompleted()) {
        sequenceRunning = false;
        waitingForCompletion = false;
        // bluetoothSerial.println("[FEEDBACK] ALL_SLAVES_COMPLETED");
        bluetoothSerial.println("DONE");
        debugSerial.println("MASTER: All slaves completed sequence");
      }
    }
  }
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

  if (upperData == "START" || upperData == "ZERO" || upperData == "PAUSE" || upperData == "RESUME" || upperData == "RESET") {
    processStandardCommand(upperData);
  } else if (upperData.startsWith("SPEED;")) {
    processSpeedCommand(data);
  } else if (currentCommand == CMD_START) {
    processCoordinateData(data);
  } else {
    debugSerial.println("MASTER: Unknown command: " + upperData);
    // bluetoothSerial.println("[FEEDBACK] UNKNOWN COMMAND");
  }
}

void PalletizerMaster::processStandardCommand(const String& command) {
  if (command == "START") {
    currentCommand = CMD_START;
    debugSerial.println("MASTER: Command set to START");
    sequenceRunning = false;
    waitingForCompletion = false;
    // bluetoothSerial.println("[FEEDBACK] START DONE");
  } else if (command == "ZERO") {
    currentCommand = CMD_ZERO;
    debugSerial.println("MASTER: Command set to ZERO");
    sendCommandToAllSlaves(CMD_ZERO);
    sequenceRunning = true;
    waitingForCompletion = indicatorEnabled;
    lastCheckTime = millis();
    // bluetoothSerial.println("[FEEDBACK] ZERO DONE");
  } else if (command == "PAUSE") {
    currentCommand = CMD_PAUSE;
    debugSerial.println("MASTER: Command set to PAUSE");
    sendCommandToAllSlaves(CMD_PAUSE);
    // bluetoothSerial.println("[FEEDBACK] PAUSE DONE");
  } else if (command == "RESUME") {
    currentCommand = CMD_RESUME;
    debugSerial.println("MASTER: Command set to RESUME");
    sendCommandToAllSlaves(CMD_RESUME);
    sequenceRunning = true;
    waitingForCompletion = indicatorEnabled;
    lastCheckTime = millis();
    // bluetoothSerial.println("[FEEDBACK] RESUME DONE");
  } else if (command == "RESET") {
    currentCommand = CMD_RESET;
    debugSerial.println("MASTER: Command set to RESET");
    sendCommandToAllSlaves(CMD_RESET);
    currentCommand = CMD_NONE;
    sequenceRunning = false;
    waitingForCompletion = false;
    // bluetoothSerial.println("[FEEDBACK] RESET DONE");
  }
}

void PalletizerMaster::processSpeedCommand(const String& data) {
  String params = data.substring(6);
  int separatorPos = params.indexOf(';');

  if (separatorPos != -1) {
    String slaveId = params.substring(0, separatorPos);
    String speedValue = params.substring(separatorPos + 1);

    slaveId.toLowerCase();
    String command = slaveId + ";" + String(CMD_SETSPEED) + ";" + speedValue;

    slaveSerial.println(command);
    debugSerial.println("MASTER→SLAVE: " + command);
    // bluetoothSerial.println("[FEEDBACK] SPEED COMMAND SENT TO " + slaveId);
  } else {
    const char* slaveIds[] = { "x", "y", "z", "t", "g" };
    for (int i = 0; i < 5; i++) {
      String command = String(slaveIds[i]) + ";" + String(CMD_SETSPEED) + ";" + params;
      slaveSerial.println(command);
      debugSerial.println("MASTER→SLAVE: " + command);
    }
    // bluetoothSerial.println("[FEEDBACK] SPEED COMMAND SENT TO ALL SLAVES");
  }
}

void PalletizerMaster::processCoordinateData(const String& data) {
  debugSerial.println("MASTER: Processing coordinates");
  parseCoordinateData(data);
  sequenceRunning = true;
  waitingForCompletion = indicatorEnabled;
  lastCheckTime = millis();

  if (!indicatorEnabled) {
    // bluetoothSerial.println("[FEEDBACK] ALL_SLAVES_COMPLETED");
    bluetoothSerial.println("DONE");
  }

  // bluetoothSerial.println("[FEEDBACK] DONE");
}

void PalletizerMaster::onSlaveData(const String& data) {
  debugSerial.println("SLAVE→MASTER: " + data);
  // bluetoothSerial.println("[SLAVE] " + data);

  if (!indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (data.indexOf("SEQUENCE COMPLETED") != -1) {
      sequenceRunning = false;
      waitingForCompletion = false;
      // bluetoothSerial.println("[FEEDBACK] ALL_SLAVES_COMPLETED");
      bluetoothSerial.println("DONE");
      debugSerial.println("MASTER: All slaves completed sequence (based on message)");
    }
  }
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

    String paramsOrig = data.substring(endPos + 1, closePos);

    String params = "";
    for (int i = 0; i < paramsOrig.length(); i++) {
      params += (paramsOrig.charAt(i) == ',') ? ';' : paramsOrig.charAt(i);
    }

    String command = slaveId + ";" + String(currentCommand) + ";" + params;
    slaveSerial.println(command);
    debugSerial.println("MASTER→SLAVE: " + command);

    pos = data.indexOf(',', closePos);
    pos = (pos == -1) ? data.length() : pos + 1;
  }
}

bool PalletizerMaster::checkAllSlavesCompleted() {
  if (!indicatorEnabled) {
    return false;
  }
  return digitalRead(indicatorPin) == HIGH;
}