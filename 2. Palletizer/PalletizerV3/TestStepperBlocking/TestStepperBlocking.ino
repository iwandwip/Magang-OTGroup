#include <AccelStepper.h>

int clkPin = 10;
int cwPin = 11;
int enPin = 12;

AccelStepper stepper(AccelStepper::DRIVER, clkPin, cwPin);

void setup() {
  Serial.begin(9600);
  stepper.setMaxSpeed(200.0);
  stepper.setAcceleration(100.0);
}

void loop() {
  if (Serial.available()) {
    int pos = Serial.readStringUntil('\n').toInt();
    Serial.print("| pos: ");
    Serial.print(pos);
    Serial.println();

    stepper.moveTo(pos);
    stepper.runToPosition();
  }
}
