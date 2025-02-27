#include "Kinematrix.h"
#include "SoftwareSerial.h"
#include "AccelStepper.h"

// Communication setup
SoftwareSerial masterComm(10, 11);  // RX, TX pins for communication with master

// Motor configuration
#define MOTOR_PIN1 4
#define MOTOR_PIN2 5
#define MOTOR_PIN3 6
#define MOTOR_PIN4 7
// Using AccelStepper with 4 pins in FULL4WIRE mode (4 pins, step by step)
AccelStepper stepper(AccelStepper::FULL4WIRE, MOTOR_PIN1, MOTOR_PIN3, MOTOR_PIN2, MOTOR_PIN4);

// Configuration
const char SLAVE_ID = 'x';         // Change this to match the specific slave: 'x', 'y', 'z', 't', or 'g'
const float MAX_SPEED = 1000.0;    // Maximum speed in steps per second
const float ACCELERATION = 500.0;  // Acceleration in steps per second per second

// Command definitions (must match master)
enum Command {
  CMD_NONE,
  CMD_START,
  CMD_ZERO,
  CMD_PAUSE,
  CMD_RESUME,
  CMD_RESET
};

// State variables
bool isPaused = false;
bool isHoming = false;

void setup() {
  Serial.begin(9600);  // For debugging
  masterComm.begin(9600);

  // Initialize AccelStepper
  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);

  Serial.print("Slave ");
  Serial.print(SLAVE_ID);
  Serial.println(" initialized");
}

void loop() {
  // Check for incoming commands
  if (masterComm.available()) {
    String data = masterComm.readStringUntil('\n');
    data.trim();

    // Parse the command
    processCommand(data);
  }

  // Run the stepper motor (only moves if a motion is queued)
  if (!isPaused) {
    stepper.run();
  }
}

void processCommand(const String& data) {
  // Log the received command for debugging
  Serial.print("Received: ");
  Serial.println(data);

  // Parse the command format: slaveId;commandCode;params
  int firstSeparator = data.indexOf(';');
  if (firstSeparator == -1) return;

  // Extract the slave ID
  String slaveId = data.substring(0, firstSeparator);
  slaveId.toLowerCase();

  // Check if this command is intended for this slave
  // 'x' means any slave should process it
  if (slaveId != String(SLAVE_ID) && slaveId != "x") {
    return;
  }

  // Extract the command code
  int secondSeparator = data.indexOf(';', firstSeparator + 1);
  int cmdCode = -1;

  if (secondSeparator == -1) {
    // No parameters, just a command
    cmdCode = data.substring(firstSeparator + 1).toInt();
  } else {
    cmdCode = data.substring(firstSeparator + 1, secondSeparator).toInt();
  }

  // Process based on the command
  switch (cmdCode) {
    case CMD_ZERO:
      handleZeroCommand();
      break;

    case CMD_PAUSE:
      handlePauseCommand();
      break;

    case CMD_RESUME:
      handleResumeCommand();
      break;

    case CMD_RESET:
      handleResetCommand();
      break;

    case CMD_START:
      if (secondSeparator != -1) {
        // Parse parameters for movement
        String params = data.substring(secondSeparator + 1);
        handleMoveCommand(params);
      }
      break;

    default:
      Serial.println("Unknown command");
      break;
  }
}

void handleZeroCommand() {
  Serial.println("Executing ZERO");
  isPaused = false;

  // In a real implementation, you would implement homing with a limit switch
  // For now, we'll simply set the current position as home (0)
  stepper.setCurrentPosition(0);

  sendFeedback("ZERO DONE");
}

void handlePauseCommand() {
  Serial.println("Executing PAUSE");
  isPaused = true;
  sendFeedback("PAUSE DONE");
}

void handleResumeCommand() {
  Serial.println("Executing RESUME");
  isPaused = false;
  sendFeedback("RESUME DONE");
}

void handleResetCommand() {
  Serial.println("Executing RESET");
  isPaused = false;
  stepper.stop();  // Stop with deceleration
  sendFeedback("RESET DONE");
}

void handleMoveCommand(const String& params) {
  // Format for movement: "position,speed"
  int commaPos = params.indexOf(',');

  long newTarget = 0;
  float newSpeed = 0;

  if (commaPos == -1) {
    // Just position provided
    newTarget = params.toInt();
    newSpeed = MAX_SPEED;
  } else {
    // Position and speed
    newTarget = params.substring(0, commaPos).toInt();
    newSpeed = params.substring(commaPos + 1).toFloat();

    // If speed is not specified or invalid, use the default
    if (newSpeed <= 0) {
      newSpeed = MAX_SPEED;
    }
  }

  // Set new movement parameters
  stepper.setMaxSpeed(newSpeed);
  stepper.moveTo(newTarget);

  Serial.print("Moving to position: ");
  Serial.print(newTarget);
  Serial.print(", Speed: ");
  Serial.println(newSpeed);

  sendFeedback("MOVING");
}

void sendFeedback(const String& message) {
  // Send feedback to master
  masterComm.print(SLAVE_ID);
  masterComm.print(";");
  masterComm.println(message);

  // Also print to serial for debugging
  Serial.print("Feedback: ");
  Serial.print(SLAVE_ID);
  Serial.print(";");
  Serial.println(message);
}

// This function can be called from loop() to check if we've reached target position
void checkPositionReached() {
  if (!isPaused && !stepper.isRunning() && stepper.distanceToGo() == 0) {
    // We've reached the target position
    sendFeedback("POSITION REACHED");
  }
}