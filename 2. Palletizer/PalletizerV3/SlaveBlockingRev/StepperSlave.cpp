#include "Arduino.h"
#include "StepperSlave.h"

StepperSlave* StepperSlave::instance = nullptr;

StepperSlave::StepperSlave(
  char id, int rxPin, int txPin, int clkPin, int cwPin, int enPin, int sensorPin,
  int brakePin, bool invertBrakeLogic, int indicatorPin, bool invertEnableLogic,
  unsigned long brakeReleaseDelayMs, unsigned long brakeEngageDelayMs,
  unsigned long enableReleaseDelayMs, unsigned long enableEngageDelayMs, bool enableDebug)
  : slaveId(id),
    masterCommSerial(rxPin, txPin),
    stepper(AccelStepper::DRIVER, clkPin, cwPin),
    enPin(enPin),
    sensorPin(sensorPin),
    brakePin(brakePin),
    indicatorPin(indicatorPin),
    invertBrakeLogic(invertBrakeLogic),
    invertEnableLogic(invertEnableLogic),
    debugEnabled(enableDebug) {

  instance = this;

  brakeReleaseDelay = brakeReleaseDelayMs;
  brakeEngageDelay = brakeEngageDelayMs;
  enableReleaseDelay = enableReleaseDelayMs;
  enableEngageDelay = enableEngageDelayMs;

  acceleration = maxSpeed * SPEED_RATIO;
}

void StepperSlave::begin() {
  Serial.begin(9600);
  masterCommSerial.begin(9600);
  masterSerial.begin(&masterCommSerial);
  debugSerial.begin(&Serial);
  masterSerial.setDataCallback(onMasterDataWrapper);

  if (enPin != NOT_CONNECTED) {
    pinMode(enPin, OUTPUT);
    setEnable(false);
    isEnableActive = false;
  }

  pinMode(sensorPin, INPUT_PULLUP);

  if (brakePin != NOT_CONNECTED) {
    pinMode(brakePin, OUTPUT);
    setBrake(true);
    isBrakeEngaged = true;
  }

  if (indicatorPin != NOT_CONNECTED) {
    pinMode(indicatorPin, OUTPUT);
    setIndicator(false);
  }

  stepper.setMaxSpeed(maxSpeed);
  stepper.setAcceleration(acceleration);
  stepper.setCurrentPosition(0);
  stepper.setMinPulseWidth(100);

  debugPrintln("SLAVE " + String(slaveId) + ": Sistem diinisialisasi");
}

void StepperSlave::update() {
  masterSerial.checkCallback();

  if (motorState != MOTOR_PAUSED) {
    handleMotion();
  }
}

void StepperSlave::onMasterDataWrapper(const String& data) {
  if (instance) {
    instance->onMasterData(data);
  }
}

void StepperSlave::onMasterData(const String& data) {
  debugPrintln("MASTER→SLAVE: " + data);
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

  debugPrintln("SLAVE " + String(slaveId) + ": Processing command " + String(cmdCode));

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
      debugPrintln("SLAVE " + String(slaveId) + ": Unknown command " + String(cmdCode));
      break;
  }
}

void StepperSlave::sendFeedback(const String& message) {
  String feedback = String(slaveId) + ";" + message;
  masterSerial.println(feedback);
  debugPrintln("SLAVE→MASTER: " + feedback);
}

void StepperSlave::reportPosition() {
  String positionUpdate = "POS:" + String(stepper.currentPosition()) + " TARGET:" + String(stepper.targetPosition());
  sendFeedback(positionUpdate);
}

void StepperSlave::handleZeroCommand() {
  debugPrintln("SLAVE " + String(slaveId) + ": Executing ZERO (Homing)");

  motorState = MOTOR_IDLE;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;
  setIndicator(false);

  performHoming();

  sendFeedback("ZERO DONE");
}

void StepperSlave::handlePauseCommand() {
  debugPrintln("SLAVE " + String(slaveId) + ": Executing PAUSE");

  stepper.stop();
  motorState = MOTOR_PAUSED;

  setBrake(true);
  isBrakeEngaged = true;

  setEnable(false);
  isEnableActive = false;

  sendFeedback("PAUSE DONE");
}

void StepperSlave::handleResumeCommand() {
  debugPrintln("SLAVE " + String(slaveId) + ": Executing RESUME");

  if (motorState == MOTOR_PAUSED) {
    if (stepper.distanceToGo() != 0) {
      setBrake(false);
      if (brakeReleaseDelay > 0) {
        delay(brakeReleaseDelay);
      }

      setEnable(true);
      if (enableReleaseDelay > 0) {
        delay(enableReleaseDelay);
      }

      motorState = MOTOR_MOVING;
    } else {
      motorState = MOTOR_IDLE;
    }
  }

  if (queuedMotionsCount > 0 && currentMotionIndex < queuedMotionsCount) {
    setIndicator(true);
  }

  sendFeedback("RESUME DONE");
}

