#include "StepperFunctionTest.h"

StepperFunctionTest::StepperFunctionTest(int clkPin, int cwPin, int enPin, int sensorPin)
  : stepper(AccelStepper::DRIVER, clkPin, cwPin),
    enPin(enPin),
    sensorPin(sensorPin) {
}

void StepperFunctionTest::begin() {
  Serial.begin(9600);

  // Configure pins
  pinMode(enPin, OUTPUT);
  pinMode(sensorPin, INPUT);

  // Enable stepper driver
  digitalWrite(enPin, LOW);  // LOW enables the stepper driver

  // Configure stepper
  stepper.setMaxSpeed(maxSpeed);
  stepper.setAcceleration(acceleration);

  Serial.println("StepperFunctionTest: System initialized");
  Serial.println("Available commands:");
  Serial.println("  cw,steps,speed - Move clockwise");
  Serial.println("  ccw,steps,speed - Move counter-clockwise");
  Serial.println("  home - Run homing sequence");
  Serial.println("  back_forth,cycles,distance,speed - Move back and forth");
  Serial.println("  sequence - Run test sequence");
  Serial.println("  stop - Stop all motion");
  Serial.println("  status - Print current status");
}

void StepperFunctionTest::update() {
  // Run the stepper motor
  stepper.run();

  // Check if we're doing a back and forth test
  if (currentTest == TEST_BACK_AND_FORTH && !stepper.isRunning()) {
    if (currentCycle < cyclesTotal) {
      if (movingForward) {
        // We just finished moving forward, now go back
        movingForward = false;
        stepper.move(-moveDistance);
      } else {
        // We just finished moving backward, start next cycle
        movingForward = true;
        currentCycle++;
        if (currentCycle < cyclesTotal) {
          stepper.move(moveDistance);
        } else {
          currentTest = TEST_IDLE;
          Serial.println("Back and forth test complete");
        }
      }
    }
  }

  // Handle sequence testing (non-blocking)
  if (currentTest == TEST_SEQUENCE && !stepper.isRunning()) {
    switch (sequenceStep) {
      case SEQ_START:
        // First step: home the stepper
        sequenceStep = SEQ_HOME;
        testHoming();
        break;

      case SEQ_HOME:
        // After homing is complete, move clockwise
        sequenceStep = SEQ_MOVE_CW;
        Serial.print("Sequence: Moving clockwise ");
        Serial.print(sequenceMaxSteps);
        Serial.println(" steps");
        testClockwise(sequenceMaxSteps, sequenceSpeed);
        break;

      case SEQ_MOVE_CW:
        // After moving clockwise, move counter-clockwise back to home
        sequenceStep = SEQ_MOVE_CCW;
        Serial.print("Sequence: Moving counter-clockwise ");
        Serial.print(sequenceMaxSteps);
        Serial.println(" steps");
        testCounterClockwise(sequenceMaxSteps, sequenceSpeed);
        break;

      case SEQ_MOVE_CCW:
        // Sequence completed
        sequenceStep = SEQ_FINISHED;
        currentTest = TEST_IDLE;
        Serial.println("Test sequence completed successfully");
        break;

      default:
        break;
    }
  }

  // Periodically update status
  if (millis() - lastStatusUpdateTime > STATUS_UPDATE_INTERVAL) {
    lastStatusUpdateTime = millis();
    printStatus();
  }

  // Process serial commands if available
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("cw,")) {
      // Parse clockwise command: cw,steps,speed
      int firstComma = command.indexOf(',');
      int secondComma = command.indexOf(',', firstComma + 1);

      if (secondComma != -1) {
        long steps = command.substring(firstComma + 1, secondComma).toInt();
        float speed = command.substring(secondComma + 1).toFloat();
        testClockwise(steps, speed);
      }
    } else if (command.startsWith("ccw,")) {
      // Parse counter-clockwise command: ccw,steps,speed
      int firstComma = command.indexOf(',');
      int secondComma = command.indexOf(',', firstComma + 1);

      if (secondComma != -1) {
        long steps = command.substring(firstComma + 1, secondComma).toInt();
        float speed = command.substring(secondComma + 1).toFloat();
        testCounterClockwise(steps, speed);
      }
    } else if (command == "home") {
      testHoming();
    } else if (command.startsWith("back_forth,")) {
      // Parse back and forth command: back_forth,cycles,distance,speed
      int firstComma = command.indexOf(',');
      int secondComma = command.indexOf(',', firstComma + 1);
      int thirdComma = command.indexOf(',', secondComma + 1);

      if (thirdComma != -1) {
        int cycles = command.substring(firstComma + 1, secondComma).toInt();
        long distance = command.substring(secondComma + 1, thirdComma).toInt();
        float speed = command.substring(thirdComma + 1).toFloat();
        testBackAndForth(cycles, distance, speed);
      }
    } else if (command.startsWith("sequence,")) {
      // Parse sequence command: sequence,maxSteps,speed
      int firstComma = command.indexOf(',');
      int secondComma = command.indexOf(',', firstComma + 1);

      if (secondComma != -1) {
        int maxSteps = command.substring(firstComma + 1, secondComma).toInt();
        float speed = command.substring(secondComma + 1).toFloat();
        testSequence(maxSteps, speed);
      } else {
        // Default safe values if parsing fails
        testSequence(500, 500);
      }
    } else if (command == "stop") {
      stop();
    } else if (command == "status") {
      printStatus();
    }
  }
}

