#define ENABLE_MODULE_SERIAL_ENHANCED
#include "Kinematrix.h"
#include "SoftwareSerial.h"
#include "AccelStepper.h"

SoftwareSerial masterCommSerial(10, 11);

EnhancedSerial masterSerial;
EnhancedSerial debugSerial;

#define MOTOR_PIN1 4
#define MOTOR_PIN2 5
#define MOTOR_PIN3 6
#define MOTOR_PIN4 7
AccelStepper stepper(AccelStepper::FULL4WIRE, MOTOR_PIN1, MOTOR_PIN3, MOTOR_PIN2, MOTOR_PIN4);

const char SLAVE_ID = 'x';  // Change this for each slave: 'x', 'y', 'z', 't', 'g'
const float MAX_SPEED = 1000.0;
const float ACCELERATION = 500.0;

enum Command {
  CMD_NONE,
  CMD_START,
  CMD_ZERO,
  CMD_PAUSE,
  CMD_RESUME,
  CMD_RESET
};

struct MotionStep {
  long position;
  float speed;
  unsigned long delayBeforeMove;
  bool completed;
  bool isDelayOnly;  // Flag to indicate this is a delay-only step
};

bool isPaused = false;
bool isHoming = false;
unsigned long delayStartTime = 0;
bool isDelaying = false;
bool hasReportedCompletion = true;

#define MAX_MOTIONS 5
MotionStep motionQueue[MAX_MOTIONS];
int currentMotionIndex = 0;
int queuedMotionsCount = 0;

void setup() {
  Serial.begin(9600);            // Hardware serial for debugging
  masterCommSerial.begin(9600);  // Software serial for master communication

  masterSerial.begin(&Serial);  // masterCommSerial x;1;d500,3700
  debugSerial.begin(&Serial);

  masterSerial.setDataCallback(onMasterData);

  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);

  debugSerial.println("SLAVE " + String(SLAVE_ID) + ": System initialized");
}

void loop() {
  masterSerial.checkCallback();

  handleMotion();

  if (!isPaused && !isDelaying) {
    stepper.run();
  }

  checkPositionReached();
}

void startNextMotion() {
  if (currentMotionIndex < queuedMotionsCount) {
    if (motionQueue[currentMotionIndex].delayBeforeMove > 0) {
      // This step includes a delay
      isDelaying = true;
      delayStartTime = millis();
      sendFeedback("DELAYING");
    } else if (motionQueue[currentMotionIndex].isDelayOnly) {
      // This is a delay-only step, but with 0 delay time - skip to next
      currentMotionIndex++;
      startNextMotion();  // Move to the next step
    } else {
      // This is a movement step
      stepper.setMaxSpeed(motionQueue[currentMotionIndex].speed);
      stepper.moveTo(motionQueue[currentMotionIndex].position);
      sendFeedback("MOVING");
    }
  }
}

void handleMotion() {
  if (queuedMotionsCount == 0) return;

  if (isDelaying) {
    if (millis() - delayStartTime >= motionQueue[currentMotionIndex].delayBeforeMove) {
      isDelaying = false;

      if (motionQueue[currentMotionIndex].isDelayOnly) {
        // This was just a delay step, move to the next
        currentMotionIndex++;
        startNextMotion();
      } else {
        // This was a delay before move, now execute the move
        stepper.setMaxSpeed(motionQueue[currentMotionIndex].speed);
        stepper.moveTo(motionQueue[currentMotionIndex].position);
        sendFeedback("MOVING");
      }
    }
    return;
  }

  if (stepper.distanceToGo() == 0 && !stepper.isRunning() && !isDelaying && !isPaused) {
    if (currentMotionIndex < queuedMotionsCount - 1) {
      currentMotionIndex++;
      startNextMotion();
    } else if (currentMotionIndex == queuedMotionsCount - 1 && !hasReportedCompletion) {
      sendFeedback("SEQUENCE COMPLETED");
      hasReportedCompletion = true;
    }
  }
}

void onMasterData(const String& data) {
  debugSerial.println("MASTER→SLAVE: " + data);
  processCommand(data);
}

