#include "StepperSlave.h"

StepperSlave* StepperSlave::instance = nullptr;

StepperSlave::StepperSlave(
  char id, int rxPin, int txPin, int clkPin, int cwPin, int enPin, int sensorPin,
  int brakePin, bool invertBrakeLogic, int indicatorPin, bool invertEnableLogic,
  unsigned long brakeReleaseDelayMs, unsigned long brakeEngageDelayMs)
  : slaveId(id),
    comm(id, rxPin, txPin),
    motor(clkPin, cwPin, enPin, sensorPin, brakePin, invertBrakeLogic,
          indicatorPin, invertEnableLogic, brakeReleaseDelayMs, brakeEngageDelayMs),
    motionQueue(5) {

  instance = this;
}

void StepperSlave::begin() {
  comm.begin();
  motor.begin();
  comm.setCommandCallback(onCommandReceived);

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": System initialized");
}

void StepperSlave::update() {
  comm.update();

  if (motorState != MOTOR_PAUSED) {
    handleMotion();
  }
}

void StepperSlave::onCommandReceived(int command, const String& params) {
  if (instance) {
    switch (command) {
      case CMD_ZERO:
        instance->handleZeroCommand();
        break;
      case CMD_SETSPEED:
        instance->handleSetSpeedCommand(params);
        break;
      case CMD_RUN:
        instance->handleMoveCommand(params);
        break;
      default:
        // Unknown command
        break;
    }
  }
}

void StepperSlave::reportPosition() {
  String positionUpdate = "POS:" + String(motor.getPosition()) + " TARGET:" + String(motor.getTargetPosition());
  comm.sendFeedback(positionUpdate);
}

void StepperSlave::handleZeroCommand() {
  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Executing ZERO (Homing)");

  motorState = MOTOR_IDLE;
  motionQueue.clear();
  motor.setIndicator(false);

  if (motor.performHoming()) {
    comm.sendFeedback("ZERO DONE");
  } else {
    comm.sendFeedback("ZERO FAILED");
  }
}

void StepperSlave::handleMoveCommand(const String& params) {
  parsePositionSequence(params);
  hasReportedCompletion = false;

  if (!motionQueue.isEmpty()) {
    motor.setIndicator(true);
    motorState = MOTOR_IDLE;
    executeCurrentMotion();
  }
}

void StepperSlave::handleSetSpeedCommand(const String& params) {
  float newSpeed = params.toFloat();
  if (newSpeed > 0) {
    motor.setSpeed(newSpeed);
    comm.sendFeedback("SPEED SET TO " + String(motor.getSpeed()));
  } else {
    comm.sendFeedback("INVALID SPEED VALUE");
  }
}

void StepperSlave::parsePositionSequence(const String& params) {
  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Parsing position sequence: " + params);

  int semicolonPos = -1;
  int startPos = 0;
  motionQueue.clear();

  do {
    semicolonPos = params.indexOf(';', startPos);
    String param = (semicolonPos == -1) ? params.substring(startPos) : params.substring(startPos, semicolonPos);
    param.trim();

    if (!motionQueue.isFull()) {
      if (param.startsWith("d")) {
        unsigned long delayValue = param.substring(1).toInt();
        motionQueue.add(0, motor.getSpeed(), delayValue, true);
        DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Queueing delay - Delay: " + String(delayValue) + "ms");
      } else {
        long position = param.toInt();
        motionQueue.add(position, motor.getSpeed(), 0, false);
        DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Queueing position - Pos: " + String(position));
      }
    }

    startPos = semicolonPos + 1;
  } while (semicolonPos != -1 && startPos < params.length() && !motionQueue.isFull());

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Total queued motions: " + String(motionQueue.size()));
}

void StepperSlave::handleMotion() {
  if (motionQueue.isEmpty()) return;

  if (motorState == MOTOR_DELAYING) {
    if (millis() - delayStartTime >= motionQueue.current()->delayMs) {
      DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Delay completed");
      motorState = MOTOR_IDLE;
      motionQueue.current()->completed = true;

      if (motionQueue.moveToNext()) {
        executeCurrentMotion();
      } else {
        motor.setIndicator(false);
        comm.sendFeedback("SEQUENCE COMPLETED");
        hasReportedCompletion = true;
        motorState = MOTOR_IDLE;
      }
    }
  }
}

void StepperSlave::executeCurrentMotion() {
  auto* currentMotion = motionQueue.current();
  if (currentMotion == nullptr) {
    motor.setIndicator(false);
    comm.sendFeedback("SEQUENCE COMPLETED");
    hasReportedCompletion = true;
    motorState = MOTOR_IDLE;
    return;
  }

  motor.setIndicator(true);

  if (currentMotion->isDelayOnly) {
    motorState = MOTOR_DELAYING;
    delayStartTime = millis();
    comm.sendFeedback("DELAYING");
    return;
  }

  motorState = MOTOR_MOVING;

  motor.activateMotor();
  motor.setSpeed(currentMotion->speed);

  comm.sendFeedback("MOVING TO " + String(currentMotion->position));
  reportPosition();

  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Moving to position: " + String(currentMotion->position));
  motor.moveTo(currentMotion->position);
  motor.runToPosition();

  reportPosition();
  comm.sendFeedback("POSITION REACHED");
  DEBUG_PRINTLN("SLAVE " + String(slaveId) + ": Position reached");

  motor.deactivateMotor();

  currentMotion->completed = true;
  motorState = MOTOR_IDLE;

  if (motionQueue.moveToNext()) {
    executeCurrentMotion();
  } else {
    motor.setIndicator(false);
    comm.sendFeedback("SEQUENCE COMPLETED");
    hasReportedCompletion = true;
  }
}