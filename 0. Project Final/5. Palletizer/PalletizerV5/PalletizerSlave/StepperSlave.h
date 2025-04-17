#ifndef STEPPER_SLAVE_H
#define STEPPER_SLAVE_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED

#include "CommSlave.h"
#include "MotorController.h"
#include "MotionQueue.h"

#define X_AXIS 'x'
#define Y_AXIS 'y'
#define Z_AXIS 'z'
#define T_AXIS 't'
#define G_AXIS 'g'

class StepperSlave {
public:
  enum CommandCode {
    CMD_NONE = 0,
    CMD_RUN = 1,
    CMD_ZERO = 2,
    CMD_SETSPEED = 6
  };

  enum MotorState {
    MOTOR_IDLE,
    MOTOR_MOVING,
    MOTOR_DELAYING,
    MOTOR_PAUSED
  };

  StepperSlave(
    char slaveId,
    int rxPin,
    int txPin,
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
  void update();

private:
  static void onCommandReceived(int command, const String& params);
  static StepperSlave* instance;

  char slaveId;
  CommSlave comm;
  MotorController motor;
  MotionQueue motionQueue;

  MotorState motorState = MOTOR_IDLE;
  bool hasReportedCompletion = true;
  unsigned long delayStartTime = 0;

  void handleZeroCommand();
  void handleMoveCommand(const String& params);
  void handleSetSpeedCommand(const String& params);

  void parsePositionSequence(const String& params);
  void handleMotion();
  void executeCurrentMotion();
  void reportPosition();
};

#endif