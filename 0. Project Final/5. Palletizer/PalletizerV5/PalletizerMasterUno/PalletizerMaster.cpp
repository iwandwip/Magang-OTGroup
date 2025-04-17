#include "PalletizerMaster.h"

PalletizerMaster* PalletizerMaster::instance = nullptr;

PalletizerMaster::PalletizerMaster(int rxPin, int txPin, int indicatorPin)
  : commManager(rxPin, txPin),
    commandQueue(5),
    indicatorPin(indicatorPin),
    sequenceRunning(false),
    waitingForCompletion(false),
    requestNextCommand(false),
    lastCheckTime(0) {

  instance = this;
  indicatorEnabled = (indicatorPin != -1);
}

void PalletizerMaster::begin() {
  // Initialize all components
  commManager.begin();
  stateManager.begin();
  stateManager.setupLedIndicators(4, 5, 6);  // LED pins

  // Set up callbacks
  commManager.setBluetoothDataCallback([this](const String& data) {
    this->onBluetoothData(data);
  });
  commManager.setSlaveDataCallback([this](const String& data) {
    this->onSlaveData(data);
  });

  stateManager.setStateChangeCallback([this](StateManager::SystemState state, const String& stateName) {
    this->onStateChanged(state, stateName);
  });

  commandProcessor.setCommandOutCallback([this](const String& slaveId, int command, const String& params) {
    this->onCommandOut(slaveId, command, params);
  });

  commandProcessor.setStateCommandCallback([this](const String& command) {
    this->onStateCommand(command);
  });

  // Set up indicator pin if enabled
  if (indicatorEnabled) {
    pinMode(indicatorPin, INPUT_PULLUP);
    DEBUG_PRINTLN("MASTER: Indicator pin enabled on pin " + String(indicatorPin));
  } else {
    DEBUG_PRINTLN("MASTER: Indicator pin disabled");
  }

  // Set initial state
  stateManager.setState(StateManager::STATE_IDLE);

  DEBUG_PRINTLN("MASTER: System initialized");
}

void PalletizerMaster::update() {
  // Update all components
  commManager.update();
  stateManager.update();

  // Request next command if needed
  if (commandQueue.size() < 4 && !requestNextCommand && !commandQueue.isFull()) {
    requestCommand();
  }

  // Check if all slaves completed their sequences
  if (indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (millis() - lastCheckTime > 50) {
      lastCheckTime = millis();

      if (checkAllSlavesCompleted()) {
        sequenceRunning = false;
        waitingForCompletion = false;
        commManager.sendToBluetooth("DONE");
        DEBUG_PRINTLN("MASTER: All slaves completed sequence");

        if (!commandQueue.isEmpty() && stateManager.getState() == StateManager::STATE_RUNNING) {
          DEBUG_PRINTLN("MASTER: Processing next command from queue");
          processNextCommand();
        } else if (commandQueue.isEmpty() && stateManager.getState() == StateManager::STATE_RUNNING) {
          stateManager.setState(StateManager::STATE_IDLE);
        }
      }
    }
  }

  // Handle stopping state
  if (stateManager.getState() == StateManager::STATE_STOPPING && !sequenceRunning && !waitingForCompletion) {
    stateManager.setState(StateManager::STATE_IDLE);
  }
}

void PalletizerMaster::onBluetoothData(const String& data) {
  requestNextCommand = false;

  String upperData = data;
  upperData.trim();
  upperData.toUpperCase();

  // Check if it's a system state command (IDLE, PLAY, etc)
  if (upperData == "IDLE" || upperData == "PLAY" || upperData == "PAUSE" || upperData == "STOP") {
    commandProcessor.processCommand(upperData);
    return;
  }

  // Process or queue command based on system state
  if (!sequenceRunning && !waitingForCompletion) {
    if (upperData == "ZERO") {
      commandProcessor.processCommand(upperData);
      sequenceRunning = true;
      waitingForCompletion = indicatorEnabled;
      lastCheckTime = millis();
    } else if (upperData.startsWith("SPEED;")) {
      commandProcessor.processCommand(data);
    } else if (upperData != "END_QUEUE") {
      if (stateManager.getState() == StateManager::STATE_RUNNING) {
        commandProcessor.processCommand(data);
        sequenceRunning = true;
        waitingForCompletion = true;
        lastCheckTime = millis();
      } else {
        if (!commandQueue.isFull()) {
          commandQueue.add(data);
          DEBUG_PRINTLN("MASTER: Added command to queue: " + data + " (Queue size: " + String(commandQueue.size()) + ")");
        } else {
          DEBUG_PRINTLN("MASTER: Command queue is full, dropping command: " + data);
        }
      }
    }
  } else if (data != "END_QUEUE") {
    if (!commandQueue.isFull()) {
      commandQueue.add(data);
      DEBUG_PRINTLN("MASTER: Added command to queue: " + data + " (Queue size: " + String(commandQueue.size()) + ")");
    } else {
      DEBUG_PRINTLN("MASTER: Command queue is full, dropping command: " + data);
    }
  }
}

