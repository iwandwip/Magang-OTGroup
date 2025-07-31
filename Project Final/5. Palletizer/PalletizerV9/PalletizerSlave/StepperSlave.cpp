#include "StepperSlave.h"

StepperSlave* StepperSlave::instance = nullptr;

StepperSlave::StepperSlave(
  char id, int rxPin, int txPin, int clkPin, int cwPin, int enPin, int sensorPin,
  int brakePin, bool invertBrakeLogic, int indicatorPin, bool invertEnableLogic,
  unsigned long brakeReleaseDelayMs, unsigned long brakeEngageDelayMs)
  : slaveId(id),
    masterCommSerial(rxPin, txPin),
    stepper(AccelStepper::DRIVER, clkPin, cwPin),
    enPin(enPin),
    sensorPin(sensorPin),
    brakePin(brakePin),
    indicatorPin(indicatorPin),
    invertBrakeLogic(invertBrakeLogic),
    invertEnableLogic(invertEnableLogic) {

  instance = this;

  brakeReleaseDelay = brakeReleaseDelayMs;
  brakeEngageDelay = brakeEngageDelayMs;

  acceleration = maxSpeed * SPEED_RATIO;
}

void StepperSlave::begin() {
  Serial.begin(9600);
  masterCommSerial.begin(9600);
  masterSerial.begin(&Serial);  // masterCommSerial
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

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Sistem diinisialisasi");
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
  DEBUG_PRINTLN("MASTER→SLAVE: " + data);
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

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Processing command " + String(cmdCode));

  switch (cmdCode) {
    case CMD_ZERO:
      handleZeroCommand();
      break;
    case CMD_SETSPEED:
      if (secondSeparator != -1) {
        handleSetSpeedCommand(data.substring(secondSeparator + 1));
      }
      break;
    case CMD_RUN:
      if (secondSeparator != -1) {
        handleMoveCommand(data.substring(secondSeparator + 1));
      }
      break;
    default:
      DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Unknown command " + String(cmdCode));
      break;
  }
}

void StepperSlave::sendFeedback(const String& message) {
  String feedback = String(slaveId) + ";" + message;
  masterSerial.println(feedback);
  DEBUG_PRINTLN("SLAVE→MASTER: " + feedback);
}

void StepperSlave::reportPosition() {
  String positionUpdate = "POS:" + String(stepper.currentPosition()) + " TARGET:" + String(stepper.targetPosition());
  sendFeedback(positionUpdate);
}

void StepperSlave::handleZeroCommand() {
  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Executing ZERO (Homing)");

  motorState = MOTOR_IDLE;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;
  setIndicator(false);

  performHoming();

  sendFeedback("ZERO DONE");
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
    DEBUG_PRINT("SLAVE ");
    DEBUG_PRINT(slaveId);
    DEBUG_PRINT(": Setting speed to ");
    DEBUG_PRINTLN(newSpeed);

    maxSpeed = newSpeed;
    stepper.setMaxSpeed(maxSpeed);

    acceleration = maxSpeed * SPEED_RATIO;
    stepper.setAcceleration(acceleration);

    sendFeedback("SPEED SET TO " + String(maxSpeed));
  } else {
    DEBUG_PRINT("SLAVE ");
    DEBUG_PRINT(slaveId);
    DEBUG_PRINTLN(": Invalid speed value");
    sendFeedback("INVALID SPEED VALUE");
  }
}

