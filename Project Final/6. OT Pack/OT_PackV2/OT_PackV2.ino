#include <AccelStepper.h>

// Hardware Pin Assignments
const byte SENSOR_INPUT_PIN = 3;
const byte STEPPER_STEP_PIN = 10;
const byte STEPPER_ENABLE_PIN = 9;
const byte STEPPER_DIRECTION_PIN = 8;

// Motor Configuration Constants
const int MICROSTEPPING_RESOLUTION = 4;
const int STEPS_PER_REVOLUTION = 58;
const int TOTAL_STEPS = STEPS_PER_REVOLUTION * MICROSTEPPING_RESOLUTION;

// Motion Profile Constants
const float FORWARD_MAX_SPEED = 1200.0;
const float FORWARD_ACCELERATION = 600.0;
const float REVERSE_MAX_SPEED = 3000.0;
const float REVERSE_ACCELERATION = 1900.0;

// Timing Constants
const int FORWARD_DEBOUNCE_DELAY = 150;
const int REVERSE_DEBOUNCE_DELAY = 250;
const int REVERSE_SETTLE_DELAY = 100;
const int POSITION_OFFSET = 2;

// System States
enum PackingState {
  WAITING_FOR_FORWARD,
  WAITING_FOR_REVERSE
};

// Global Variables
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIRECTION_PIN);
PackingState currentState = WAITING_FOR_FORWARD;

void setup() {
  Serial.begin(9600);
  
  initializePins();
  initializeMotor();
  
  Serial.println("OT Pack V2 - Initialized");
  Serial.println("Waiting for sensor trigger...");
}

void initializePins() {
  pinMode(SENSOR_INPUT_PIN, INPUT_PULLUP);
  pinMode(STEPPER_STEP_PIN, OUTPUT);
  pinMode(STEPPER_ENABLE_PIN, OUTPUT);
  pinMode(STEPPER_DIRECTION_PIN, OUTPUT);
  
  digitalWrite(STEPPER_ENABLE_PIN, LOW);
}

void initializeMotor() {
  stepper.setMaxSpeed(FORWARD_MAX_SPEED * MICROSTEPPING_RESOLUTION);
  stepper.setAcceleration(FORWARD_ACCELERATION * MICROSTEPPING_RESOLUTION);
}

void loop() {
  bool sensorTriggered = digitalRead(SENSOR_INPUT_PIN) == HIGH;
  
  printSensorStatus(sensorTriggered);
  
  switch (currentState) {
    case WAITING_FOR_FORWARD:
      if (sensorTriggered) {
        executeForwardMotion();
        currentState = WAITING_FOR_REVERSE;
      }
      break;
      
    case WAITING_FOR_REVERSE:
      if (!sensorTriggered) {
        executeReverseMotion();
        currentState = WAITING_FOR_FORWARD;
      }
      break;
  }
}

void printSensorStatus(bool triggered) {
  Serial.print("Sensor Status: ");
  Serial.print(triggered ? "TRIGGERED" : "CLEAR");
  Serial.print(" | State: ");
  Serial.println(currentState == WAITING_FOR_FORWARD ? "WAITING_FORWARD" : "WAITING_REVERSE");
}

void executeForwardMotion() {
  Serial.println("Executing Forward Motion...");
  
  configureMotorForForward();
  delay(FORWARD_DEBOUNCE_DELAY);
  
  enableMotor();
  stepper.move(TOTAL_STEPS);
  stepper.runToPosition();
  
  Serial.println("Forward Motion Complete");
}

void executeReverseMotion() {
  Serial.println("Executing Reverse Motion...");
  
  configureMotorForReverse();
  delay(REVERSE_DEBOUNCE_DELAY);
  
  stepper.move(-(TOTAL_STEPS - POSITION_OFFSET));
  stepper.runToPosition();
  
  delay(REVERSE_SETTLE_DELAY);
  disableMotor();
  
  Serial.println("Reverse Motion Complete");
}

void configureMotorForForward() {
  stepper.setMaxSpeed(FORWARD_MAX_SPEED * MICROSTEPPING_RESOLUTION);
  stepper.setAcceleration(FORWARD_ACCELERATION * MICROSTEPPING_RESOLUTION);
}

void configureMotorForReverse() {
  stepper.setMaxSpeed(REVERSE_MAX_SPEED * MICROSTEPPING_RESOLUTION);
  stepper.setAcceleration(REVERSE_ACCELERATION * MICROSTEPPING_RESOLUTION);
}

void enableMotor() {
  digitalWrite(STEPPER_ENABLE_PIN, HIGH);
}

void disableMotor() {
  digitalWrite(STEPPER_ENABLE_PIN, LOW);
}
