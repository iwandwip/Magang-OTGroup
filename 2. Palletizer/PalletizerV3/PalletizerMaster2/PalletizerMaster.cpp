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
    DEBUG_PRINTLN("MASTER: Indicator pin enabled on pin " + String(indicatorPin));
  } else {
    DEBUG_PRINTLN("MASTER: Indicator pin disabled");
  }

  systemState = STATE_IDLE;
  sendStateUpdate();
  DEBUG_PRINTLN("MASTER: System initialized");
}

void PalletizerMaster::update() {
  bluetoothSerial.checkCallback();
  slaveSerial.checkCallback();

  if (queueSize < MAX_QUEUE_SIZE - 1 && !requestNextCommand && !isQueueFull()) {
    requestCommand();
  }

  if (indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (millis() - lastCheckTime > 50) {
      lastCheckTime = millis();

      if (checkAllSlavesCompleted()) {
        sequenceRunning = false;
        waitingForCompletion = false;
        bluetoothSerial.println("DONE");
        DEBUG_PRINTLN("MASTER: All slaves completed sequence");

        if (!isQueueEmpty() && systemState == STATE_RUNNING) {
          DEBUG_PRINTLN("MASTER: Processing next command from queue");
          processNextCommand();
        } else if (isQueueEmpty() && systemState == STATE_RUNNING) {
          setSystemState(STATE_IDLE);
        }
      }
    }
  }

  if (systemState == STATE_STOPPING && !sequenceRunning && !waitingForCompletion) {
    setSystemState(STATE_IDLE);
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
  DEBUG_PRINTLN("ANDROID→MASTER: " + data);
  requestNextCommand = false;

  String upperData = data;
  upperData.trim();
  upperData.toUpperCase();

  if (upperData == "IDLE" || upperData == "PLAY" || upperData == "PAUSE" || upperData == "STOP") {
    processSystemStateCommand(upperData);
    return;
  }

  if (!sequenceRunning && !waitingForCompletion) {
    if (upperData == "ZERO") {
      processStandardCommand(upperData);
    } else if (upperData.startsWith("SPEED;")) {
      processSpeedCommand(data);
    } else if (upperData != "END_QUEUE") {
      if (systemState == STATE_RUNNING) {
        processCoordinateData(data);
      } else {
        addToQueue(data);
      }
    }
  } else if (data != "END_QUEUE") {
    addToQueue(data);
  }
}

void PalletizerMaster::onSlaveData(const String& data) {
  DEBUG_PRINTLN("SLAVE→MASTER: " + data);

  if (!indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (data.indexOf("SEQUENCE COMPLETED") != -1) {
      sequenceRunning = false;
      waitingForCompletion = false;
      bluetoothSerial.println("DONE");
      DEBUG_PRINTLN("MASTER: All slaves completed sequence (based on message)");

      if (!isQueueEmpty() && systemState == STATE_RUNNING) {
        DEBUG_PRINTLN("MASTER: Processing next command from queue");
        processNextCommand();
      } else if (isQueueEmpty() && systemState == STATE_RUNNING) {
        setSystemState(STATE_IDLE);
      } else if (systemState == STATE_STOPPING) {
        clearQueue();
        setSystemState(STATE_IDLE);
      }
    }
  }
}

void PalletizerMaster::processStandardCommand(const String& command) {
  if (command == "ZERO") {
    currentCommand = CMD_ZERO;
    DEBUG_PRINTLN("MASTER: Command set to ZERO");
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
    DEBUG_PRINTLN("MASTER→SLAVE: " + command);
  } else {
    const char* slaveIds[] = { "x", "y", "z", "t", "g" };
    for (int i = 0; i < 5; i++) {
      String command = String(slaveIds[i]) + ";" + String(CMD_SETSPEED) + ";" + params;
      slaveSerial.println(command);
      DEBUG_PRINTLN("MASTER→SLAVE: " + command);
    }
  }
}

void PalletizerMaster::processCoordinateData(const String& data) {
  DEBUG_PRINTLN("MASTER: Processing coordinates");
  currentCommand = CMD_RUN;
  parseCoordinateData(data);
  sequenceRunning = true;
  waitingForCompletion = indicatorEnabled;
  lastCheckTime = millis();

  if (!indicatorEnabled) {
    bluetoothSerial.println("DONE");
  }
}

void PalletizerMaster::processSystemStateCommand(const String& command) {
  DEBUG_PRINTLN("MASTER: Processing system state command: " + command);

  if (command == "IDLE") {
    if (systemState == STATE_RUNNING || systemState == STATE_PAUSED) {
      if (sequenceRunning) {
        setSystemState(STATE_STOPPING);
      } else {
        clearQueue();
        setSystemState(STATE_IDLE);
      }
    } else {
      setSystemState(STATE_IDLE);
    }
  } else if (command == "PLAY") {
    setSystemState(STATE_RUNNING);
    if (!sequenceRunning && !waitingForCompletion && !isQueueEmpty()) {
      processNextCommand();
    }
  } else if (command == "PAUSE") {
    setSystemState(STATE_PAUSED);
  } else if (command == "STOP") {
    if (sequenceRunning) {
      setSystemState(STATE_STOPPING);
    } else {
      clearQueue();
      setSystemState(STATE_IDLE);
    }
  }
}

void PalletizerMaster::sendCommandToAllSlaves(Command cmd) {
  const char* slaveIds[] = { "x", "y", "z", "t", "g" };
  for (int i = 0; i < 5; i++) {
    String command = String(slaveIds[i]) + ";" + String(cmd);
    slaveSerial.println(command);
    DEBUG_PRINTLN("MASTER→SLAVE: " + command);
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
    DEBUG_PRINTLN("MASTER→SLAVE: " + command);

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
    DEBUG_PRINTLN("MASTER: Command queue is full, dropping command: " + command);
    return;
  }

  commandQueue[queueTail] = command;
  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  queueSize++;

  DEBUG_PRINTLN("MASTER: Added command to queue: " + command + " (Queue size: " + String(queueSize) + ")");
}

String PalletizerMaster::getFromQueue() {
  if (isQueueEmpty()) {
    return "";
  }

  String command = commandQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
  queueSize--;

  DEBUG_PRINTLN("MASTER: Processing command from queue: " + command + " (Queue size: " + String(queueSize) + ")");

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
    DEBUG_PRINTLN("MASTER: Command queue is empty");
    return;
  }

  if (systemState != STATE_RUNNING) {
    DEBUG_PRINTLN("MASTER: Not processing command because system is not in RUNNING state");
    return;
  }

  String command = getFromQueue();

  String upperData = command;
  upperData.trim();
  upperData.toUpperCase();

  if (upperData == "ZERO") {
    processStandardCommand(upperData);
  } else if (upperData.startsWith("SPEED;")) {
    processSpeedCommand(command);
  } else {
    processCoordinateData(command);
  }
}

