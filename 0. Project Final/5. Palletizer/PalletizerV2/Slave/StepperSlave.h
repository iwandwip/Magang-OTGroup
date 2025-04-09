#ifndef STEPPER_SLAVE_H
#define STEPPER_SLAVE_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED

#include "Kinematrix.h"
#include "SoftwareSerial.h"
#include "AccelStepper.h"

class StepperSlave {
public:
  enum Command {
    CMD_NONE,
    CMD_START,
    CMD_ZERO,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_RESET
  };

  StepperSlave(char slaveId, int rxPin, int txPin, int clkPin, int cwPin, int enPin, int sensorPin);
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

  const float MAX_SPEED = 2000.0;
  const float SPEED_RATIO = 0.6;
  const float ACCELERATION = MAX_SPEED * SPEED_RATIO;
  const float HOMING_SPEED = 250.0;
  const float HOMING_ACCEL = 125.0;

  bool isPaused = false;
  bool isHoming = false;
  unsigned long delayStartTime = 0;
  bool isDelaying = false;
  bool hasReportedCompletion = true;

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
  void parsePositionSequence(const String& params);
  void handleMotion();
  void startNextMotion();
  void sendFeedback(const String& message);
  void checkPositionReached();
  void performHoming();
};

#endif