#include "PalletizerMaster.h"

PalletizerMaster* PalletizerMaster::instance = nullptr;

PalletizerMaster::PalletizerMaster(int rxPin, int txPin, int indicatorPin)
  : slaveCommSerial(rxPin, txPin), indicatorPin(indicatorPin) {
  instance = this;
  indicatorEnabled = (indicatorPin != -1);  // Enable indicator feature only if pin is valid
}

void PalletizerMaster::begin() {
  Serial.begin(9600);
  slaveCommSerial.begin(9600);

  bluetoothSerial.begin(&Serial);
  slaveSerial.begin(&slaveCommSerial);
  debugSerial.begin(&Serial);

  bluetoothSerial.setDataCallback(onBluetoothDataWrapper);
  slaveSerial.setDataCallback(onSlaveDataWrapper);

  // Initialize indicator pin only if enabled
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

  // Check for sequence completion if needed and indicator is enabled
  if (indicatorEnabled && waitingForCompletion && sequenceRunning) {
    // Only check every 50ms to avoid excessive checks
    if (millis() - lastCheckTime > 50) {
      lastCheckTime = millis();

      if (checkAllSlavesCompleted()) {
        sequenceRunning = false;
        waitingForCompletion = false;

        // Send completion feedback to GUI
        bluetoothSerial.println("[FEEDBACK] ALL_SLAVES_COMPLETED");
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

  if (upperData == "START") {
    currentCommand = CMD_START;
    debugSerial.println("MASTER: Command set to START");
    sequenceRunning = false;  // Reset sequence status
    waitingForCompletion = false;
    bluetoothSerial.println("[FEEDBACK] START DONE");
  } else if (upperData == "ZERO") {
    currentCommand = CMD_ZERO;
    debugSerial.println("MASTER: Command set to ZERO");
    sendCommandToAllSlaves(CMD_ZERO);
    sequenceRunning = true;
    waitingForCompletion = indicatorEnabled;  // Only wait if indicator is enabled
    lastCheckTime = millis();
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
    sequenceRunning = true;
    waitingForCompletion = indicatorEnabled;  // Only wait if indicator is enabled
    lastCheckTime = millis();
    bluetoothSerial.println("[FEEDBACK] RESUME DONE");
  } else if (upperData == "RESET") {
    currentCommand = CMD_RESET;
    debugSerial.println("MASTER: Command set to RESET");
    sendCommandToAllSlaves(CMD_RESET);
    currentCommand = CMD_NONE;
    sequenceRunning = false;
    waitingForCompletion = false;
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
    sequenceRunning = true;
    waitingForCompletion = indicatorEnabled;  // Only wait if indicator is enabled
    lastCheckTime = millis();

    // If indicator is disabled, immediately send completion feedback
    if (!indicatorEnabled) {
      bluetoothSerial.println("[FEEDBACK] ALL_SLAVES_COMPLETED");
    }

    bluetoothSerial.println("[FEEDBACK] DONE");
  }
}

void PalletizerMaster::onSlaveData(const String& data) {
  debugSerial.println("SLAVE→MASTER: " + data);
  bluetoothSerial.println("[SLAVE] " + data);

  // If indicator is disabled, check for sequence completion messages from slaves
  if (!indicatorEnabled && waitingForCompletion && sequenceRunning) {
    // If any slave reports sequence completed, send feedback to GUI
    // This is an alternative method when indicator pin is not used
    if (data.indexOf("SEQUENCE COMPLETED") != -1) {
      // Count completed sequences from each slave
      // For simplicity, we're just going to assume all slaves are done if one reports completion
      // In a more sophisticated implementation, you would track which slaves have reported completion

      sequenceRunning = false;
      waitingForCompletion = false;

      // Send completion feedback to GUI
      bluetoothSerial.println("[FEEDBACK] ALL_SLAVES_COMPLETED");
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

bool PalletizerMaster::checkAllSlavesCompleted() {
  if (!indicatorEnabled) {
    return false;  // If indicator is disabled, don't check
  }

  // Read the indicator pin
  // The pin will be HIGH when all slaves have set their indicator pins to HIGH (completed)
  return digitalRead(indicatorPin) == HIGH;
}