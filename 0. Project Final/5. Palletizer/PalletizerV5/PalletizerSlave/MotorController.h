#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "AccelStepper.h"

#define NOT_CONNECTED -1
#define NO_DELAY 0
#define HIGH_LOGIC_BRAKE false
#define LOW_LOGIC_BRAKE true
#define HIGH_LOGIC_ENABLE false
#define LOW_LOGIC_ENABLE true

class MotorController {
public:
  MotorController(
    int clkPin,
    int cwPin,
    int enPin,
    int sensorPin,
    int brakePin = NOT_CONNECTED,
    bool invertBrakeLogic = false,
    int indicatorPin = NOT_CONNECTED,
    bool invertEnableLogic = false,
    unsigned long brakeReleaseDelayMs = 500,
    unsigned long brakeEngageDelayMs = 1500);

  void begin();

  void setSpeed(float speed);
  float getSpeed() const;
  void setPosition(long position);
  long getPosition() const;
  long getTargetPosition() const;

  void moveTo(long position);
  void runToPosition();
  void stop();
  bool isRunning() const;

  bool performHoming();
  void setIndicator(bool active);

  void activateMotor();
  void deactivateMotor();

private:
  AccelStepper stepper;
  float maxSpeed = 200.0;
  float acceleration;
  const float SPEED_RATIO = 0.6;
  const float HOMING_SPEED = 200.0;
  const float HOMING_ACCEL = 100.0;

  int enPin;
  int sensorPin;
  int brakePin;
  int indicatorPin;
  bool invertBrakeLogic;
  bool invertEnableLogic;

  unsigned long brakeReleaseDelay = 500;
  unsigned long brakeEngageDelay = 1500;
  bool isBrakeEngaged = true;
  bool isEnableActive = false;

  void setBrake(bool engaged);
  void setEnable(bool active);
};

#endif