void processCommand(const String& data) {
  int firstSeparator = data.indexOf(';');
  if (firstSeparator == -1) return;

  String slaveId = data.substring(0, firstSeparator);
  slaveId.toLowerCase();

  // Process only if command is for this slave or for all ('x')
  if (slaveId != String(SLAVE_ID) && slaveId != "all") return;

  int secondSeparator = data.indexOf(';', firstSeparator + 1);
  int cmdCode = (secondSeparator == -1)
                  ? data.substring(firstSeparator + 1).toInt()
                  : data.substring(firstSeparator + 1, secondSeparator).toInt();

  debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Processing command " + String(cmdCode));

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
        handleMoveCommand(data.substring(secondSeparator + 1));
      }
      break;
    default:
      debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Unknown command " + String(cmdCode));
      break;
  }
}

void handleZeroCommand() {
  debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Executing ZERO");
  isPaused = false;
  isDelaying = false;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;
  stepper.stop();
  stepper.setCurrentPosition(0);
  sendFeedback("ZERO DONE");
}

void handlePauseCommand() {
  debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Executing PAUSE");
  isPaused = true;
  sendFeedback("PAUSE DONE");
}

void handleResumeCommand() {
  debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Executing RESUME");
  isPaused = false;
  sendFeedback("RESUME DONE");
}

void handleResetCommand() {
  debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Executing RESET");
  isPaused = false;
  isDelaying = false;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;
  stepper.stop();
  sendFeedback("RESET DONE");
}

void parsePositionSequence(const String& params) {
  debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Parsing position sequence: " + params);

  int semicolonPos = -1;
  int startPos = 0;
  queuedMotionsCount = 0;
  currentMotionIndex = 0;

  do {
    semicolonPos = params.indexOf(';', startPos);
    String param = (semicolonPos == -1) ? params.substring(startPos) : params.substring(startPos, semicolonPos);
    param.trim();

    debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Parsing parameter: " + param);

    if (queuedMotionsCount < MAX_MOTIONS) {
      // Initialize the queue entry with default values
      motionQueue[queuedMotionsCount].speed = MAX_SPEED;
      motionQueue[queuedMotionsCount].completed = false;

      if (param.startsWith("d")) {
        // It's a delay command - just delay without moving
        unsigned long delayValue = param.substring(1).toInt();
        motionQueue[queuedMotionsCount].delayBeforeMove = delayValue;
        motionQueue[queuedMotionsCount].position = 0;        // This value won't be used
        motionQueue[queuedMotionsCount].isDelayOnly = true;  // Mark as delay-only

        debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Queueing delay " + String(queuedMotionsCount) + " - Delay: " + String(delayValue) + "ms");
      } else {
        // It's a position command - set position and no delay
        long position = param.toInt();
        motionQueue[queuedMotionsCount].position = position;
        motionQueue[queuedMotionsCount].delayBeforeMove = 0;  // No delay
        motionQueue[queuedMotionsCount].isDelayOnly = false;  // Not a delay-only step

        debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Queueing position " + String(queuedMotionsCount) + " - Pos: " + String(position));
      }

      queuedMotionsCount++;
    }

    startPos = semicolonPos + 1;
  } while (semicolonPos != -1 && startPos < params.length() && queuedMotionsCount < MAX_MOTIONS);

  debugSerial.println("SLAVE " + String(SLAVE_ID) + ": Total queued motions: " + String(queuedMotionsCount));

  // Start executing the first motion if there are any queued
  if (queuedMotionsCount > 0) {
    hasReportedCompletion = false;
    startNextMotion();
  }
}

void handleMoveCommand(const String& params) {
  parsePositionSequence(params);
  hasReportedCompletion = false;
}

void sendFeedback(const String& message) {
  String feedback = String(SLAVE_ID) + ";" + message;
  masterSerial.println(feedback);
  debugSerial.println("SLAVE→MASTER: " + feedback);
}

void checkPositionReached() {
  static unsigned long lastReportTime = 0;
  static long lastReportedPosition = -999999;

  if ((millis() - lastReportTime > 2000) || (abs(stepper.currentPosition() - lastReportedPosition) > 100)) {

    if (stepper.isRunning() || abs(stepper.currentPosition() - lastReportedPosition) > 10) {
      lastReportTime = millis();
      lastReportedPosition = stepper.currentPosition();

      String positionUpdate = "POS:" + String(stepper.currentPosition()) + " TARGET:" + String(stepper.targetPosition());
      sendFeedback(positionUpdate);
    }
  }
}