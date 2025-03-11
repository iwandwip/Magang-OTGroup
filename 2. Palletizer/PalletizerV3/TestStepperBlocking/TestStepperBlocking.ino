#include <AccelStepper.h>

int clkPin = 10;
int cwPin = 11;
// int enPin = 12;
int brakePin = 7;

AccelStepper stepper(AccelStepper::DRIVER, clkPin, cwPin);

void setup() {
  Serial.begin(9600);
  stepper.setMaxSpeed(2000.0);
  stepper.setAcceleration(1000.0);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(brakePin, OUTPUT);
  // pinMode(enPin, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(brakePin, HIGH);
  // digitalWrite(enPin, LOW);
}

void loop() {
  if (Serial.available()) {
    // digitalWrite(enPin, HIGH);
    digitalWrite(brakePin, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    int pos = Serial.readStringUntil('\n').toInt();
    Serial.print("| pos: ");
    Serial.print(pos);
    Serial.println();

    stepper.moveTo(pos);
    stepper.runToPosition();

    delay(1500);
    // digitalWrite(enPin, LOW);
    digitalWrite(brakePin, HIGH);

    Serial.println("| done");
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
