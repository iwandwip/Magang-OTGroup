#include "PalletizerMaster.h"

PalletizerMaster* PalletizerMaster::instance = nullptr;

PalletizerMaster::PalletizerMaster(int rxPin, int txPin, int indicatorPin)
  : comms(rxPin, txPin), indicatorPin(indicatorPin), ledIndicator{
      DigitalOut(27, true),
      DigitalOut(14, true),
      DigitalOut(13, true),
    } {
  instance = this;
  indicatorEnabled = (indicatorPin != -1);
}

void PalletizerMaster::begin() {
  comms.begin();
  comms.setBluetoothDataCallback(onBluetoothDataWrapper);
  comms.setSlaveDataCallback(onSlaveDataWrapper);

  if (indicatorEnabled) {
    pinMode(indicatorPin, INPUT_PULLUP);
    DEBUG_PRINTLN("MASTER: Indicator pin enabled on pin " + String(indicatorPin));
  } else {
    DEBUG_PRINTLN("MASTER: Indicator pin disabled");
  }

  if (!initFileSystem()) {
    DEBUG_PRINTLN("MASTER: Failed to initialize file system");
  } else {
    DEBUG_PRINTLN("MASTER: File system initialized");
    clearQueue();
  }

  systemState = STATE_IDLE;
  sendStateUpdate();
  DEBUG_PRINTLN("MASTER: System initialized");
}

bool PalletizerMaster::initFileSystem() {
  if (!SPIFFS.begin(true)) {
    return false;
  }

  if (!SPIFFS.exists(queueIndexPath)) {
    File indexFile = SPIFFS.open(queueIndexPath, "w");
    if (!indexFile) {
      return false;
    }
    indexFile.println("0");
    indexFile.println("0");
    indexFile.close();
  }

  readQueueIndex();
  return true;
}

bool PalletizerMaster::writeQueueIndex() {
  File indexFile = SPIFFS.open(queueIndexPath, "w");
  if (!indexFile) {
    return false;
  }
  indexFile.println(String(queueHead));
  indexFile.println(String(queueSize));
  indexFile.close();
  return true;
}

bool PalletizerMaster::readQueueIndex() {
  File indexFile = SPIFFS.open(queueIndexPath, "r");
  if (!indexFile) {
    return false;
  }

  String headStr = indexFile.readStringUntil('\n');
  String sizeStr = indexFile.readStringUntil('\n');
  indexFile.close();

  queueHead = headStr.toInt();
  queueSize = sizeStr.toInt();
  return true;
}

bool PalletizerMaster::appendToQueueFile(const String& command) {
  File queueFile = SPIFFS.open(queueFilePath, "a");
  if (!queueFile) {
    return false;
  }
  queueFile.println(command);
  queueFile.close();
  return true;
}

String PalletizerMaster::readQueueCommandAt(int index) {
  File queueFile = SPIFFS.open(queueFilePath, "r");
  if (!queueFile) {
    return "";
  }

  String command = "";
  int currentLine = 0;

  while (queueFile.available()) {
    String line = queueFile.readStringUntil('\n');
    if (currentLine == index) {
      command = line;
      break;
    }
    currentLine++;
  }

  queueFile.close();
  return command;
}

