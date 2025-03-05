#include <AccelStepper.h>

int clkPin = 10;
int cwPin = 11;
int enPin = 12;

AccelStepper stepper(AccelStepper::DRIVER, clkPin, cwPin);

void setup() {
  Serial.begin(9600);
  stepper.setMaxSpeed(1000.0);
  stepper.setAcceleration(500.0);
  stepper.setCurrentPosition(0);
  pinMode(6, INPUT_PULLUP);
  pinMode(13, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    int pos = Serial.readStringUntil('\n').toInt();
    Serial.print("| pos: ");
    Serial.print(pos);
    Serial.println();
    stepper.moveTo(pos);
  }

  int sensor = digitalRead(6);
  digitalWrite(13, sensor);
  int pos = stepper.currentPosition();
  Serial.print("| sensor: ");
  Serial.print(sensor);
  Serial.print("| pos: ");
  Serial.print(pos);
  Serial.println();
  stepper.run();
}

// X G Z T Y
// T Y Z X G
