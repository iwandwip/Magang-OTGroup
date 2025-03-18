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
        bluetoothSerial.println("DONE");
        debugSerial.println("MASTER: All slaves completed sequence");

        if (!isQueueEmpty()) {
          debugSerial.println("MASTER: Processing next command from queue");
          processNextCommand();
        } else if (!requestNextCommand) {
          requestCommand();
        }
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
  if (requestNextCommand) {
    requestNextCommand = false;
    if (data != "END_QUEUE") {
      addToQueue(data);
      if (!isQueueFull()) {
        requestCommand();
      }
    }
    return;
  }

  if (!sequenceRunning && !waitingForCompletion) {
    String upperData = data;
    upperData.trim();
    upperData.toUpperCase();

    if (upperData == "ZERO") {
      processStandardCommand(upperData);
    } else if (upperData.startsWith("SPEED;")) {
      processSpeedCommand(data);
    } else {
      processCoordinateData(data);
    }

    if (!isQueueFull()) {
      requestCommand();
    }
  } else {
    addToQueue(data);
  }
}

void PalletizerMaster::processStandardCommand(const String& command) {
  if (command == "ZERO") {
    currentCommand = CMD_ZERO;
    debugSerial.println("MASTER: Command set to ZERO");
    sendCommandToAllSlaves(CMD_ZERO);
    sequenceRunning = true;
    waitingForCompletion = indicatorEnabled;
    lastCheckTime = millis();
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
  } else {
    const char* slaveIds[] = { "x", "y", "z", "t", "g" };
    for (int i = 0; i < 5; i++) {
      String command = String(slaveIds[i]) + ";" + String(CMD_SETSPEED) + ";" + params;
      slaveSerial.println(command);
      debugSerial.println("MASTER→SLAVE: " + command);
    }
  }
}

void PalletizerMaster::processCoordinateData(const String& data) {
  debugSerial.println("MASTER: Processing coordinates");
  currentCommand = CMD_RUN;  // Set to RUN when processing coordinates
  parseCoordinateData(data);
  sequenceRunning = true;
  waitingForCompletion = indicatorEnabled;
  lastCheckTime = millis();

  if (!indicatorEnabled) {
    bluetoothSerial.println("DONE");
  }
}

void PalletizerMaster::onSlaveData(const String& data) {
  debugSerial.println("SLAVE→MASTER: " + data);

  if (!indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (data.indexOf("SEQUENCE COMPLETED") != -1) {
      sequenceRunning = false;
      waitingForCompletion = false;
      bluetoothSerial.println("DONE");
      debugSerial.println("MASTER: All slaves completed sequence (based on message)");

      if (!isQueueEmpty()) {
        debugSerial.println("MASTER: Processing next command from queue");
        processNextCommand();
      } else if (!requestNextCommand) {
        requestCommand();
      }
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

void PalletizerMaster::addToQueue(const String& command) {
  if (isQueueFull()) {
    debugSerial.println("MASTER: Command queue is full, dropping command: " + command);
    return;
  }

  commandQueue[queueTail] = command;
  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  queueSize++;

  debugSerial.println("MASTER: Added command to queue: " + command + " (Queue size: " + String(queueSize) + ")");
}

String PalletizerMaster::getFromQueue() {
  if (isQueueEmpty()) {
    return "";
  }

  String command = commandQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
  queueSize--;

  debugSerial.println("MASTER: Processing command from queue: " + command + " (Queue size: " + String(queueSize) + ")");

  return command;
}

bool PalletizerMaster::isQueueEmpty() {
  return queueSize == 0;
}

bool PalletizerMaster::isQueueFull() {
  return queueSize >= MAX_QUEUE_SIZE;
}

void PalletizerMaster::processNextCommand() {
  if (isQueueEmpty()) {
    debugSerial.println("MASTER: Command queue is empty");
    return;
  }

  String command = getFromQueue();

  // Process the command as before
  String upperData = command;
  upperData.trim();
  upperData.toUpperCase();

  if (upperData == "ZERO") {
    processStandardCommand(upperData);
  } else if (upperData.startsWith("SPEED;")) {
    processSpeedCommand(command);
  } else {
    // Process coordinate data
    processCoordinateData(command);
  }

  // If the queue is getting low and we're not waiting for completion,
  // request more commands
  if (queueSize < 3 && !waitingForCompletion && !sequenceRunning) {
    requestCommand();
  }
}

void PalletizerMaster::requestCommand() {
  if (!isQueueFull()) {
    requestNextCommand = true;
    bluetoothSerial.println("NEXT");  // Request next command from bluetooth
    debugSerial.println("MASTER: Requesting next command");
  }
}