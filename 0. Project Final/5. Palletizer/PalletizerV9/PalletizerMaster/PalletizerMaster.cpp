#include "PalletizerMaster.h"

PalletizerMaster* PalletizerMaster::instance = nullptr;

PalletizerMaster::PalletizerMaster(int rxPin, int txPin, int indicatorPin)
  : rxPin(rxPin), txPin(txPin), rxIndicatorLed(2), indicatorPin(indicatorPin),
    syncSetPin(SYNC_SET_PIN), syncWaitPin(SYNC_WAIT_PIN), waitingForSync(false), waitStartTime(0),
    scriptParser(this), ledIndicator{
      DigitalOut(27, true),
      DigitalOut(14, true),
      DigitalOut(13, true),
    } {
  instance = this;
  indicatorEnabled = (indicatorPin != -1);
}

void PalletizerMaster::begin() {
  slaveCommSerial.begin(9600, SERIAL_8N1, rxPin, txPin);
  slaveSerial.begin(&slaveCommSerial);

  pinMode(syncSetPin, OUTPUT);
  pinMode(syncWaitPin, INPUT_PULLDOWN);
  digitalWrite(syncSetPin, LOW);

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
  DEBUG_PRINTLN("MASTER: Sync pins initialized - Set:" + String(syncSetPin) + " Wait:" + String(syncWaitPin));
}