void PalletizerMaster::onSlaveData(const String& data) {
  if (!indicatorEnabled && waitingForCompletion && sequenceRunning) {
    if (data.indexOf("SEQUENCE COMPLETED") != -1) {
      sequenceRunning = false;
      waitingForCompletion = false;
      commManager.sendToBluetooth("DONE");
      DEBUG_PRINTLN("MASTER: All slaves completed sequence (based on message)");

      if (!commandQueue.isEmpty() && stateManager.getState() == StateManager::STATE_RUNNING) {
        DEBUG_PRINTLN("MASTER: Processing next command from queue");
        processNextCommand();
      } else if (commandQueue.isEmpty() && stateManager.getState() == StateManager::STATE_RUNNING) {
        stateManager.setState(StateManager::STATE_IDLE);
      } else if (stateManager.getState() == StateManager::STATE_STOPPING) {
        commandQueue.clear();
        stateManager.setState(StateManager::STATE_IDLE);
      }
    }
  }
}

void PalletizerMaster::onStateChanged(StateManager::SystemState state, const String& stateName) {
  DEBUG_PRINTLN("MASTER: System state changed to " + stateName);
  commManager.sendToBluetooth("STATE:" + stateName);

  if (state == StateManager::STATE_RUNNING && !sequenceRunning && !waitingForCompletion && !commandQueue.isEmpty()) {
    processNextCommand();
  }
}

void PalletizerMaster::onCommandOut(const String& slaveId, int command, const String& params) {
  String slaveCommand = slaveId + ";" + String(command);
  if (params.length() > 0) {
    slaveCommand += ";" + params;
  }
  commManager.sendToSlave(slaveCommand);
}

void PalletizerMaster::onStateCommand(const String& command) {
  if (command == "IDLE") {
    if (stateManager.getState() == StateManager::STATE_RUNNING || stateManager.getState() == StateManager::STATE_PAUSED) {
      if (sequenceRunning) {
        stateManager.setState(StateManager::STATE_STOPPING);
      } else {
        commandQueue.clear();
        stateManager.setState(StateManager::STATE_IDLE);
      }
    } else {
      stateManager.setState(StateManager::STATE_IDLE);
    }
  } else if (command == "PLAY") {
    stateManager.setState(StateManager::STATE_RUNNING);
  } else if (command == "PAUSE") {
    stateManager.setState(StateManager::STATE_PAUSED);
  } else if (command == "STOP") {
    if (sequenceRunning) {
      stateManager.setState(StateManager::STATE_STOPPING);
    } else {
      commandQueue.clear();
      stateManager.setState(StateManager::STATE_IDLE);
    }
  }
}

void PalletizerMaster::processNextCommand() {
  if (commandQueue.isEmpty()) {
    DEBUG_PRINTLN("MASTER: Command queue is empty");
    return;
  }

  if (stateManager.getState() != StateManager::STATE_RUNNING) {
    DEBUG_PRINTLN("MASTER: Not processing command because system is not in RUNNING state");
    return;
  }

  String command = commandQueue.get();
  DEBUG_PRINTLN("MASTER: Processing command from queue: " + command + " (Queue size: " + String(commandQueue.size()) + ")");

  String upperData = command;
  upperData.trim();
  upperData.toUpperCase();

  if (upperData == "ZERO") {
    commandProcessor.processCommand(upperData);
    sequenceRunning = true;
    waitingForCompletion = indicatorEnabled;
    lastCheckTime = millis();
  } else if (upperData.startsWith("SPEED;")) {
    commandProcessor.processCommand(command);
  } else {
    commandProcessor.processCommand(command);
    sequenceRunning = true;
    waitingForCompletion = true;
    lastCheckTime = millis();
  }
}

void PalletizerMaster::requestCommand() {
  if (!commandQueue.isFull() && !requestNextCommand) {
    requestNextCommand = true;
    commManager.sendToBluetooth("NEXT");
    DEBUG_PRINTLN("MASTER: Requesting next command");
  }
}

bool PalletizerMaster::checkAllSlavesCompleted() {
  if (!indicatorEnabled) {
    return false;
  }
  return digitalRead(indicatorPin) == HIGH;
}