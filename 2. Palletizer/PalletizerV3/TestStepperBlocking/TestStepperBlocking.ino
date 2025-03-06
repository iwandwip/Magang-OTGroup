#include <AccelStepper.h>

int clkPin = 10;
int cwPin = 11;
int enPin = 12;

AccelStepper stepper(AccelStepper::DRIVER, clkPin, cwPin);

void setup() {
  Serial.begin(9600);
  stepper.setMaxSpeed(2000.0);
  stepper.setAcceleration(1000.0);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // pinMode(clkPin, OUTPUT);
  // pinMode(cwPin, OUTPUT);

  // digitalWrite(clkPin, LOW);
  // digitalWrite(cwPin, LOW);
}

void loop() {
  if (Serial.available()) {
    digitalWrite(LED_BUILTIN, LOW);
    int pos = Serial.readStringUntil('\n').toInt();
    Serial.print("| pos: ");
    Serial.print(pos);
    Serial.println();

    stepper.moveTo(pos);
    // stepper.move(pos);
    stepper.runToPosition();

    Serial.println("| done");
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