void StepperSlave::parsePositionSequence(const String& params) {
  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Parsing position sequence: " + params);

  int semicolonPos = -1;
  int startPos = 0;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;

  do {
    semicolonPos = params.indexOf(';', startPos);
    String param = (semicolonPos == -1) ? params.substring(startPos) : params.substring(startPos, semicolonPos);
    param.trim();

    DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Parsing parameter: " + param);

    if (queuedMotionsCount < MAX_MOTIONS) {
      motionQueue[queuedMotionsCount].speed = maxSpeed;
      motionQueue[queuedMotionsCount].completed = false;

      if (param.startsWith("d")) {
        unsigned long delayValue = param.substring(1).toInt();
        motionQueue[queuedMotionsCount].delayMs = delayValue;
        motionQueue[queuedMotionsCount].position = 0;
        motionQueue[queuedMotionsCount].isDelayOnly = true;

        DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Queueing delay " + String(queuedMotionsCount) + " - Delay: " + String(delayValue) + "ms");
      } else {
        long position = param.toInt();
        motionQueue[queuedMotionsCount].position = position;
        motionQueue[queuedMotionsCount].delayMs = 0;
        motionQueue[queuedMotionsCount].isDelayOnly = false;

        DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Queueing position " + String(queuedMotionsCount) + " - Pos: " + String(position));
      }

      queuedMotionsCount++;
    }

    startPos = semicolonPos + 1;
  } while (semicolonPos != -1 && startPos < params.length() && queuedMotionsCount < MAX_MOTIONS);

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Total queued motions: " + String(queuedMotionsCount));
}

void StepperSlave::handleMotion() {
  if (queuedMotionsCount == 0 || currentMotionIndex >= queuedMotionsCount) return;

  if (motorState == MOTOR_DELAYING) {
    if (millis() - delayStartTime >= motionQueue[currentMotionIndex].delayMs) {
      DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Delay completed");
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

  activateMotor();

  long targetPosition = motionQueue[currentMotionIndex].position;
  stepper.setMaxSpeed(motionQueue[currentMotionIndex].speed);

  sendFeedback("MOVING TO " + String(targetPosition));
  reportPosition();

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Moving to position: " + String(targetPosition));
  stepper.moveTo(targetPosition);
  stepper.runToPosition();

  reportPosition();
  sendFeedback("POSITION REACHED");
  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Position reached");

  deactivateMotor();

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
  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Starting homing sequence");
  setIndicator(true);

  float originalSpeed = stepper.maxSpeed();
  float originalAccel = stepper.acceleration();

  stepper.setMaxSpeed(HOMING_SPEED);
  stepper.setAcceleration(HOMING_ACCEL);

  activateMotor();

  long distance = 0;
  int count = 20000;

  if (digitalRead(sensorPin) == HIGH) {
    DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Already in sensor area, moving out first");
    stepper.move(count);
    while (digitalRead(sensorPin) == HIGH && stepper.distanceToGo() != 0) {
      stepper.run();
    }

    if (stepper.distanceToGo() != 0) {
      stepper.stop();
      stepper.setCurrentPosition(stepper.currentPosition());
      DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Moving back to sensor");
      stepper.move(-count);
      while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0) {
        stepper.run();
      }
    }
  } else {
    DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Outside sensor area, moving to sensor");
    stepper.move(-count);
    while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0) {
      stepper.run();
    }
  }

  stepper.stop();
  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Sensor detected");

  distance = stepper.distanceToGo();
  stepper.runToPosition();

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Correcting overshot by " + String(distance) + " steps");
  stepper.move(-distance);
  stepper.runToPosition();

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Setting home position (0)");
  stepper.setCurrentPosition(0);

  stepper.setMaxSpeed(originalSpeed);
  stepper.setAcceleration(originalAccel);

  deactivateMotor();

  setIndicator(false);

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Homing completed");
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

void StepperSlave::activateMotor() {
  setBrake(false);
  setEnable(true);
  if (brakeReleaseDelay > 0) {
    delay(brakeReleaseDelay);
  }
}

void StepperSlave::deactivateMotor() {
  if (brakeEngageDelay > 0) {
    delay(brakeEngageDelay);
  }
  setBrake(true);
  setEnable(false);
}

void StepperSlave::setIndicator(bool active) {
  if (indicatorPin != NOT_CONNECTED) {
    digitalWrite(indicatorPin, active ? LOW : HIGH);
  }
}