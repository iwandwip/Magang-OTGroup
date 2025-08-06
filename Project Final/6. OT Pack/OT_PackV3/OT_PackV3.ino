#include <AccelStepper.h>

// Pin Definitions
const byte SENSOR_PIN = 3;
const byte STEP_PIN = 10;
const byte ENABLE_PIN = 9;
const byte DIRECTION_PIN = 8;

// Motor Configuration
const int MICROSTEPPING_RESOLUTION = 4;
const int BASE_STEPS_PER_REV = 58;
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIRECTION_PIN);
int stepsPerRevolution = BASE_STEPS_PER_REV * MICROSTEPPING_RESOLUTION;
bool isExtended = false;

void setup() {
  Serial.begin(9600);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
}

void loop() {
  Serial.print("| SENSOR_PIN: ");
  Serial.print(digitalRead(SENSOR_PIN));
  Serial.println();

  if (digitalRead(SENSOR_PIN) == HIGH && !isExtended) {
    stepper.setMaxSpeed(1200.0 * MICROSTEPPING_RESOLUTION);
    stepper.setAcceleration(600.0 * MICROSTEPPING_RESOLUTION);
    delay(150);
    stepper.move(stepsPerRevolution);
    digitalWrite(ENABLE_PIN, HIGH);
    stepper.runToPosition();
    isExtended = true;
  }
  if (digitalRead(SENSOR_PIN) == LOW && isExtended) {
    stepper.setMaxSpeed(3000.0 * MICROSTEPPING_RESOLUTION);
    stepper.setAcceleration(1900.0 * MICROSTEPPING_RESOLUTION);
    delay(250);
    stepper.move(-stepsPerRevolution + 2);
    stepper.runToPosition();
    delay(100);
    digitalWrite(ENABLE_PIN, LOW);
    isExtended = false;
  }
}
