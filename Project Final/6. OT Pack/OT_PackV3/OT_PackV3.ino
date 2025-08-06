#include <AccelStepper.h>

// Pin Definitions
const byte sensorPin = 3;
const byte stepPin = 10;
const byte enablePin = 9;
const byte directionPin = 8;

// Motor Configuration
const int microsteppingResolution = 4;
AccelStepper stepper(AccelStepper::DRIVER, stepPin, directionPin);
int stepsPerRevolution = 58 * microsteppingResolution;
bool isExtended = false;

void setup() {
  Serial.begin(9600);
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(stepPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(directionPin, OUTPUT);
}

void loop() {
  Serial.print("| sensorPin: ");
  Serial.print(digitalRead(3));
  Serial.println();

  if (digitalRead(sensorPin) == HIGH && !isExtended) {
    stepper.setMaxSpeed(1200.0 * microsteppingResolution);
    stepper.setAcceleration(600.0 * microsteppingResolution);
    delay(150);
    stepper.move(stepsPerRevolution);
    digitalWrite(enablePin, HIGH);
    stepper.runToPosition();
    isExtended = true;
  }
  if (digitalRead(sensorPin) == LOW && isExtended) {
    stepper.setMaxSpeed(3000.0 * microsteppingResolution);
    stepper.setAcceleration(1900.0 * microsteppingResolution);
    delay(250);
    stepper.move(-stepsPerRevolution + 2);
    stepper.runToPosition();
    delay(100);
    digitalWrite(enablePin, LOW);
    isExtended = false;
  }
}
