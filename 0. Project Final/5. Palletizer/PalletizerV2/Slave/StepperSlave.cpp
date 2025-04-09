#include "StepperSlave.h"

StepperSlave* StepperSlave::instance = nullptr;

StepperSlave::StepperSlave(char id, int rxPin, int txPin, int clkPin, int cwPin, int enPin, int sensorPin)
  : slaveId(id),
    masterCommSerial(rxPin, txPin),
    stepper(AccelStepper::DRIVER, clkPin, cwPin),
    enPin(enPin),
    sensorPin(sensorPin) {
  instance = this;
}

void StepperSlave::begin() {
  Serial.begin(9600);
  masterCommSerial.begin(9600);

  masterSerial.begin(&Serial);
  debugSerial.begin(&Serial);

  masterSerial.setDataCallback(onMasterDataWrapper);

  pinMode(enPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  digitalWrite(enPin, LOW);

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);

  debugSerial.println("SLAVE " + String(slaveId) + ": System initialized");
}

void StepperSlave::update() {
  masterSerial.checkCallback();
  handleMotion();

  if (!isPaused && !isDelaying) {
    stepper.run();
  }

  checkPositionReached();
}

void StepperSlave::onMasterDataWrapper(const String& data) {
  if (instance) {
    instance->onMasterData(data);
  }
}

void StepperSlave::onMasterData(const String& data) {
  debugSerial.println("MASTER→SLAVE: " + data);
  processCommand(data);
}

void StepperSlave::processCommand(const String& data) {
  int firstSeparator = data.indexOf(';');
  if (firstSeparator == -1) return;

  String slaveIdStr = data.substring(0, firstSeparator);
  slaveIdStr.toLowerCase();

  if (slaveIdStr != String(slaveId) && slaveIdStr != "all") return;

  int secondSeparator = data.indexOf(';', firstSeparator + 1);
  int cmdCode = (secondSeparator == -1)
                  ? data.substring(firstSeparator + 1).toInt()
                  : data.substring(firstSeparator + 1, secondSeparator).toInt();

  debugSerial.println("SLAVE " + String(slaveId) + ": Processing command " + String(cmdCode));

  switch (cmdCode) {
    case CMD_ZERO:
      handleZeroCommand();
      break;
    case CMD_PAUSE:
      handlePauseCommand();
      break;
    case CMD_RESUME:
      handleResumeCommand();
      break;
    case CMD_RESET:
      handleResetCommand();
      break;
    case CMD_START:
      if (secondSeparator != -1) {
        handleMoveCommand(data.substring(secondSeparator + 1));
      }
      break;
    default:
      debugSerial.println("SLAVE " + String(slaveId) + ": Unknown command " + String(cmdCode));
      break;
  }
}

void StepperSlave::performHoming() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Starting homing sequence");

  float originalSpeed = stepper.maxSpeed();
  float originalAccel = stepper.acceleration();

  stepper.setMaxSpeed(HOMING_SPEED);
  stepper.setAcceleration(HOMING_ACCEL);

  long distance = 0;
  int count = 20000;

  if (digitalRead(sensorPin) == HIGH) {
    debugSerial.println("SLAVE " + String(slaveId) + ": Already in sensor area, moving out first");

    stepper.move(count);
    do {
      stepper.run();
    } while (digitalRead(sensorPin) == HIGH && stepper.distanceToGo() != 0);

    if (stepper.distanceToGo() != 0) {
      stepper.stop();
      stepper.setCurrentPosition(stepper.currentPosition());

      debugSerial.println("SLAVE " + String(slaveId) + ": Moving back to sensor");
      stepper.move(-count);
      do {
        stepper.run();
      } while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0);
    }
  } else {
    debugSerial.println("SLAVE " + String(slaveId) + ": Outside sensor area, moving to sensor");

    stepper.move(-count);
    do {
      stepper.run();
    } while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0);
  }

  stepper.stop();
  debugSerial.println("SLAVE " + String(slaveId) + ": Sensor detected");

  distance = stepper.distanceToGo();
  stepper.runToPosition();

  debugSerial.println("SLAVE " + String(slaveId) + ": Correcting overshot by " + String(distance) + " steps");
  stepper.move(-distance);
  stepper.runToPosition();

  debugSerial.println("SLAVE " + String(slaveId) + ": Setting home position (0)");
  stepper.setCurrentPosition(0);

  stepper.setMaxSpeed(originalSpeed);
  stepper.setAcceleration(originalAccel);

  debugSerial.println("SLAVE " + String(slaveId) + ": Homing completed");
}

void StepperSlave::handleZeroCommand() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Executing ZERO (Homing)");
  isPaused = false;
  isDelaying = false;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;

  performHoming();

  sendFeedback("ZERO DONE");
}

void StepperSlave::handlePauseCommand() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Executing PAUSE");
  isPaused = true;
  sendFeedback("PAUSE DONE");
}