int PalletizerMaster::getQueueCount() {
  File queueFile = SPIFFS.open(queueFilePath, "r");
  if (!queueFile) {
    return 0;
  }

  int count = 0;
  while (queueFile.available()) {
    queueFile.readStringUntil('\n');
    count++;
  }

  queueFile.close();
  return count;
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

void PalletizerMaster::update() {
  comms.update();

  if (!requestNextCommand && !isQueueFull()) {
    requestCommand();
  }

  if (indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (millis() - lastCheckTime > 50) {
      lastCheckTime = millis();

      if (checkAllSlavesCompleted()) {
        sequenceRunning = false;
        waitingForCompletion = false;
        comms.sendToBluetooth("DONE");
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

  for (int i = 0; i < MAX_LED_INDICATOR_SIZE; i++) {
    ledIndicator[i].update();
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
    } else if (upperData == "END_QUEUE") {
      DEBUG_PRINTLN("MASTER: Queue loading completed");
    } else {
      bool isCoordinateCommand = upperData.indexOf('(') != -1;

      if (isCoordinateCommand) {
#if QUEUE_OPERATION_MODE == QUEUE_MODE_OVERWRITE
        clearQueue();
#endif
      }

      int startPos = 0;
      int nextPos = data.indexOf("NEXT", startPos);

      while (startPos < data.length()) {
        if (nextPos == -1) {
          String command = data.substring(startPos);
          command.trim();
          if (command.length() > 0) {
            addToQueue(command);
          }
          break;
        } else {
          String command = data.substring(startPos, nextPos);
          command.trim();
          if (command.length() > 0) {
            addToQueue(command);
          }
          startPos = nextPos + 4;
          nextPos = data.indexOf("NEXT", startPos);
        }
      }
    }
  } else if (data != "END_QUEUE") {
#if QUEUE_OPERATION_MODE == QUEUE_MODE_OVERWRITE
    if (data.indexOf('(') != -1) {
      clearQueue();
    }
#endif

    int startPos = 0;
    int nextPos = data.indexOf("NEXT", startPos);

    while (startPos < data.length()) {
      if (nextPos == -1) {
        String command = data.substring(startPos);
        command.trim();
        if (command.length() > 0) {
          addToQueue(command);
        }
        break;
      } else {
        String command = data.substring(startPos, nextPos);
        command.trim();
        if (command.length() > 0) {
          addToQueue(command);
        }
        startPos = nextPos + 4;
        nextPos = data.indexOf("NEXT", startPos);
      }
    }
  }
}

void PalletizerMaster::onSlaveData(const String& data) {
  DEBUG_PRINTLN("SLAVE→MASTER: " + data);

  if (!indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (data.indexOf("SEQUENCE COMPLETED") != -1) {
      sequenceRunning = false;
      waitingForCompletion = false;
      comms.sendToBluetooth("DONE");
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

    comms.sendToSlave(command);
    DEBUG_PRINTLN("MASTER→SLAVE: " + command);
  } else {
    const char* slaveIds[] = { "x", "y", "z", "t", "g" };
    for (int i = 0; i < 5; i++) {
      String command = String(slaveIds[i]) + ";" + String(CMD_SETSPEED) + ";" + params;
      comms.sendToSlave(command);
      DEBUG_PRINTLN("MASTER→SLAVE: " + command);
    }
  }
}

void PalletizerMaster::processCoordinateData(const String& data) {
  DEBUG_PRINTLN("MASTER: Processing coordinates");
  currentCommand = CMD_RUN;
  parseCoordinateData(data);
  sequenceRunning = true;
  waitingForCompletion = true;
  lastCheckTime = millis();
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
    comms.sendToSlave(command);
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
    comms.sendToSlave(command);
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
  if (appendToQueueFile(command)) {
    queueSize++;
    writeQueueIndex();
    DEBUG_PRINTLN("MASTER: Added command to queue: " + command + " (Queue size: " + String(queueSize) + ")");
  } else {
    DEBUG_PRINTLN("MASTER: Failed to add command to queue: " + command);
  }
}

String PalletizerMaster::getFromQueue() {
  if (isQueueEmpty()) {
    return "";
  }

  String command = readQueueCommandAt(queueHead);
  queueHead++;
  queueSize--;
  writeQueueIndex();

  DEBUG_PRINTLN("MASTER: Processing command from queue: " + command + " (Queue size: " + String(queueSize) + ")");

  return command;
}

bool PalletizerMaster::isQueueEmpty() {
  return queueSize == 0;
}

bool PalletizerMaster::isQueueFull() {
  return queueSize >= 100;
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
    comms.sendToBluetooth("NEXT");
    DEBUG_PRINTLN("MASTER: Requesting next command");
  }
}

void PalletizerMaster::clearQueue() {
  if (SPIFFS.exists(queueFilePath)) {
    SPIFFS.remove(queueFilePath);
  }

  File queueFile = SPIFFS.open(queueFilePath, "w");
  queueFile.close();

  queueHead = 0;
  queueSize = 0;
  writeQueueIndex();

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
    case STATE_IDLE:
      setOnLedIndicator(LED_RED);
      stateStr = "IDLE";
      break;
    case STATE_RUNNING:
      setOnLedIndicator(LED_GREEN);
      stateStr = "RUNNING";
      break;
    case STATE_PAUSED:
      setOnLedIndicator(LED_YELLOW);
      stateStr = "PAUSED";
      break;
    case STATE_STOPPING:
      setOnLedIndicator(LED_RED);
      stateStr = "STOPPING";
      break;
    default:
      setOnLedIndicator(LED_OFF);
      stateStr = "UNKNOWN";
      break;
  }
  if (send) {
    comms.sendToBluetooth("STATE:" + stateStr);
  }
  DEBUG_PRINTLN("MASTER→ANDROID: STATE:" + stateStr);
}

void PalletizerMaster::setOnLedIndicator(LedIndicator index) {
  for (int i = 0; i < MAX_LED_INDICATOR_SIZE; i++) {
    ledIndicator[i].off();
  }
  if (index >= MAX_LED_INDICATOR_SIZE || index == LED_OFF) {
    return;
  }
  ledIndicator[index].on();
}