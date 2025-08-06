#include "OTPack.h"

// Hardware Pin Assignments
const byte SENSOR_INPUT_PIN = 3;
const byte STEPPER_STEP_PIN = 10;
const byte STEPPER_ENABLE_PIN = 9;
const byte STEPPER_DIRECTION_PIN = 8;

// Create OTPack instance
OTPack otpack(SENSOR_INPUT_PIN, STEPPER_STEP_PIN, STEPPER_DIRECTION_PIN, STEPPER_ENABLE_PIN);

void setup() {
  Serial.begin(9600);

  // Initialize OTPack with default settings
  otpack.begin();

  // Optional: Customize settings
  otpack.setMotorConfig(58, 4);                        // 58 steps/rev, 4x microstepping
  otpack.setForwardProfile(1200.0, 600.0, 150);        // speed, accel, debounce
  otpack.setReverseProfile(3000.0, 1900.0, 250, 100);  // speed, accel, debounce, settle
  otpack.setPositionOffset(2);

  // Choose motion mode
  otpack.setMotionMode(OTPack::NON_BLOCKING);  // or OTPack::BLOCKING

  Serial.print("OT Pack V2 - Ready | Mode: ");
  Serial.println(otpack.getMotionMode() == OTPack::BLOCKING ? "BLOCKING" : "NON_BLOCKING");
}

void loop() {
  // Update OTPack - handles all sensor reading and motion control
  otpack.update();

  // Optional: Print status periodically
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {  // Print every 1 second
    printStatus();
    lastPrint = millis();
  }

  delay(10);  // Small delay for stability
}

void printStatus() {
  Serial.print("State: ");
  Serial.print(otpack.getState() == OTPack::WAITING_FORWARD ? "WAITING_FORWARD" : "WAITING_REVERSE");
  Serial.print(" | Mode: ");
  Serial.print(otpack.getMotionMode() == OTPack::BLOCKING ? "BLOCKING" : "NON_BLOCKING");
  Serial.print(" | Busy: ");
  Serial.print(otpack.isBusy() ? "YES" : "NO");
  Serial.print(" | Moving: ");
  Serial.println(otpack.isMoving() ? "YES" : "NO");
}

// Example function to switch between modes (call from serial input or button)
void toggleMotionMode() {
  if (otpack.getMotionMode() == OTPack::BLOCKING) {
    otpack.setMotionMode(OTPack::NON_BLOCKING);
    Serial.println("Switched to NON_BLOCKING mode");
  } else {
    otpack.setMotionMode(OTPack::BLOCKING);
    Serial.println("Switched to BLOCKING mode");
  }
}