void PalletizerMaster::requestCommand() {
  if (!isQueueFull() && !requestNextCommand) {
    requestNextCommand = true;
    bluetoothSerial.println("NEXT");
    DEBUG_PRINTLN("MASTER: Requesting next command");
  }
}

void PalletizerMaster::clearQueue() {
  queueHead = 0;
  queueTail = 0;
  queueSize = 0;
  DEBUG_PRINTLN("MASTER: Command queue cleared");
}

void PalletizerMaster::setSystemState(SystemState newState) {
  if (systemState != newState) {
    systemState = newState;
    DEBUG_PRINTLN("MASTER: System state changed to " + String(systemState));
    sendStateUpdate();

    if (newState == STATE_RUNNING && !sequenceRunning && !waitingForCompletion && !isQueueEmpty()) {
      processNextCommand();
    }
  }
}

void PalletizerMaster::sendStateUpdate(bool send) {
  String stateStr;
  switch (systemState) {
    case STATE_IDLE: stateStr = "IDLE"; break;
    case STATE_RUNNING: stateStr = "RUNNING"; break;
    case STATE_PAUSED: stateStr = "PAUSED"; break;
    case STATE_STOPPING: stateStr = "STOPPING"; break;
    default: stateStr = "UNKNOWN"; break;
  }
  if (send) {
    bluetoothSerial.println("STATE:" + stateStr);
  }
  DEBUG_PRINTLN("MASTER→ANDROID: STATE:" + stateStr);
}