void StepperSlave::handleResumeCommand() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Executing RESUME");
  isPaused = false;
  sendFeedback("RESUME DONE");
}

void StepperSlave::handleResetCommand() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Executing RESET");
  isPaused = false;
  isDelaying = false;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;
  stepper.stop();
  sendFeedback("RESET DONE");
}

void StepperSlave::startNextMotion() {
  if (currentMotionIndex < queuedMotionsCount) {
    if (motionQueue[currentMotionIndex].delayBeforeMove > 0) {
      isDelaying = true;
      delayStartTime = millis();
      sendFeedback("DELAYING");
    } else if (motionQueue[currentMotionIndex].isDelayOnly) {
      currentMotionIndex++;
      startNextMotion();
    } else {
      stepper.setMaxSpeed(motionQueue[currentMotionIndex].speed);
      stepper.moveTo(motionQueue[currentMotionIndex].position);
      sendFeedback("MOVING");
    }
  }
}

void StepperSlave::handleMotion() {
  if (queuedMotionsCount == 0) return;

  if (isDelaying) {
    if (millis() - delayStartTime >= motionQueue[currentMotionIndex].delayBeforeMove) {
      isDelaying = false;

      if (motionQueue[currentMotionIndex].isDelayOnly) {
        currentMotionIndex++;
        startNextMotion();
      } else {
        stepper.setMaxSpeed(motionQueue[currentMotionIndex].speed);
        stepper.moveTo(motionQueue[currentMotionIndex].position);
        sendFeedback("MOVING");
      }
    }
    return;
  }

  if (stepper.distanceToGo() == 0 && !stepper.isRunning() && !isDelaying && !isPaused) {
    if (currentMotionIndex < queuedMotionsCount - 1) {
      currentMotionIndex++;
      startNextMotion();
    } else if (currentMotionIndex == queuedMotionsCount - 1 && !hasReportedCompletion) {
      sendFeedback("SEQUENCE COMPLETED");
      hasReportedCompletion = true;
    }
  }
}

void StepperSlave::parsePositionSequence(const String& params) {
  debugSerial.println("SLAVE " + String(slaveId) + ": Parsing position sequence: " + params);

  int semicolonPos = -1;
  int startPos = 0;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;

  do {
    semicolonPos = params.indexOf(';', startPos);
    String param = (semicolonPos == -1) ? params.substring(startPos) : params.substring(startPos, semicolonPos);
    param.trim();

    debugSerial.println("SLAVE " + String(slaveId) + ": Parsing parameter: " + param);

    if (queuedMotionsCount < MAX_MOTIONS) {
      motionQueue[queuedMotionsCount].speed = MAX_SPEED;
      motionQueue[queuedMotionsCount].completed = false;

      if (param.startsWith("d")) {
        unsigned long delayValue = param.substring(1).toInt();
        motionQueue[queuedMotionsCount].delayBeforeMove = delayValue;
        motionQueue[queuedMotionsCount].position = 0;
        motionQueue[queuedMotionsCount].isDelayOnly = true;

        debugSerial.println("SLAVE " + String(slaveId) + ": Queueing delay " + String(queuedMotionsCount) + " - Delay: " + String(delayValue) + "ms");
      } else {
        long position = param.toInt();
        motionQueue[queuedMotionsCount].position = position;
        motionQueue[queuedMotionsCount].delayBeforeMove = 0;
        motionQueue[queuedMotionsCount].isDelayOnly = false;

        debugSerial.println("SLAVE " + String(slaveId) + ": Queueing position " + String(queuedMotionsCount) + " - Pos: " + String(position));
      }

      queuedMotionsCount++;
    }

    startPos = semicolonPos + 1;
  } while (semicolonPos != -1 && startPos < params.length() && queuedMotionsCount < MAX_MOTIONS);

  debugSerial.println("SLAVE " + String(slaveId) + ": Total queued motions: " + String(queuedMotionsCount));

  if (queuedMotionsCount > 0) {
    hasReportedCompletion = false;
    startNextMotion();
  }
}

void StepperSlave::handleMoveCommand(const String& params) {
  parsePositionSequence(params);
  hasReportedCompletion = false;
}

void StepperSlave::sendFeedback(const String& message) {
  String feedback = String(slaveId) + ";" + message;
  masterSerial.println(feedback);
  debugSerial.println("SLAVE→MASTER: " + feedback);
}

void StepperSlave::checkPositionReached() {
  static unsigned long lastReportTime = 0;
  static long lastReportedPosition = -999999;

  if ((millis() - lastReportTime > 2000) || (abs(stepper.currentPosition() - lastReportedPosition) > 100)) {
    if (stepper.isRunning() || abs(stepper.currentPosition() - lastReportedPosition) > 10) {
      lastReportTime = millis();
      lastReportedPosition = stepper.currentPosition();

      String positionUpdate = "POS:" + String(stepper.currentPosition()) + " TARGET:" + String(stepper.targetPosition());
      sendFeedback(positionUpdate);
    }
  }
}