#include <AccelStepper.h>
#include <EEPROM.h>
#include "Constants.h"

// Motor Configuration
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIRECTION_PIN);
int stepsPerRevolution = BASE_STEPS_PER_REV * MICROSTEPPING_RESOLUTION;
bool isExtended = false;

// Debug Mode
bool debugMode = false;  // Debug mode toggle for status output

// Configurable Motion Parameters (Initialized with defaults from Constants.h)
float extendMaxSpeed = DEFAULT_EXTEND_MAX_SPEED;
float extendAcceleration = DEFAULT_EXTEND_ACCELERATION;
int extendDelayBefore = DEFAULT_EXTEND_DELAY_BEFORE;

float retractMaxSpeed = DEFAULT_RETRACT_MAX_SPEED;
float retractAcceleration = DEFAULT_RETRACT_ACCELERATION;
int retractDelayBefore = DEFAULT_RETRACT_DELAY_BEFORE;
int retractDelayAfter = DEFAULT_RETRACT_DELAY_AFTER;
int retractStepAdjustment = DEFAULT_RETRACT_STEP_ADJUSTMENT;

// EEPROM Functions
void saveToEEPROM() {
  // Save signature first
  EEPROM.put(EEPROM_SIGNATURE_ADDR, EEPROM_SIGNATURE);

  // Save all parameters
  EEPROM.put(EXTEND_SPEED_ADDR, extendMaxSpeed);
  EEPROM.put(EXTEND_ACCEL_ADDR, extendAcceleration);
  EEPROM.put(EXTEND_DELAY_ADDR, extendDelayBefore);
  EEPROM.put(RETRACT_SPEED_ADDR, retractMaxSpeed);
  EEPROM.put(RETRACT_ACCEL_ADDR, retractAcceleration);
  EEPROM.put(RETRACT_DELAY_BEFORE_ADDR, retractDelayBefore);
  EEPROM.put(RETRACT_DELAY_AFTER_ADDR, retractDelayAfter);
  EEPROM.put(RETRACT_ADJUSTMENT_ADDR, retractStepAdjustment);

  Serial.println(F("Configuration saved to EEPROM"));
}

void loadFromEEPROM() {
  uint32_t signature;
  EEPROM.get(EEPROM_SIGNATURE_ADDR, signature);

  if (signature == EEPROM_SIGNATURE) {
    // Load all parameters
    EEPROM.get(EXTEND_SPEED_ADDR, extendMaxSpeed);
    EEPROM.get(EXTEND_ACCEL_ADDR, extendAcceleration);
    EEPROM.get(EXTEND_DELAY_ADDR, extendDelayBefore);
    EEPROM.get(RETRACT_SPEED_ADDR, retractMaxSpeed);
    EEPROM.get(RETRACT_ACCEL_ADDR, retractAcceleration);
    EEPROM.get(RETRACT_DELAY_BEFORE_ADDR, retractDelayBefore);
    EEPROM.get(RETRACT_DELAY_AFTER_ADDR, retractDelayAfter);
    EEPROM.get(RETRACT_ADJUSTMENT_ADDR, retractStepAdjustment);

    Serial.println(F("Configuration loaded from EEPROM"));
  } else {
    Serial.println(F("EEPROM not initialized, using default values"));
    saveToEEPROM();  // Save defaults to EEPROM
  }
}

void resetEEPROM() {
  // Reset to default values from Constants.h
  extendMaxSpeed = DEFAULT_EXTEND_MAX_SPEED;
  extendAcceleration = DEFAULT_EXTEND_ACCELERATION;
  extendDelayBefore = DEFAULT_EXTEND_DELAY_BEFORE;
  retractMaxSpeed = DEFAULT_RETRACT_MAX_SPEED;
  retractAcceleration = DEFAULT_RETRACT_ACCELERATION;
  retractDelayBefore = DEFAULT_RETRACT_DELAY_BEFORE;
  retractDelayAfter = DEFAULT_RETRACT_DELAY_AFTER;
  retractStepAdjustment = DEFAULT_RETRACT_STEP_ADJUSTMENT;

  // Save defaults to EEPROM
  saveToEEPROM();
  Serial.println(F("Configuration reset to defaults"));
}