void PalletizerMaster::update() {
  checkSlaveData();

  if (waitingForSync && checkSyncTimeout()) {
    DEBUG_PRINTLN("MASTER: WAIT command timeout!");
    waitingForSync = false;
  }

  if (!requestNextCommand && !isQueueFull() && !waitingForSync) {
    requestCommand();
  }

  if (indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (millis() - lastCheckTime > 50) {
      lastCheckTime = millis();

      if (checkAllSlavesCompleted()) {
        sequenceRunning = false;
        waitingForCompletion = false;
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

void PalletizerMaster::sendToSlave(const String& data) {
  slaveSerial.println(data);
}

void PalletizerMaster::setSlaveDataCallback(DataCallback callback) {
  slaveDataCallback = callback;
}

void PalletizerMaster::processCommand(const String& data) {
  if (instance) {
    instance->onCommandReceived(data);
  }
}

PalletizerMaster::SystemState PalletizerMaster::getSystemState() {
  return systemState;
}

void PalletizerMaster::checkSlaveData() {
  if (slaveSerial.available() > 0) {
    while (slaveSerial.available() > 0) {
      rxIndicatorLed.on();
      char c = slaveSerial.read();

      if (c == '\n' || c == '\r') {
        if (slavePartialBuffer.length() > 0) {
          slavePartialBuffer.trim();
          if (slaveDataCallback) {
            slaveDataCallback(slavePartialBuffer);
          }
          onSlaveData(slavePartialBuffer);
          slavePartialBuffer = "";
        }
      } else {
        slavePartialBuffer += c;
      }
      rxIndicatorLed.off();
    }
  }
}

void PalletizerMaster::onCommandReceived(const String& data) {
  DEBUG_PRINTLN("COMMAND→MASTER: " + data);
  requestNextCommand = false;

  String upperData = data;
  upperData.trim();
  upperData.toUpperCase();

  if (data.indexOf(';') != -1 || data.indexOf('{') != -1 || data.indexOf("FUNC(") != -1 || data.indexOf("CALL(") != -1) {
    processScriptCommand(data);
    return;
  }

  if (upperData.startsWith("SET(") || upperData == "WAIT") {
    processSyncCommand(upperData);
    return;
  }

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

    sendToSlave(command);
    DEBUG_PRINTLN("MASTER→SLAVE: " + command);
  } else {
    const char* slaveIds[] = { "x", "y", "z", "t", "g" };
    for (int i = 0; i < 5; i++) {
      String command = String(slaveIds[i]) + ";" + String(CMD_SETSPEED) + ";" + params;
      sendToSlave(command);
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

void PalletizerMaster::processSyncCommand(const String& command) {
  if (command.startsWith("SET(")) {
    processSetCommand(command);
  } else if (command == "WAIT") {
    processWaitCommand();
  }
}

void PalletizerMaster::processSetCommand(const String& data) {
  int startPos = data.indexOf('(');
  int endPos = data.indexOf(')');

  if (startPos != -1 && endPos != -1) {
    String value = data.substring(startPos + 1, endPos);
    int setValue = value.toInt();

    if (setValue == 1) {
      digitalWrite(syncSetPin, HIGH);
      DEBUG_PRINTLN("MASTER: SET(1) - Sync signal HIGH");
    } else if (setValue == 0) {
      digitalWrite(syncSetPin, LOW);
      DEBUG_PRINTLN("MASTER: SET(0) - Sync signal LOW");
    } else {
      DEBUG_PRINTLN("MASTER: Invalid SET value: " + value);
    }
  }
}

void PalletizerMaster::processWaitCommand() {
  DEBUG_PRINTLN("MASTER: WAIT command - blocking until sync signal HIGH");
  waitingForSync = true;
  waitStartTime = millis();

  while (waitingForSync && !checkSyncTimeout()) {
    if (digitalRead(syncWaitPin) == HIGH) {
      DEBUG_PRINTLN("MASTER: WAIT completed - sync signal received");
      waitingForSync = false;
      return;
    }
    delay(10);

    checkSlaveData();
    for (int i = 0; i < MAX_LED_INDICATOR_SIZE; i++) {
      ledIndicator[i].update();
    }
  }
}

void PalletizerMaster::processScriptCommand(const String& script) {
  DEBUG_PRINTLN("MASTER: Processing script command");
  scriptParser.parseScript(script);
}

void PalletizerMaster::sendCommandToAllSlaves(Command cmd) {
  const char* slaveIds[] = { "x", "y", "z", "t", "g" };
  for (int i = 0; i < 5; i++) {
    String command = String(slaveIds[i]) + ";" + String(cmd);
    sendToSlave(command);
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
    sendToSlave(command);
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

bool PalletizerMaster::checkSyncTimeout() {
  if (waitingForSync && (millis() - waitStartTime) > maxWaitTime) {
    return true;
  }
  return false;
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

  if (command.indexOf(';') != -1 || command.indexOf('{') != -1 || command.indexOf("FUNC(") != -1 || command.indexOf("CALL(") != -1) {
    processScriptCommand(command);
  } else if (upperData == "ZERO") {
    processStandardCommand(upperData);
  } else if (upperData.startsWith("SPEED;")) {
    processSpeedCommand(command);
  } else if (upperData.startsWith("SET(") || upperData == "WAIT") {
    processSyncCommand(upperData);
  } else {
    processCoordinateData(command);
  }
}

void PalletizerMaster::requestCommand() {
  if (!isQueueFull() && !requestNextCommand) {
    requestNextCommand = true;
    DEBUG_PRINTLN("MASTER: Ready for next command");
  }
}

void PalletizerMaster::clearQueue() {
  bool isRemoved = false;
  int retryCount = 0;
  const int maxRetry = 3;

  while (!isRemoved && retryCount < maxRetry) {
    if (LittleFS.exists(queueFilePath)) {
      isRemoved = LittleFS.remove(queueFilePath);
      if (!isRemoved) {
        DEBUG_PRINTLN("MASTER: Failed to remove queue file, retrying...");
        delay(100);
        retryCount++;
      }
    } else {
      isRemoved = true;
      break;
    }
  }

  if (!isRemoved && retryCount >= maxRetry) {
    DEBUG_PRINTLN("MASTER: Failed to remove queue file after multiple attempts");
  }

  File queueFile = LittleFS.open(queueFilePath, "w");
  if (queueFile) {
    queueFile.close();
  } else {
    DEBUG_PRINTLN("MASTER: Failed to create new queue file");
  }

  queueHead = 0;
  queueSize = 0;
  writeQueueIndex();

  DEBUG_PRINTLN("MASTER: Command queue cleared");
}

bool PalletizerMaster::initFileSystem() {
  if (!LittleFS.begin(true)) {
    return false;
  }

  if (!LittleFS.exists(queueIndexPath)) {
    File indexFile = LittleFS.open(queueIndexPath, "w");
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
  File indexFile = LittleFS.open(queueIndexPath, "w");
  if (!indexFile) {
    return false;
  }
  indexFile.println(String(queueHead));
  indexFile.println(String(queueSize));
  indexFile.close();
  return true;
}

bool PalletizerMaster::readQueueIndex() {
  File indexFile = LittleFS.open(queueIndexPath, "r");
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
  File queueFile = LittleFS.open(queueFilePath, "a");
  if (!queueFile) {
    return false;
  }
  queueFile.println(command);
  queueFile.close();
  return true;
}

String PalletizerMaster::readQueueCommandAt(int index) {
  File queueFile = LittleFS.open(queueFilePath, "r");
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
  File queueFile = LittleFS.open(queueFilePath, "r");
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

void PalletizerMaster::sendStateUpdate() {
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
  DEBUG_PRINTLN("MASTER: STATE:" + stateStr);
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