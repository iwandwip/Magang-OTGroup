// ========================================
// OT_PackV3.ino - Main Core Logic
// Arduino stepper motor control system
// ========================================

#include <AccelStepper.h>
#include <EEPROM.h>
#include "Constants.h"

// Motor Configuration
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIRECTION_PIN);
int stepsPerRevolution = BASE_STEPS_PER_REV * MICROSTEPPING_RESOLUTION;
bool isExtended = false;

// Debug Mode
bool debugMode = false;  // Debug mode toggle for status output

// Operation Mode and Sensor Simulation
int operationMode = MODE_NORMAL;  // Current operation mode (normal/testing)
int sensorValue = HIGH;           // Virtual sensor value for testing mode

// Configurable Motion Parameters (Initialized with defaults from Constants.h)
float extendMaxSpeed = DEFAULT_EXTEND_MAX_SPEED;
float extendAcceleration = DEFAULT_EXTEND_ACCELERATION;
int extendDelayBefore = DEFAULT_EXTEND_DELAY_BEFORE;

float retractMaxSpeed = DEFAULT_RETRACT_MAX_SPEED;
float retractAcceleration = DEFAULT_RETRACT_ACCELERATION;
int retractDelayBefore = DEFAULT_RETRACT_DELAY_BEFORE;
int retractDelayAfter = DEFAULT_RETRACT_DELAY_AFTER;
int retractStepAdjustment = DEFAULT_RETRACT_STEP_ADJUSTMENT;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);

  Serial.print(F(SYSTEM_NAME));
  Serial.print(F(" v"));
  Serial.print(F(VERSION));
  Serial.println(F(" - Serial Commander Ready"));
  Serial.println(F("Loading configuration from EEPROM..."));
  loadFromEEPROM();
  Serial.println(F("Type HELP for available commands"));
}

void loop() {
  // Check for serial commands
  serialCommander();

  // Display sensor status (only when debug mode is enabled)
  if (debugMode) {
    static unsigned long lastStatusTime = 0;
    if (millis() - lastStatusTime > STATUS_UPDATE_INTERVAL) {
      int currentSensor = getSensorReading();
      Serial.print(F("| SENSOR: "));
      Serial.print(currentSensor);
      Serial.print(operationMode == MODE_TESTING ? F(" (SIM)") : F(" (REAL)"));
      Serial.print(F(" | State: "));
      Serial.println(isExtended ? F("Extended") : F("Retracted"));
      lastStatusTime = millis();
    }
  }

  // Main motion control logic with interrupt protection
  noInterrupts();  // Disable interrupts during critical sensor reading
  int currentSensorReading = getSensorReading();
  interrupts();    // Re-enable interrupts
  
  if (currentSensorReading == HIGH && !isExtended) {
    performSmoothExtendMotion();
  }

  if (currentSensorReading == LOW && isExtended) {
    performSmoothRetractMotion();
  }
}

// Legacy motion functions for backward compatibility
void performExtendMotion() {
  performSmoothExtendMotion();
}

void performRetractMotion() {
  performSmoothRetractMotion();
}