void StepperFunctionTest::testClockwise(long steps, float speed) {
  currentTest = TEST_CLOCKWISE;
  stepper.setMaxSpeed(speed);
  stepper.move(steps);
  Serial.print("Moving clockwise: ");
  Serial.print(steps);
  Serial.print(" steps at speed ");
  Serial.println(speed);
}

void StepperFunctionTest::testCounterClockwise(long steps, float speed) {
  currentTest = TEST_COUNTERCLOCKWISE;
  stepper.setMaxSpeed(speed);
  stepper.move(-steps);
  Serial.print("Moving counter-clockwise: ");
  Serial.print(steps);
  Serial.print(" steps at speed ");
  Serial.println(speed);
}

void StepperFunctionTest::testHoming() {
  currentTest = TEST_HOMING;
  Serial.println("Starting homing sequence");
  performHoming();
  currentTest = TEST_IDLE;
}

void StepperFunctionTest::performHoming() {
  Serial.println("Starting homing sequence");

  // Save current speed and acceleration settings
  float originalSpeed = stepper.maxSpeed();
  float originalAccel = stepper.acceleration();

  // Set homing speed and acceleration
  stepper.setMaxSpeed(homingSpeed);
  stepper.setAcceleration(homingAcceleration);

  long distance = 0;
  int count = 20000;  // Max steps to move during homing

  // Check if we're already at the sensor
  if (digitalRead(sensorPin) == HIGH) {
    Serial.println("Already in sensor area, moving out first");

    // Move away from sensor
    stepper.move(count);
    do {
      stepper.run();
    } while (digitalRead(sensorPin) == HIGH && stepper.distanceToGo() != 0);

    if (stepper.distanceToGo() != 0) {
      // We moved away from the sensor
      stepper.stop();
      stepper.setCurrentPosition(stepper.currentPosition());

      Serial.println("Moving back to sensor");
      stepper.move(-count);
      do {
        stepper.run();
      } while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0);
    }
  } else {
    Serial.println("Outside sensor area, moving to sensor");

    // Move toward sensor
    stepper.move(-count);
    do {
      stepper.run();
    } while (digitalRead(sensorPin) == LOW && stepper.distanceToGo() != 0);
  }

  stepper.stop();
  Serial.println("Sensor detected");

  // Record how much further we would have moved
  distance = stepper.distanceToGo();
  stepper.runToPosition();

  // Correct overshoot
  Serial.print("Correcting overshot by ");
  Serial.print(distance);
  Serial.println(" steps");
  stepper.move(-distance);
  stepper.runToPosition();

  // Set this position as home (0)
  Serial.println("Setting home position (0)");
  stepper.setCurrentPosition(0);

  // Restore original speed and acceleration
  stepper.setMaxSpeed(originalSpeed);
  stepper.setAcceleration(originalAccel);

  Serial.println("Homing completed");
}

