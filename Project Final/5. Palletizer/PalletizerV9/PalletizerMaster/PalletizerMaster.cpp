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
    loadTimeoutConfig();
  }

  systemState = STATE_IDLE;
  sendStateUpdate();
  DEBUG_PRINTLN("MASTER: System initialized");
  DEBUG_PRINTLN("MASTER: Sync pins initialized - Set:" + String(syncSetPin) + " Wait:" + String(syncWaitPin));
}

void PalletizerMaster::update() {
  safeYield();
  checkSlaveData();

  if (waitingForSync) {
    if (digitalRead(syncWaitPin) == HIGH) {
      DEBUG_PRINTLN("MASTER: WAIT completed - sync signal received");
      waitingForSync = false;
      updateTimeoutStats(true);

      if (!isQueueEmpty() && systemState == STATE_RUNNING) {
        processNextCommand();
      }
    } else if (checkSyncTimeout()) {
      handleWaitTimeout();
    }

    safeYield();
    return;
  }

  if (!requestNextCommand && !isQueueFull() && !waitingForSync && !scriptProcessing) {
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
      safeYield();
    }
  }

  if (systemState == STATE_STOPPING && !sequenceRunning && !waitingForCompletion) {
    setSystemState(STATE_IDLE);
  }

  for (int i = 0; i < MAX_LED_INDICATOR_SIZE; i++) {
    ledIndicator[i].update();
  }

  safeYield();
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

void PalletizerMaster::setTimeoutConfig(const TimeoutConfig& config) {
  timeoutConfig = config;
  if (config.saveToFile) {
    saveTimeoutConfig();
  }
  DEBUG_PRINTLN("MASTER: Timeout config updated - timeout:" + String(config.maxWaitTime) + "ms strategy:" + String(config.strategy));
}

PalletizerMaster::TimeoutConfig PalletizerMaster::getTimeoutConfig() {
  return timeoutConfig;
}

void PalletizerMaster::setWaitTimeout(unsigned long timeoutMs) {
  timeoutConfig.maxWaitTime = timeoutMs;
  if (timeoutConfig.saveToFile) {
    saveTimeoutConfig();
  }
  DEBUG_PRINTLN("MASTER: Wait timeout set to " + String(timeoutMs) + "ms");
}

void PalletizerMaster::setTimeoutStrategy(WaitTimeoutStrategy strategy) {
  timeoutConfig.strategy = strategy;
  if (timeoutConfig.saveToFile) {
    saveTimeoutConfig();
  }
  DEBUG_PRINTLN("MASTER: Timeout strategy set to " + String(strategy));
}

void PalletizerMaster::setMaxTimeoutWarning(int maxWarning) {
  timeoutConfig.maxTimeoutWarning = maxWarning;
  if (timeoutConfig.saveToFile) {
    saveTimeoutConfig();
  }
  DEBUG_PRINTLN("MASTER: Max timeout warning set to " + String(maxWarning));
}

PalletizerMaster::TimeoutStats PalletizerMaster::getTimeoutStats() {
  return timeoutStats;
}

void PalletizerMaster::clearTimeoutStats() {
  timeoutStats.totalTimeouts = 0;
  timeoutStats.successfulWaits = 0;
  timeoutStats.lastTimeoutTime = 0;
  timeoutStats.totalWaitTime = 0;
  timeoutStats.currentRetryCount = 0;
  DEBUG_PRINTLN("MASTER: Timeout statistics cleared");
}

float PalletizerMaster::getTimeoutSuccessRate() {
  int totalAttempts = timeoutStats.totalTimeouts + timeoutStats.successfulWaits;
  if (totalAttempts == 0) return 100.0;
  return (float(timeoutStats.successfulWaits) / float(totalAttempts)) * 100.0;
}

