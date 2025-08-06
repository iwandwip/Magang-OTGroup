#include <AccelStepper.h>
#include <EEPROM.h>

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

// EEPROM Memory Mapping
const int EEPROM_SIGNATURE_ADDR = 0;    // 4 bytes - Signature to check if EEPROM is initialized
const int EXTEND_SPEED_ADDR = 4;        // 4 bytes - float extendMaxSpeed
const int EXTEND_ACCEL_ADDR = 8;        // 4 bytes - float extendAcceleration  
const int EXTEND_DELAY_ADDR = 12;       // 4 bytes - int extendDelayBefore
const int RETRACT_SPEED_ADDR = 16;      // 4 bytes - float retractMaxSpeed
const int RETRACT_ACCEL_ADDR = 20;      // 4 bytes - float retractAcceleration
const int RETRACT_DELAY_BEFORE_ADDR = 24; // 4 bytes - int retractDelayBefore
const int RETRACT_DELAY_AFTER_ADDR = 28;  // 4 bytes - int retractDelayAfter
const int RETRACT_ADJUSTMENT_ADDR = 32;   // 4 bytes - int retractStepAdjustment
// Total EEPROM usage: 36 bytes

const uint32_t EEPROM_SIGNATURE = 0x4F545033; // "OTP3" in hex

// Configurable Motion Parameters (Default Values)
float extendMaxSpeed = 1200.0;        // Base speed for extend motion (steps/sec)
float extendAcceleration = 600.0;     // Base acceleration for extend motion (steps/sec²)
int extendDelayBefore = 150;          // Delay before extend motion (ms)

float retractMaxSpeed = 3000.0;       // Base speed for retract motion (steps/sec)
float retractAcceleration = 1900.0;   // Base acceleration for retract motion (steps/sec²)
int retractDelayBefore = 250;         // Delay before retract motion (ms)
int retractDelayAfter = 100;          // Delay after retract motion (ms)
int retractStepAdjustment = 2;        // Step adjustment for retract motion

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
    saveToEEPROM(); // Save defaults to EEPROM
  }
}

void resetEEPROM() {
  // Reset to default values
  extendMaxSpeed = 1200.0;
  extendAcceleration = 600.0;
  extendDelayBefore = 150;
  retractMaxSpeed = 3000.0;
  retractAcceleration = 1900.0;
  retractDelayBefore = 250;
  retractDelayAfter = 100;
  retractStepAdjustment = 2;
  
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
        }
        else if (paramName == "EXTEND_ACCEL") {
          extendAcceleration = value;
          Serial.print(F("Extend acceleration set to: "));
          Serial.println(extendAcceleration);
          saveToEEPROM();
        }
        else if (paramName == "EXTEND_DELAY") {
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
        }
        else if (paramName == "RETRACT_ACCEL") {
          retractAcceleration = value;
          Serial.print(F("Retract acceleration set to: "));
          Serial.println(retractAcceleration);
          saveToEEPROM();
        }
        else if (paramName == "RETRACT_DELAY_BEFORE") {
          retractDelayBefore = (int)value;
          Serial.print(F("Retract delay before set to: "));
          Serial.print(retractDelayBefore);
          Serial.println(F("ms"));
          saveToEEPROM();
        }
        else if (paramName == "RETRACT_DELAY_AFTER") {
          retractDelayAfter = (int)value;
          Serial.print(F("Retract delay after set to: "));
          Serial.print(retractDelayAfter);
          Serial.println(F("ms"));
          saveToEEPROM();
        }
        else if (paramName == "RETRACT_ADJUSTMENT") {
          retractStepAdjustment = (int)value;
          Serial.print(F("Retract step adjustment set to: "));
          Serial.println(retractStepAdjustment);
          saveToEEPROM();
        }
        else {
          Serial.print(F("Unknown parameter: "));
          Serial.println(paramName);
        }
      } else {
        Serial.println(F("Invalid format. Use: SET PARAMETER=VALUE"));
      }
    }
    else if (command == "SHOW") {
      Serial.println(F("=== Current Configuration ==="));
      Serial.print(F("EXTEND_SPEED=")); Serial.println(extendMaxSpeed);
      Serial.print(F("EXTEND_ACCEL=")); Serial.println(extendAcceleration);
      Serial.print(F("EXTEND_DELAY=")); Serial.println(extendDelayBefore);
      Serial.print(F("RETRACT_SPEED=")); Serial.println(retractMaxSpeed);
      Serial.print(F("RETRACT_ACCEL=")); Serial.println(retractAcceleration);
      Serial.print(F("RETRACT_DELAY_BEFORE=")); Serial.println(retractDelayBefore);
      Serial.print(F("RETRACT_DELAY_AFTER=")); Serial.println(retractDelayAfter);
      Serial.print(F("RETRACT_ADJUSTMENT=")); Serial.println(retractStepAdjustment);
      Serial.println(F("============================="));
    }
    else if (command == "SAVE") {
      saveToEEPROM();
    }
    else if (command == "LOAD") {
      loadFromEEPROM();
    }
    else if (command == "RESET") {
      resetEEPROM();
    }
    else if (command == "HELP") {
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
      Serial.println(F("HELP                          - Show this help"));
      Serial.println(F("============================"));
    }
    else {
      Serial.println(F("Unknown command. Type HELP for available commands."));
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  
  Serial.println(F("OT Pack V3 - Serial Commander Ready"));
  Serial.println(F("Loading configuration from EEPROM..."));
  loadFromEEPROM();
  Serial.println(F("Type HELP for available commands"));
}

void loop() {
  // Check for serial commands
  serialCommander();
  
  // Display sensor status (reduced frequency for readability)
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 500) {  // Update every 500ms
    Serial.print(F("| SENSOR_PIN: "));
    Serial.print(digitalRead(SENSOR_PIN));
    Serial.print(F(" | State: "));
    Serial.println(isExtended ? F("Extended") : F("Retracted"));
    lastStatusTime = millis();
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