void StepperSlave::handleResetCommand() {
  debugPrintln("SLAVE " + String(slaveId) + ": Executing RESET");

  motorState = MOTOR_IDLE;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;
  stepper.stop();

  setBrake(true);
  isBrakeEngaged = true;

  if (enPin != NOT_CONNECTED) {
    setEnable(false);
    isEnableActive = false;
  }

  setIndicator(false);

  sendFeedback("RESET DONE");
}

void StepperSlave::handleMoveCommand(const String& params) {
  parsePositionSequence(params);
  hasReportedCompletion = false;

  if (queuedMotionsCount > 0) {
    setIndicator(true);
    motorState = MOTOR_IDLE;
    currentMotionIndex = 0;
    executeCurrentMotion();
  }
}

void StepperSlave::handleSetSpeedCommand(const String& params) {
  float newSpeed = params.toFloat();
  if (newSpeed > 0) {
    if (debugEnabled) {
      debugSerial.print("SLAVE ");
      debugSerial.print(slaveId);
      debugSerial.print(": Setting speed to ");
      debugSerial.println(newSpeed);
    }

    maxSpeed = newSpeed;
    stepper.setMaxSpeed(maxSpeed);

    acceleration = maxSpeed * SPEED_RATIO;
    stepper.setAcceleration(acceleration);

    sendFeedback("SPEED SET TO " + String(maxSpeed));
  } else {
    if (debugEnabled) {
      debugSerial.print("SLAVE ");
      debugSerial.print(slaveId);
      debugSerial.println(": Invalid speed value");
    }
    sendFeedback("INVALID SPEED VALUE");
  }
}

void StepperSlave::parsePositionSequence(const String& params) {
  debugPrintln("SLAVE " + String(slaveId) + ": Parsing position sequence: " + params);

  int semicolonPos = -1;
  int startPos = 0;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;

  do {
    semicolonPos = params.indexOf(';', startPos);
    String param = (semicolonPos == -1) ? params.substring(startPos) : params.substring(startPos, semicolonPos);
    param.trim();

    debugPrintln("SLAVE " + String(slaveId) + ": Parsing parameter: " + param);

    if (queuedMotionsCount < MAX_MOTIONS) {
      motionQueue[queuedMotionsCount].speed = maxSpeed;
      motionQueue[queuedMotionsCount].completed = false;

      if (param.startsWith("d")) {
        unsigned long delayValue = param.substring(1).toInt();
        motionQueue[queuedMotionsCount].delayMs = delayValue;
        motionQueue[queuedMotionsCount].position = 0;
        motionQueue[queuedMotionsCount].isDelayOnly = true;

        debugPrintln("SLAVE " + String(slaveId) + ": Queueing delay " + String(queuedMotionsCount) + " - Delay: " + String(delayValue) + "ms");
      } else {
        long position = param.toInt();
        motionQueue[queuedMotionsCount].position = position;
        motionQueue[queuedMotionsCount].delayMs = 0;
        motionQueue[queuedMotionsCount].isDelayOnly = false;

        debugPrintln("SLAVE " + String(slaveId) + ": Queueing position " + String(queuedMotionsCount) + " - Pos: " + String(position));
      }

      queuedMotionsCount++;
    }

    startPos = semicolonPos + 1;
  } while (semicolonPos != -1 && startPos < params.length() && queuedMotionsCount < MAX_MOTIONS);

  debugPrintln("SLAVE " + String(slaveId) + ": Total queued motions: " + String(queuedMotionsCount));
}

void StepperSlave::handleMotion() {
  if (queuedMotionsCount == 0 || currentMotionIndex >= queuedMotionsCount) return;

  if (motorState == MOTOR_DELAYING) {
    if (millis() - delayStartTime >= motionQueue[currentMotionIndex].delayMs) {
      debugPrintln("SLAVE " + String(slaveId) + ": Delay completed");
      motorState = MOTOR_IDLE;
      motionQueue[currentMotionIndex].completed = true;

      currentMotionIndex++;
      if (currentMotionIndex < queuedMotionsCount) {
        executeCurrentMotion();
      } else {
        setIndicator(false);
        sendFeedback("SEQUENCE COMPLETED");
        hasReportedCompletion = true;
        motorState = MOTOR_IDLE;
      }
    }
  }
}

void StepperSlave::executeCurrentMotion() {
  if (currentMotionIndex >= queuedMotionsCount) {
    setIndicator(false);
    sendFeedback("SEQUENCE COMPLETED");
    hasReportedCompletion = true;
    motorState = MOTOR_IDLE;
    return;
  }

  setIndicator(true);

  if (motionQueue[currentMotionIndex].isDelayOnly) {
    motorState = MOTOR_DELAYING;
    delayStartTime = millis();
    sendFeedback("DELAYING");
    return;
  }

  motorState = MOTOR_MOVING;

  setBrake(false);
  if (brakeReleaseDelay > 0) {
    delay(brakeReleaseDelay);
  }

  setEnable(true);
  if (enableReleaseDelay > 0) {
    delay(enableReleaseDelay);
  }

  long targetPosition = motionQueue[currentMotionIndex].position;
  stepper.setMaxSpeed(motionQueue[currentMotionIndex].speed);

  sendFeedback("MOVING TO " + String(targetPosition));
  reportPosition();

  stepper.moveTo(targetPosition);
  stepper.runToPosition();

  reportPosition();
  sendFeedback("POSITION REACHED");

  if (brakeEngageDelay > 0) {
    delay(brakeEngageDelay);
  }
  setBrake(true);

  if (isEnableActive) {
    setEnable(false);
    if (enableEngageDelay > 0) {
      delay(enableEngageDelay);
    }
  }

  motionQueue[currentMotionIndex].completed = true;
  motorState = MOTOR_IDLE;

  currentMotionIndex++;
  if (currentMotionIndex < queuedMotionsCount) {
    executeCurrentMotion();
  } else {
    setIndicator(false);
    sendFeedback("SEQUENCE COMPLETED");
    hasReportedCompletion = true;
  }
}