bool PalletizerMaster::saveTimeoutConfig() {
  File configFile = LittleFS.open(timeoutConfigPath, "w");
  if (!configFile) {
    DEBUG_PRINTLN("MASTER: Failed to save timeout config");
    return false;
  }

  String jsonConfig = "{";
  jsonConfig += "\"maxWaitTime\":" + String(timeoutConfig.maxWaitTime) + ",";
  jsonConfig += "\"strategy\":" + String(timeoutConfig.strategy) + ",";
  jsonConfig += "\"maxTimeoutWarning\":" + String(timeoutConfig.maxTimeoutWarning) + ",";
  jsonConfig += "\"autoRetryCount\":" + String(timeoutConfig.autoRetryCount) + ",";
  jsonConfig += "\"saveToFile\":" + String(timeoutConfig.saveToFile ? "true" : "false");
  jsonConfig += "}";

  configFile.print(jsonConfig);
  ensureFileIsClosed(configFile);
  DEBUG_PRINTLN("MASTER: Timeout config saved");
  return true;
}

bool PalletizerMaster::loadTimeoutConfig() {
  if (!LittleFS.exists(timeoutConfigPath)) {
    DEBUG_PRINTLN("MASTER: No timeout config file found, using defaults");
    return false;
  }

  File configFile = LittleFS.open(timeoutConfigPath, "r");
  if (!configFile) {
    DEBUG_PRINTLN("MASTER: Failed to load timeout config");
    return false;
  }

  String jsonConfig = configFile.readString();
  ensureFileIsClosed(configFile);

  int maxWaitTimePos = jsonConfig.indexOf("\"maxWaitTime\":");
  int strategyPos = jsonConfig.indexOf("\"strategy\":");
  int maxTimeoutWarningPos = jsonConfig.indexOf("\"maxTimeoutWarning\":");
  int autoRetryCountPos = jsonConfig.indexOf("\"autoRetryCount\":");
  int saveToFilePos = jsonConfig.indexOf("\"saveToFile\":");

  if (maxWaitTimePos != -1) {
    int valueStart = jsonConfig.indexOf(":", maxWaitTimePos) + 1;
    int valueEnd = jsonConfig.indexOf(",", valueStart);
    if (valueEnd == -1) valueEnd = jsonConfig.indexOf("}", valueStart);
    timeoutConfig.maxWaitTime = jsonConfig.substring(valueStart, valueEnd).toInt();
  }

  if (strategyPos != -1) {
    int valueStart = jsonConfig.indexOf(":", strategyPos) + 1;
    int valueEnd = jsonConfig.indexOf(",", valueStart);
    if (valueEnd == -1) valueEnd = jsonConfig.indexOf("}", valueStart);
    timeoutConfig.strategy = (WaitTimeoutStrategy)jsonConfig.substring(valueStart, valueEnd).toInt();
  }

  if (maxTimeoutWarningPos != -1) {
    int valueStart = jsonConfig.indexOf(":", maxTimeoutWarningPos) + 1;
    int valueEnd = jsonConfig.indexOf(",", valueStart);
    if (valueEnd == -1) valueEnd = jsonConfig.indexOf("}", valueStart);
    timeoutConfig.maxTimeoutWarning = jsonConfig.substring(valueStart, valueEnd).toInt();
  }

  if (autoRetryCountPos != -1) {
    int valueStart = jsonConfig.indexOf(":", autoRetryCountPos) + 1;
    int valueEnd = jsonConfig.indexOf(",", valueStart);
    if (valueEnd == -1) valueEnd = jsonConfig.indexOf("}", valueStart);
    timeoutConfig.autoRetryCount = jsonConfig.substring(valueStart, valueEnd).toInt();
  }

  if (saveToFilePos != -1) {
    int valueStart = jsonConfig.indexOf(":", saveToFilePos) + 1;
    int valueEnd = jsonConfig.indexOf("}", valueStart);
    String value = jsonConfig.substring(valueStart, valueEnd);
    value.trim();
    timeoutConfig.saveToFile = (value == "true");
  }

  DEBUG_PRINTLN("MASTER: Timeout config loaded - timeout:" + String(timeoutConfig.maxWaitTime) + "ms");
  return true;
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
      safeYield();
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

  if (!sequenceRunning && !waitingForCompletion && !scriptProcessing) {
    if (upperData == "ZERO") {
      processStandardCommand(upperData);
    } else if (upperData.startsWith("SPEED;")) {
      processSpeedCommand(data);
    } else if (upperData == "END_QUEUE") {
      DEBUG_PRINTLN("MASTER: Queue loading completed");
      scriptProcessing = false;
    } else {
      if (shouldClearQueue(data)) {
        if (!queueClearRequested) {
          clearQueue();
          queueClearRequested = true;
        }
      }

      processCommandsBatch(data);
    }
  } else if (data != "END_QUEUE") {
    if (shouldClearQueue(data)) {
      if (!queueClearRequested) {
        clearQueue();
        queueClearRequested = true;
      }
    }

    processCommandsBatch(data);
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
      safeYield();
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
    if (isQueueEmpty()) {
      loadCommandsFromFile();
    }

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
  DEBUG_PRINTLN("MASTER: WAIT command - waiting for sync signal HIGH");
  waitingForSync = true;
  waitStartTime = millis();
  waitStartTimeForStats = millis();
}

void PalletizerMaster::processScriptCommand(const String& script) {
  DEBUG_PRINTLN("MASTER: Processing script command");
  scriptProcessing = true;
  queueClearRequested = false;
  scriptParser.parseScript(script);
  scriptProcessing = false;
}

void PalletizerMaster::sendCommandToAllSlaves(Command cmd) {
  const char* slaveIds[] = { "x", "y", "z", "t", "g" };
  for (int i = 0; i < 5; i++) {
    String command = String(slaveIds[i]) + ";" + String(cmd);
    sendToSlave(command);
    DEBUG_PRINTLN("MASTER→SLAVE: " + command);
    safeYield();
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
    safeYield();
  }
}

bool PalletizerMaster::checkAllSlavesCompleted() {
  if (!indicatorEnabled) {
    return false;
  }
  return digitalRead(indicatorPin) == HIGH;
}

bool PalletizerMaster::checkSyncTimeout() {
  return waitingForSync && (millis() - waitStartTime) > timeoutConfig.maxWaitTime;
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
  queueClearRequested = false;

  if (LittleFS.exists(queueFilePath)) {
    File testFile = LittleFS.open(queueFilePath, "r");
    if (testFile) {
      ensureFileIsClosed(testFile);
      delay(10);
    }

    bool removed = LittleFS.remove(queueFilePath);
    if (!removed) {
      DEBUG_PRINTLN("MASTER: Warning - could not remove queue file");
    }
  }

  File queueFile = LittleFS.open(queueFilePath, "w");
  if (queueFile) {
    ensureFileIsClosed(queueFile);
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
    ensureFileIsClosed(indexFile);
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
  ensureFileIsClosed(indexFile);
  return true;
}

bool PalletizerMaster::readQueueIndex() {
  File indexFile = LittleFS.open(queueIndexPath, "r");
  if (!indexFile) {
    return false;
  }

  String headStr = indexFile.readStringUntil('\n');
  String sizeStr = indexFile.readStringUntil('\n');
  ensureFileIsClosed(indexFile);

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
  ensureFileIsClosed(queueFile);
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
    safeYield();
  }

  ensureFileIsClosed(queueFile);
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
    safeYield();
  }

  ensureFileIsClosed(queueFile);
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

void PalletizerMaster::loadCommandsFromFile() {
  if (!LittleFS.exists(queueFilePath)) {
    DEBUG_PRINTLN("MASTER: No saved commands found");
    return;
  }

  File file = LittleFS.open(queueFilePath, "r");
  if (!file) {
    DEBUG_PRINTLN("MASTER: Failed to open queue file");
    return;
  }

  DEBUG_PRINTLN("MASTER: Loading commands from file...");

  String allCommands = file.readString();
  ensureFileIsClosed(file);

  clearQueue();

  if (allCommands.length() > 0) {
    processCommandsBatch(allCommands);
    processCommand("END_QUEUE");
    DEBUG_PRINTLN("MASTER: Commands loaded from file successfully");
  } else {
    DEBUG_PRINTLN("MASTER: No valid commands found in file");
  }
}

void PalletizerMaster::handleWaitTimeout() {
  updateTimeoutStats(false);

  switch (timeoutConfig.strategy) {
    case TIMEOUT_SKIP_CONTINUE:
      DEBUG_PRINTLN("MASTER: WAIT timeout #" + String(timeoutStats.totalTimeouts) + " - skipping and continuing");
      waitingForSync = false;

      if (timeoutStats.totalTimeouts >= timeoutConfig.maxTimeoutWarning) {
        DEBUG_PRINTLN("MASTER: WARNING - Frequent WAIT timeouts detected!");

        if (timeoutStats.totalTimeouts >= timeoutConfig.maxTimeoutWarning * 2) {
          DEBUG_PRINTLN("MASTER: Too many timeouts - auto pausing");
          setSystemState(STATE_PAUSED);
          return;
        }
      }

      if (!isQueueEmpty() && systemState == STATE_RUNNING) {
        processNextCommand();
      }
      break;

    case TIMEOUT_PAUSE_SYSTEM:
      DEBUG_PRINTLN("MASTER: WAIT timeout - pausing system for user intervention");
      waitingForSync = false;
      setSystemState(STATE_PAUSED);
      break;

    case TIMEOUT_ABORT_RESET:
      DEBUG_PRINTLN("MASTER: WAIT timeout - aborting sequence and resetting");
      waitingForSync = false;
      clearQueue();
      setSystemState(STATE_IDLE);
      break;

    case TIMEOUT_RETRY_BACKOFF:
      if (timeoutStats.currentRetryCount < timeoutConfig.autoRetryCount) {
        timeoutStats.currentRetryCount++;
        DEBUG_PRINTLN("MASTER: WAIT timeout - retry " + String(timeoutStats.currentRetryCount) + "/" + String(timeoutConfig.autoRetryCount));
        waitStartTime = millis();
      } else {
        DEBUG_PRINTLN("MASTER: WAIT timeout - max retries reached, pausing");
        timeoutStats.currentRetryCount = 0;
        waitingForSync = false;
        setSystemState(STATE_PAUSED);
      }
      break;
  }
}

void PalletizerMaster::resetTimeoutCounter() {
  if (timeoutStats.totalTimeouts > 0) {
    DEBUG_PRINTLN("MASTER: WAIT successful - resetting timeout counter");
    timeoutStats.currentRetryCount = 0;
  }
}

void PalletizerMaster::updateTimeoutStats(bool success) {
  if (success) {
    timeoutStats.successfulWaits++;
    timeoutStats.totalWaitTime += millis() - waitStartTimeForStats;
    resetTimeoutCounter();
  } else {
    timeoutStats.totalTimeouts++;
    timeoutStats.lastTimeoutTime = millis();
  }
}

void PalletizerMaster::processCommandsBatch(const String& commands) {
  int startPos = 0;
  int nextPos = commands.indexOf("NEXT", startPos);

  while (startPos < commands.length()) {
    safeYield();

    if (nextPos == -1) {
      String command = commands.substring(startPos);
      command.trim();
      if (command.length() > 0) {
        addToQueue(command);
      }
      break;
    } else {
      String command = commands.substring(startPos, nextPos);
      command.trim();
      if (command.length() > 0) {
        addToQueue(command);
      }
      startPos = nextPos + 4;
      nextPos = commands.indexOf("NEXT", startPos);
    }
  }
}

void PalletizerMaster::safeYield() {
  unsigned long currentTime = millis();
  if (currentTime - lastYieldTime >= 5) {
    yield();
    lastYieldTime = currentTime;
  }
}

bool PalletizerMaster::shouldClearQueue(const String& data) {
  bool isCoordinateCommand = data.indexOf('(') != -1;
  return isCoordinateCommand && QUEUE_OPERATION_MODE == QUEUE_MODE_OVERWRITE;
}

void PalletizerMaster::ensureFileIsClosed(File& file) {
  if (file) {
    file.close();
  }
}