void StepperFunctionTest::testSequence(int maxSteps, float speed) {
  // Make sure values are reasonable
  if (maxSteps <= 0) maxSteps = 500;  // Default safe value
  if (speed <= 0) speed = 500;        // Default safe value

  currentTest = TEST_SEQUENCE;
  Serial.print("Running test sequence with max steps: ");
  Serial.print(maxSteps);
  Serial.print(", speed: ");
  Serial.println(speed);

  // Store sequence parameters
  sequenceMaxSteps = maxSteps;
  sequenceSpeed = speed;
  sequenceStep = SEQ_START;

  // The actual sequence execution will be handled in the update() method
  // This approach is completely non-blocking
}

void StepperFunctionTest::testBackAndForth(int cycles, long distance, float speed) {
  currentTest = TEST_BACK_AND_FORTH;
  cyclesTotal = cycles;
  currentCycle = 0;
  moveDistance = distance;
  movingForward = true;

  Serial.print("Starting back and forth test: ");
  Serial.print(cycles);
  Serial.print(" cycles, ");
  Serial.print(distance);
  Serial.print(" steps, at speed ");
  Serial.println(speed);

  stepper.setMaxSpeed(speed);
  stepper.move(moveDistance);
}

void StepperFunctionTest::stop() {
  stepper.stop();
  currentTest = TEST_IDLE;
  Serial.println("Stopped all motion");
}

void StepperFunctionTest::setMaxSpeed(float speed) {
  maxSpeed = speed;
  stepper.setMaxSpeed(speed);
  Serial.print("Max speed set to: ");
  Serial.println(speed);
}

void StepperFunctionTest::setAcceleration(float accel) {
  acceleration = accel;
  stepper.setAcceleration(accel);
  Serial.print("Acceleration set to: ");
  Serial.println(accel);
}

long StepperFunctionTest::getCurrentPosition() {
  return stepper.currentPosition();
}

long StepperFunctionTest::getTargetPosition() {
  return stepper.targetPosition();
}

bool StepperFunctionTest::isMoving() {
  return stepper.isRunning();
}

bool StepperFunctionTest::isSensorActive() {
  return digitalRead(sensorPin) == HIGH;
}

void StepperFunctionTest::printStatus() {
  Serial.print("Position: ");
  Serial.print(stepper.currentPosition());
  Serial.print(" Target: ");
  Serial.print(stepper.targetPosition());
  Serial.print(" Speed: ");
  Serial.print(stepper.speed());
  Serial.print(" Sensor: ");
  Serial.print(digitalRead(sensorPin) ? "ACTIVE" : "INACTIVE");
  Serial.print(" Test: ");

  switch (currentTest) {
    case TEST_IDLE: Serial.println("IDLE"); break;
    case TEST_CLOCKWISE: Serial.println("CLOCKWISE"); break;
    case TEST_COUNTERCLOCKWISE: Serial.println("COUNTERCLOCKWISE"); break;
    case TEST_HOMING: Serial.println("HOMING"); break;
    case TEST_BACK_AND_FORTH:
      Serial.print("BACK_AND_FORTH (Cycle ");
      Serial.print(currentCycle + 1);
      Serial.print(" of ");
      Serial.print(cyclesTotal);
      Serial.println(")");
      break;
    case TEST_SEQUENCE: Serial.println("SEQUENCE"); break;
  }
}