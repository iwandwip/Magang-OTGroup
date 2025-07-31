#include "MotorController.h"

MotorController::MotorController(
  int clkPin, int cwPin, int enPin, int sensorPin, 
  int brakePin, bool invertBrakeLogic, int indicatorPin, bool invertEnableLogic,
  unsigned long brakeReleaseDelayMs, unsigned long brakeEngageDelayMs)
  : stepper(AccelStepper::DRIVER, clkPin, cwPin),
    enPin(enPin),
    sensorPin(sensorPin),
    brakePin(brakePin),
    indicatorPin(indicatorPin),
    invertBrakeLogic(invertBrakeLogic),
    invertEnableLogic(invertEnableLogic) {
      
  brakeReleaseDelay = brakeReleaseDelayMs;
  brakeEngageDelay = brakeEngageDelayMs;
  
  acceleration = maxSpeed * SPEED_RATIO;
}

void MotorController::begin() {
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
}

void MotorController::setSpeed(float speed) {
  if (speed > 0) {
    maxSpeed = speed;
    stepper.setMaxSpeed(maxSpeed);

    acceleration = maxSpeed * SPEED_RATIO;
    stepper.setAcceleration(acceleration);
  }
}

float MotorController::getSpeed() const {
  return maxSpeed;
}

void MotorController::setPosition(long position) {
  stepper.setCurrentPosition(position);
}

long MotorController::getPosition() const {
  return stepper.currentPosition();
}

long MotorController::getTargetPosition() const {
  return stepper.targetPosition();
}

void MotorController::moveTo(long position) {
  stepper.moveTo(position);
}

void MotorController::runToPosition() {
  stepper.runToPosition();
}

void MotorController::stop() {
  stepper.stop();
}

bool MotorController::isRunning() const {
  return stepper.isRunning();
}

bool MotorController::performHoming() {
  setIndicator(true);

  float originalSpeed = stepper.maxSpeed();
  float originalAccel = stepper.acceleration();

  stepper.setMaxSpeed(HOMING_SPEED);
  stepper.setAcceleration(HOMING_ACCEL);

  activateMotor();

  long distance = 0;
  int count = 20000;

  if (digitalRead(sensorPin) == HIGH) {
    // Already in sensor area, moving out first
    stepper.move(count);
    while (digitalRead(sensorPin) == HIGH && stepper.distanceToGo() != 0) {
      stepper.run();
    }

    if (stepper.distanceToGo() != 0) {
      stepper.stop();
      stepper.setCurrentPosition(stepper.currentPosition());
      stepper.move(-count);
      while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0) {
        stepper.run();
      }
    }
  } else {
    // Outside sensor area, moving to sensor
    stepper.move(-count);
    while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0) {
      stepper.run();
    }
  }

  stepper.stop();

  distance = stepper.distanceToGo();
  stepper.runToPosition();

  stepper.move(-distance);
  stepper.runToPosition();

  stepper.setCurrentPosition(0);

  stepper.setMaxSpeed(originalSpeed);
  stepper.setAcceleration(originalAccel);

  deactivateMotor();

  setIndicator(false);
  
  return true;
}

void MotorController::setIndicator(bool active) {
  if (indicatorPin != NOT_CONNECTED) {
    digitalWrite(indicatorPin, active ? LOW : HIGH);
  }
}

void MotorController::setBrake(bool engaged) {
  if (brakePin != NOT_CONNECTED) {
    bool brakeState = engaged;
    if (invertBrakeLogic) {
      brakeState = !brakeState;
    }
    digitalWrite(brakePin, brakeState ? HIGH : LOW);
    isBrakeEngaged = engaged;
  }
}

void MotorController::setEnable(bool active) {
  if (enPin != NOT_CONNECTED) {
    bool enableState = active;
    if (invertEnableLogic) {
      enableState = !enableState;
    }
    digitalWrite(enPin, enableState ? LOW : HIGH);
    isEnableActive = active;
  }
}

void MotorController::activateMotor() {
  setBrake(false);
  setEnable(true);
  if (brakeReleaseDelay > 0) {
    delay(brakeReleaseDelay);
  }
}

void MotorController::deactivateMotor() {
  if (brakeEngageDelay > 0) {
    delay(brakeEngageDelay);
  }
  setBrake(true);
  setEnable(false);
}