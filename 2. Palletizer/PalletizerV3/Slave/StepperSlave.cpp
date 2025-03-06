#include "Arduino.h"
#include "StepperSlave.h"

StepperSlave* StepperSlave::instance = nullptr;

StepperSlave::StepperSlave(char id, int rxPin, int txPin, int clkPin, int cwPin, int enPin, int sensorPin,
                           int brakePin, bool invertBrakeLogic, int indicatorPin)
  : slaveId(id),
    masterCommSerial(rxPin, txPin),
    stepper(AccelStepper::DRIVER, clkPin, cwPin),
    enPin(enPin),
    sensorPin(sensorPin),
    brakePin(brakePin),
    indicatorPin(indicatorPin),
    invertBrakeLogic(invertBrakeLogic) {
  instance = this;
}

void StepperSlave::begin() {
  Serial.begin(9600);
  masterCommSerial.begin(9600);

  masterSerial.begin(&Serial);
  debugSerial.begin(&Serial);

  masterSerial.setDataCallback(onMasterDataWrapper);

  // pinMode(enPin, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  // digitalWrite(enPin, LOW);

  if (brakePin != -1) {
    pinMode(brakePin, OUTPUT);
    setBrake(true);
  }

  // Initialize indicator pin
  if (indicatorPin != -1) {
    pinMode(indicatorPin, OUTPUT);
    setIndicator(false);  // Start with LOW (inactive)
  }

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);
  stepper.setCurrentPosition(0);
  stepper.setMinPulseWidth(100);

  debugSerial.println("SLAVE " + String(slaveId) + ": System initialized");
}

void StepperSlave::update() {
  masterSerial.checkCallback();
  handleMotion();

  if (!isPaused && !isDelaying) {
    if (stepper.isRunning()) {
      setBrake(false);
    }
    stepper.run();
    if (!stepper.isRunning() && stepper.distanceToGo() == 0) {
      setBrake(true);
    }
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
    case CMD_SETSPEED:
      if (secondSeparator != -1) {
        handleSetSpeedCommand(data.substring(secondSeparator + 1));
      }
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

void StepperSlave::handleSetSpeedCommand(const String& params) {
  float newSpeed = params.toFloat();
  if (newSpeed > 0) {
    debugSerial.print("SLAVE ");
    debugSerial.print(slaveId);
    debugSerial.print(": Setting speed to ");
    debugSerial.println(newSpeed);

    MAX_SPEED = newSpeed;
    stepper.setMaxSpeed(MAX_SPEED);

    ACCELERATION = MAX_SPEED * SPEED_RATIO;
    stepper.setAcceleration(ACCELERATION);

    sendFeedback("SPEED SET TO " + String(MAX_SPEED));
  } else {
    debugSerial.print("SLAVE ");
    debugSerial.print(slaveId);
    debugSerial.println(": Invalid speed value");
    sendFeedback("INVALID SPEED VALUE");
  }
}

void StepperSlave::performHoming() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Starting homing sequence");
  setIndicator(true);

  float originalSpeed = stepper.maxSpeed();
  float originalAccel = stepper.acceleration();

  stepper.setMaxSpeed(HOMING_SPEED);
  stepper.setAcceleration(HOMING_ACCEL);
  setBrake(false);

  long distance = 0;
  int count = 20000;

  if (digitalRead(sensorPin) == HIGH) {
    debugSerial.println("SLAVE " + String(slaveId) + ": Already in sensor area, moving out first");
    stepper.move(count);
    while (digitalRead(sensorPin) == HIGH && stepper.distanceToGo() != 0) {
      stepper.run();
      Serial.print("| sensorPin: ");
      Serial.print(digitalRead(sensorPin));
      Serial.println();
    }

    if (stepper.distanceToGo() != 0) {
      stepper.stop();
      stepper.setCurrentPosition(stepper.currentPosition());
      debugSerial.println("SLAVE " + String(slaveId) + ": Moving back to sensor");
      stepper.move(-count);
      while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0) {
        stepper.run();
        Serial.print("| sensorPin: ");
        Serial.print(digitalRead(sensorPin));
        Serial.println();
      }
    }
  } else {
    debugSerial.println("SLAVE " + String(slaveId) + ": Outside sensor area, moving to sensor");
    stepper.move(-count);
    while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0) {
      stepper.run();
      Serial.print("| sensorPin: ");
      Serial.print(digitalRead(sensorPin));
      Serial.println();
    }
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
  setBrake(true);

  setIndicator(false);

  debugSerial.println("SLAVE " + String(slaveId) + ": Homing completed");
}