void StepperSlave::performHoming() {
  debugPrintln("SLAVE " + String(slaveId) + ": Starting homing sequence");
  setIndicator(true);

  float originalSpeed = stepper.maxSpeed();
  float originalAccel = stepper.acceleration();

  stepper.setMaxSpeed(HOMING_SPEED);
  stepper.setAcceleration(HOMING_ACCEL);

  setBrake(false);
  isBrakeEngaged = false;
  setEnable(true);
  isEnableActive = true;

  long distance = 0;
  int count = 20000;

  if (digitalRead(sensorPin) == HIGH) {
    debugPrintln("SLAVE " + String(slaveId) + ": Already in sensor area, moving out first");
    stepper.move(count);
    while (digitalRead(sensorPin) == HIGH && stepper.distanceToGo() != 0) {
      stepper.run();
    }

    if (stepper.distanceToGo() != 0) {
      stepper.stop();
      stepper.setCurrentPosition(stepper.currentPosition());
      debugPrintln("SLAVE " + String(slaveId) + ": Moving back to sensor");
      stepper.move(-count);
      while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0) {
        stepper.run();
      }
    }
  } else {
    debugPrintln("SLAVE " + String(slaveId) + ": Outside sensor area, moving to sensor");
    stepper.move(-count);
    while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0) {
      stepper.run();
    }
  }

  stepper.stop();
  debugPrintln("SLAVE " + String(slaveId) + ": Sensor detected");

  distance = stepper.distanceToGo();
  stepper.runToPosition();

  debugPrintln("SLAVE " + String(slaveId) + ": Correcting overshot by " + String(distance) + " steps");
  stepper.move(-distance);
  stepper.runToPosition();

  debugPrintln("SLAVE " + String(slaveId) + ": Setting home position (0)");
  stepper.setCurrentPosition(0);

  stepper.setMaxSpeed(originalSpeed);
  stepper.setAcceleration(originalAccel);
  setBrake(true);
  isBrakeEngaged = true;

  if (enPin != NOT_CONNECTED) {
    setEnable(false);
    isEnableActive = false;
  }

  setIndicator(false);

  debugPrintln("SLAVE " + String(slaveId) + ": Homing completed");
}

void StepperSlave::setBrake(bool engaged) {
  if (brakePin != NOT_CONNECTED) {
    bool brakeState = engaged;
    if (invertBrakeLogic) {
      brakeState = !brakeState;
    }
    digitalWrite(brakePin, brakeState ? HIGH : LOW);
    isBrakeEngaged = engaged;
  }
}

void StepperSlave::setBrakeWithDelay(bool engaged) {
  if (brakePin != NOT_CONNECTED) {
    if (engaged == isBrakeEngaged) {
      return;
    }


    if (engaged && brakeEngageDelay > 0) {
      setBrake(engaged);
      delay(brakeEngageDelay);
    } else if (!engaged && brakeReleaseDelay > 0) {
      delay(brakeReleaseDelay);
      setBrake(engaged);
    }
  } else {
    setBrake(engaged);
  }
}

void StepperSlave::setEnable(bool active) {
  if (enPin != NOT_CONNECTED) {
    bool enableState = active;
    if (invertEnableLogic) {
      enableState = !enableState;
    }
    digitalWrite(enPin, enableState ? LOW : HIGH);
    isEnableActive = active;
  }
}

void StepperSlave::setEnableWithDelay(bool active) {
  if (enPin != NOT_CONNECTED) {
    if (active == isEnableActive) {
      return;
    }

    setEnable(active);

    if (active && enableReleaseDelay > 0) {
      delay(enableReleaseDelay);
    } else if (!active && enableEngageDelay > 0) {
      delay(enableEngageDelay);
    }
  }
}

void StepperSlave::setIndicator(bool active) {
  if (indicatorPin != NOT_CONNECTED) {
    digitalWrite(indicatorPin, active ? LOW : HIGH);
  }
}

void StepperSlave::debugPrint(const String& message) {
  if (debugEnabled) {
    debugSerial.print(message);
  }
}

void StepperSlave::debugPrintln(const String& message) {
  if (debugEnabled) {
    debugSerial.println(message);
  }
}