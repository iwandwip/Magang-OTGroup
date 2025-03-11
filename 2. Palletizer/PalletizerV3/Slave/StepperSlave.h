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
  enum Command {
    CMD_NONE,
    CMD_START,
    CMD_ZERO,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_RESET,
    CMD_SETSPEED
  };

  StepperSlave(char slaveId, int rxPin, int txPin, int clkPin, int cwPin, int enPin, int sensorPin, int brakePin = -1,
               bool invertBrakeLogic = false, int indicatorPin = -1, bool invertEnableLogic = false,
               unsigned long brakeReleaseDelayMs = 500, unsigned long brakeEngageDelayMs = 1500,
               unsigned long enableReleaseDelayMs = 500, unsigned long enableEngageDelayMs = 1500);

  void begin();
  void update();
  static void onMasterDataWrapper(const String& data);

private:
  struct MotionStep {
    long position;
    float speed;
    unsigned long delayBeforeMove;
    bool completed;
    bool isDelayOnly;
  };

  static StepperSlave* instance;

  char slaveId;
  SoftwareSerial masterCommSerial;
  EnhancedSerial masterSerial;
  EnhancedSerial debugSerial;
  AccelStepper stepper;

  int enPin;
  int sensorPin;
  int brakePin;
  int indicatorPin;
  bool invertBrakeLogic;
  bool invertEnableLogic;

  const float SPEED_RATIO = 0.5;
  float MAX_SPEED = 200.0;
  float ACCELERATION = MAX_SPEED * SPEED_RATIO;
  const float HOMING_SPEED = 200.0;
  const float HOMING_ACCEL = 100.0;

  bool isPaused = false;
  bool isHoming = false;
  unsigned long delayStartTime = 0;
  bool isDelaying = false;
  bool hasReportedCompletion = true;

  bool isBrakeReleaseDelayActive = false;
  bool isBrakeEngageDelayActive = false;
  unsigned long brakeReleaseDelayStart = 0;
  unsigned long brakeEngageDelayStart = 0;
  unsigned long BRAKE_RELEASE_DELAY = 500;
  unsigned long BRAKE_ENGAGE_DELAY = 1500;
  bool isBrakeEngaged = true;

  bool isEnableReleaseDelayActive = false;
  bool isEnableEngageDelayActive = false;
  unsigned long enableReleaseDelayStart = 0;
  unsigned long enableEngageDelayStart = 0;
  unsigned long ENABLE_RELEASE_DELAY = 500;
  unsigned long ENABLE_ENGAGE_DELAY = 1500;
  bool isEnableActive = false;

  static const int MAX_MOTIONS = 5;
  MotionStep motionQueue[MAX_MOTIONS];
  int currentMotionIndex = 0;
  int queuedMotionsCount = 0;

  void onMasterData(const String& data);
  void processCommand(const String& data);
  void handleZeroCommand();
  void handlePauseCommand();
  void handleResumeCommand();
  void handleResetCommand();
  void handleMoveCommand(const String& params);
  void handleSetSpeedCommand(const String& params);
  void parsePositionSequence(const String& params);
  void handleMotion();
  void startNextMotion();
  void sendFeedback(const String& message);
  void checkPositionReached();
  void performHoming();
  void setBrake(bool engaged);
  void setBrakeWithDelay(bool engaged);
  void setEnable(bool active);
  void setEnableWithDelay(bool active);
  void setIndicator(bool active);
};

#endif