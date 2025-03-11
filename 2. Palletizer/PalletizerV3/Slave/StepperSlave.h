#ifndef STEPPER_SLAVE_H
#define STEPPER_SLAVE_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED

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
    CMD_START = 1,
    CMD_ZERO = 2,
    CMD_PAUSE = 3,
    CMD_RESUME = 4,
    CMD_RESET = 5,
    CMD_SETSPEED = 6
  };

  enum HomingState {
    HOMING_IDLE = 0,
    HOMING_INIT = 1,
    HOMING_MOVE_FROM_SENSOR = 2,
    HOMING_SEEK_SENSOR = 3,
    HOMING_ADJUST_POSITION = 4,
    HOMING_COMPLETE = 5
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
    unsigned long brakeEngageDelayMs = 1500,
    unsigned long enableReleaseDelayMs = 500,
    unsigned long enableEngageDelayMs = 1500);

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
  bool isHoming = false;
  HomingState homingState = HOMING_IDLE;
  bool hasReportedCompletion = true;

  long homingStepsLimit = 20000;
  long homingDistanceCorrection = 0;
  float originalSpeed = 0;
  float originalAccel = 0;

  unsigned long delayStartTime = 0;

  bool isBrakeReleaseDelayActive = false;
  bool isBrakeEngageDelayActive = false;
  unsigned long brakeReleaseDelayStart = 0;
  unsigned long brakeEngageDelayStart = 0;
  unsigned long brakeReleaseDelay = 500;
  unsigned long brakeEngageDelay = 1500;
  bool isBrakeEngaged = true;

  bool isEnableReleaseDelayActive = false;
  bool isEnableEngageDelayActive = false;
  unsigned long enableReleaseDelayStart = 0;
  unsigned long enableEngageDelayStart = 0;
  unsigned long enableReleaseDelay = 500;
  unsigned long enableEngageDelay = 1500;
  bool isEnableActive = false;

  MotionStep motionQueue[MAX_MOTIONS];
  int currentMotionIndex = 0;
  int queuedMotionsCount = 0;

  void onMasterData(const String& data);
  void processCommand(const String& data);
  void sendFeedback(const String& message);
  void reportPosition();

  void handleZeroCommand();
  void handlePauseCommand();
  void handleResumeCommand();
  void handleResetCommand();
  void handleMoveCommand(const String& params);
  void handleSetSpeedCommand(const String& params);

  void parsePositionSequence(const String& params);
  void handleMotion();
  void startNextMotion();
  void checkPositionReached();

  void startHoming();
  void updateHoming();

  void setBrake(bool engaged);
  void setBrakeWithDelay(bool engaged);
  void setEnable(bool active);
  void setEnableWithDelay(bool active);
  void setIndicator(bool active);

  void updateTimers();
};

#endif