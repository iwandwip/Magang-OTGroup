#ifndef STEPPER_SLAVE_H
#define STEPPER_SLAVE_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED

#define DEBUG 1

#if DEBUG
#define DEBUG_PRINT(x) debugSerial.print(x)
#define DEBUG_PRINTLN(x) debugSerial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#include "Kinematrix.h"
#include "SoftwareSerial.h"
#include "AccelStepper.h"

#define X_AXIS 'x'
#define Y_AXIS 'y'
#define Z_AXIS 'z'
#define T_AXIS 't'
#define G_AXIS 'g'

#define NO_DELAY 0
#define NOT_CONNECTED -1
#define HIGH_LOGIC_BRAKE false
#define LOW_LOGIC_BRAKE true
#define HIGH_LOGIC_ENABLE false
#define LOW_LOGIC_ENABLE true

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

  struct MotionStep {
    long position;
    float speed;
    unsigned long delayMs;
    bool isDelayOnly;
    bool completed;
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
  static void onMasterDataWrapper(const String& data);

private:
  static StepperSlave* instance;

  static const int MAX_MOTIONS = 5;
  const float SPEED_RATIO = 0.6;
  const float HOMING_SPEED = 200.0;
  const float HOMING_ACCEL = 100.0;

  char slaveId;
  int enPin;
  int sensorPin;
  int brakePin;
  int indicatorPin;
  bool invertBrakeLogic;
  bool invertEnableLogic;

  SoftwareSerial masterCommSerial;
  EnhancedSerial masterSerial;
  EnhancedSerial debugSerial;

  AccelStepper stepper;
  float maxSpeed = 200.0;
  float acceleration;

  MotorState motorState = MOTOR_IDLE;
  bool hasReportedCompletion = true;

  unsigned long delayStartTime = 0;

  unsigned long brakeReleaseDelay = 500;
  unsigned long brakeEngageDelay = 1500;
  bool isBrakeEngaged = true;
  bool isEnableActive = false;

  MotionStep motionQueue[MAX_MOTIONS];
  int currentMotionIndex = 0;
  int queuedMotionsCount = 0;

  void onMasterData(const String& data);
  void processCommand(const String& data);
  void sendFeedback(const String& message);
  void reportPosition();

  void handleZeroCommand();
  void handleMoveCommand(const String& params);
  void handleSetSpeedCommand(const String& params);

  void parsePositionSequence(const String& params);
  void handleMotion();
  void executeCurrentMotion();

  void performHoming();

  void setBrake(bool engaged);
  void setEnable(bool active);
  void activateMotor();
  void deactivateMotor();
  void setIndicator(bool active);
};

#endif