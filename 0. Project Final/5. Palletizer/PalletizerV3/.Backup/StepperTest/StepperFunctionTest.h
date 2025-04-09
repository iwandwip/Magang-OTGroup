#ifndef STEPPER_FUNCTION_TEST_H
#define STEPPER_FUNCTION_TEST_H

#include "AccelStepper.h"
#include "Arduino.h"

class StepperFunctionTest {
public:
  // Constructor
  StepperFunctionTest(int clkPin, int cwPin, int enPin, int sensorPin);

  // Basic methods
  void begin();
  void update();

  // Test functions
  void testClockwise(long steps, float speed);
  void testCounterClockwise(long steps, float speed);
  void testHoming();
  void testSequence(int maxSteps, float speed);
  void testBackAndForth(int cycles, long distance, float speed);
  void stop();

  // Settings
  void setMaxSpeed(float speed);
  void setAcceleration(float acceleration);

  // Status methods
  long getCurrentPosition();
  long getTargetPosition();
  bool isMoving();
  bool isSensorActive();

private:
  AccelStepper stepper;
  int enPin;
  int sensorPin;

  float maxSpeed = 2000.0;
  float acceleration = 1000.0;
  float homingSpeed = 250.0;
  float homingAcceleration = 125.0;

  unsigned long lastStatusUpdateTime = 0;
  const unsigned long STATUS_UPDATE_INTERVAL = 500;  // ms

  // Test sequence variables
  enum TestState {
    TEST_IDLE,
    TEST_CLOCKWISE,
    TEST_COUNTERCLOCKWISE,
    TEST_HOMING,
    TEST_BACK_AND_FORTH,
    TEST_SEQUENCE
  };

  enum SequenceStep {
    SEQ_START,
    SEQ_HOME,
    SEQ_MOVE_CW,
    SEQ_MOVE_CCW,
    SEQ_FINISHED
  };

  TestState currentTest = TEST_IDLE;
  SequenceStep sequenceStep = SEQ_START;
  int sequenceMaxSteps = 0;
  float sequenceSpeed = 0;

  // Back and forth specific variables
  int cyclesTotal = 0;
  int currentCycle = 0;
  long moveDistance = 0;
  bool movingForward = true;

  // Private methods
  void performHoming();
  void printStatus();
};

#endif