void serialCommander() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("SET ")) {
      String parameter = command.substring(4);
      int equalIndex = parameter.indexOf('=');

      if (equalIndex > 0) {
        String paramName = parameter.substring(0, equalIndex);
        String paramValue = parameter.substring(equalIndex + 1);
        float value = paramValue.toFloat();

        paramName.toUpperCase();

        // Extend Motion Parameters
        if (paramName == "EXTEND_SPEED") {
          extendMaxSpeed = value;
          Serial.print(F("Extend speed set to: "));
          Serial.println(extendMaxSpeed);
          saveToEEPROM();
        } else if (paramName == "EXTEND_ACCEL") {
          extendAcceleration = value;
          Serial.print(F("Extend acceleration set to: "));
          Serial.println(extendAcceleration);
          saveToEEPROM();
        } else if (paramName == "EXTEND_DELAY") {
          extendDelayBefore = (int)value;
          Serial.print(F("Extend delay set to: "));
          Serial.print(extendDelayBefore);
          Serial.println(F("ms"));
          saveToEEPROM();
        }
        // Retract Motion Parameters
        else if (paramName == "RETRACT_SPEED") {
          retractMaxSpeed = value;
          Serial.print(F("Retract speed set to: "));
          Serial.println(retractMaxSpeed);
          saveToEEPROM();
        } else if (paramName == "RETRACT_ACCEL") {
          retractAcceleration = value;
          Serial.print(F("Retract acceleration set to: "));
          Serial.println(retractAcceleration);
          saveToEEPROM();
        } else if (paramName == "RETRACT_DELAY_BEFORE") {
          retractDelayBefore = (int)value;
          Serial.print(F("Retract delay before set to: "));
          Serial.print(retractDelayBefore);
          Serial.println(F("ms"));
          saveToEEPROM();
        } else if (paramName == "RETRACT_DELAY_AFTER") {
          retractDelayAfter = (int)value;
          Serial.print(F("Retract delay after set to: "));
          Serial.print(retractDelayAfter);
          Serial.println(F("ms"));
          saveToEEPROM();
        } else if (paramName == "RETRACT_ADJUSTMENT") {
          retractStepAdjustment = (int)value;
          Serial.print(F("Retract step adjustment set to: "));
          Serial.println(retractStepAdjustment);
          saveToEEPROM();
        } else {
          Serial.print(F("Unknown parameter: "));
          Serial.println(paramName);
        }
      } else {
        Serial.println(F("Invalid format. Use: SET PARAMETER=VALUE"));
      }
    } else if (command == "SHOW") {
      Serial.println(F("=== Current Configuration ==="));
      Serial.print(F("EXTEND_SPEED="));
      Serial.println(extendMaxSpeed);
      Serial.print(F("EXTEND_ACCEL="));
      Serial.println(extendAcceleration);
      Serial.print(F("EXTEND_DELAY="));
      Serial.println(extendDelayBefore);
      Serial.print(F("RETRACT_SPEED="));
      Serial.println(retractMaxSpeed);
      Serial.print(F("RETRACT_ACCEL="));
      Serial.println(retractAcceleration);
      Serial.print(F("RETRACT_DELAY_BEFORE="));
      Serial.println(retractDelayBefore);
      Serial.print(F("RETRACT_DELAY_AFTER="));
      Serial.println(retractDelayAfter);
      Serial.print(F("RETRACT_ADJUSTMENT="));
      Serial.println(retractStepAdjustment);
      Serial.println(F("============================="));
    } else if (command == "SAVE") {
      saveToEEPROM();
    } else if (command == "LOAD") {
      loadFromEEPROM();
    } else if (command == "RESET") {
      resetEEPROM();
    } else if (command == "DEBUG") {
      debugMode = !debugMode;  // Toggle debug mode
      Serial.print(F("Debug mode "));
      Serial.println(debugMode ? F("ENABLED") : F("DISABLED"));
    } else if (command == "HELP") {
      Serial.println(F("=== SerialCommander Help ==="));
      Serial.println(F("SET EXTEND_SPEED=value        - Set extend motion speed"));
      Serial.println(F("SET EXTEND_ACCEL=value        - Set extend acceleration"));
      Serial.println(F("SET EXTEND_DELAY=value        - Set extend delay (ms)"));
      Serial.println(F("SET RETRACT_SPEED=value       - Set retract motion speed"));
      Serial.println(F("SET RETRACT_ACCEL=value       - Set retract acceleration"));
      Serial.println(F("SET RETRACT_DELAY_BEFORE=val  - Set retract delay before (ms)"));
      Serial.println(F("SET RETRACT_DELAY_AFTER=val   - Set retract delay after (ms)"));
      Serial.println(F("SET RETRACT_ADJUSTMENT=val    - Set retract step adjustment"));
      Serial.println(F("SHOW                          - Show current configuration"));
      Serial.println(F("SAVE                          - Save current config to EEPROM"));
      Serial.println(F("LOAD                          - Load config from EEPROM"));
      Serial.println(F("RESET                         - Reset config to defaults"));
      Serial.println(F("DEBUG                         - Toggle debug status output"));
      Serial.println(F("HELP                          - Show this help"));
      Serial.println(F("============================"));
    } else {
      Serial.println(F("Unknown command. Type HELP for available commands."));
    }
  }
}

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
      Serial.print(F("| SENSOR_PIN: "));
      Serial.print(digitalRead(SENSOR_PIN));
      Serial.print(F(" | State: "));
      Serial.println(isExtended ? F("Extended") : F("Retracted"));
      lastStatusTime = millis();
    }
  }

  // Main motion control logic
  if (digitalRead(SENSOR_PIN) == HIGH && !isExtended) {
    Serial.println(F("Starting extend motion..."));
    stepper.setMaxSpeed(extendMaxSpeed * MICROSTEPPING_RESOLUTION);
    stepper.setAcceleration(extendAcceleration * MICROSTEPPING_RESOLUTION);
    delay(extendDelayBefore);
    stepper.move(stepsPerRevolution);
    digitalWrite(ENABLE_PIN, HIGH);
    stepper.runToPosition();
    isExtended = true;
    Serial.println(F("Extend motion completed."));
  }

  if (digitalRead(SENSOR_PIN) == LOW && isExtended) {
    Serial.println(F("Starting retract motion..."));
    stepper.setMaxSpeed(retractMaxSpeed * MICROSTEPPING_RESOLUTION);
    stepper.setAcceleration(retractAcceleration * MICROSTEPPING_RESOLUTION);
    delay(retractDelayBefore);
    stepper.move(-stepsPerRevolution + retractStepAdjustment);
    stepper.runToPosition();
    delay(retractDelayAfter);
    digitalWrite(ENABLE_PIN, LOW);
    isExtended = false;
    Serial.println(F("Retract motion completed."));
  }
}