void StepperSlave::handleZeroCommand() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Executing ZERO (Homing)");
  isPaused = false;
  isDelaying = false;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;

  // Turn off indicator during command reset
  setIndicator(false);

  performHoming();

  sendFeedback("ZERO DONE");
}

void StepperSlave::handlePauseCommand() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Executing PAUSE");
  isPaused = true;
  setBrake(true);

  // Keep indicator ON when paused - sequence isn't completed yet

  sendFeedback("PAUSE DONE");
}

void StepperSlave::handleResumeCommand() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Executing RESUME");
  isPaused = false;
  if (stepper.distanceToGo() != 0) {
    setBrake(false);
  }

  // Turn indicator back ON if we have motions to complete
  if (queuedMotionsCount > 0 && currentMotionIndex < queuedMotionsCount) {
    setIndicator(true);
  }

  sendFeedback("RESUME DONE");
}

void StepperSlave::handleResetCommand() {
  debugSerial.println("SLAVE " + String(slaveId) + ": Executing RESET");
  isPaused = false;
  isDelaying = false;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;
  stepper.stop();
  setBrake(true);

  // Turn off indicator when reset
  setIndicator(false);

  sendFeedback("RESET DONE");
}

void StepperSlave::startNextMotion() {
  if (currentMotionIndex < queuedMotionsCount) {
    // Turn on indicator as we're starting motion
    setIndicator(true);

    if (motionQueue[currentMotionIndex].delayBeforeMove > 0) {
      isDelaying = true;
      delayStartTime = millis();
      sendFeedback("DELAYING");
    } else if (motionQueue[currentMotionIndex].isDelayOnly) {
      currentMotionIndex++;
      startNextMotion();
    } else {
      setBrake(false);
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
        setBrake(false);
        stepper.setMaxSpeed(motionQueue[currentMotionIndex].speed);
        stepper.moveTo(motionQueue[currentMotionIndex].position);
        sendFeedback("MOVING");
      }
    }
    return;
  }

  if (stepper.distanceToGo() == 0 && !stepper.isRunning() && !isDelaying && !isPaused) {
    setBrake(true);

    if (currentMotionIndex < queuedMotionsCount - 1) {
      currentMotionIndex++;
      startNextMotion();
    } else if (currentMotionIndex == queuedMotionsCount - 1 && !hasReportedCompletion) {
      // Turn off indicator when sequence is completed
      setIndicator(false);

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

  // Turn on indicator if we have motions to perform
  if (queuedMotionsCount > 0) {
    setIndicator(true);
  }
}

void StepperSlave::sendFeedback(const String& message) {
  String feedback = String(slaveId) + ";" + message;
  masterSerial.println(feedback);
  debugSerial.println("SLAVE→MASTER: " + feedback);
}

void StepperSlave::checkPositionReached() {
  static unsigned long lastReportTime = 0;
  static long lastReportedPosition = -999999;

  if ((millis() - lastReportTime > 2000) || (abs(stepper.currentPosition() - lastReportedPosition) > 50)) {
    if (stepper.isRunning() || abs(stepper.currentPosition() - lastReportedPosition) > 10) {
      lastReportTime = millis();
      lastReportedPosition = stepper.currentPosition();

      String positionUpdate = "POS:" + String(stepper.currentPosition()) + " TARGET:" + String(stepper.targetPosition());
      sendFeedback(positionUpdate);
    }
  }
}

void StepperSlave::setBrake(bool engaged) {
  if (brakePin != -1) {
    bool brakeState = engaged;
    if (invertBrakeLogic) {
      brakeState = !brakeState;
    }
    digitalWrite(brakePin, brakeState ? HIGH : LOW);
    // debugSerial.println("SLAVE " + String(slaveId) + ": Brake " + (engaged ? "engaged" : "released"));
  }
}

void StepperSlave::setIndicator(bool active) {
  if (indicatorPin != -1) {
    digitalWrite(indicatorPin, active ? LOW : HIGH);
    // debugSerial.println("SLAVE " + String(slaveId) + ": Indicator " + (active ? "ON" : "OFF"));
  }
}