#include <AccelStepper.h>

// Pin Definitions
const byte P1_Input = 3;
const byte P2_Output = 10;
const byte Enable = 9;
const byte Clockwise = 8;

// Motor Configuration
const int MicrosteppingResolution = 4;
AccelStepper stepper(AccelStepper::DRIVER, P2_Output, Clockwise);
int Steps = 58*MicrosteppingResolution; 
bool state = false;

void setup() {
  pinMode(P1_Input, INPUT_PULLUP);
  pinMode(P2_Output, OUTPUT);
  pinMode(Enable, OUTPUT);
  pinMode(Clockwise, OUTPUT); 
}

void loop() {
  if (digitalRead(P1_Input) == HIGH && !state) { 
    stepper.setMaxSpeed(1200.0*MicrosteppingResolution); 
    stepper.setAcceleration(600.0*MicrosteppingResolution);
    delay(150);
    stepper.move(Steps);
    digitalWrite(Enable, HIGH); 
    stepper.runToPosition(); 
    state = true;
  }
  if (digitalRead(P1_Input) == LOW && state) { 
    stepper.setMaxSpeed(3000.0*MicrosteppingResolution); 
    stepper.setAcceleration(1900.0*MicrosteppingResolution);
    delay(250);
    stepper.move(-Steps+2);
    stepper.runToPosition();
    delay(100);
    digitalWrite(Enable, LOW); 
    state = false;
  }
}
