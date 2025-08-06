#include <AccelStepper.h>

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

// Configurable Motion Parameters
float extendMaxSpeed = 1200.0;        // Base speed for extend motion (steps/sec)
float extendAcceleration = 600.0;     // Base acceleration for extend motion (steps/sec²)
int extendDelayBefore = 150;          // Delay before extend motion (ms)

float retractMaxSpeed = 3000.0;       // Base speed for retract motion (steps/sec)
float retractAcceleration = 1900.0;   // Base acceleration for retract motion (steps/sec²)
int retractDelayBefore = 250;         // Delay before retract motion (ms)
int retractDelayAfter = 100;          // Delay after retract motion (ms)
int retractStepAdjustment = 2;        // Step adjustment for retract motion

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
          Serial.println("Extend speed set to: " + String(extendMaxSpeed));
        }
        else if (paramName == "EXTEND_ACCEL") {
          extendAcceleration = value;
          Serial.println("Extend acceleration set to: " + String(extendAcceleration));
        }
        else if (paramName == "EXTEND_DELAY") {
          extendDelayBefore = (int)value;
          Serial.println("Extend delay set to: " + String(extendDelayBefore) + "ms");
        }
        // Retract Motion Parameters
        else if (paramName == "RETRACT_SPEED") {
          retractMaxSpeed = value;
          Serial.println("Retract speed set to: " + String(retractMaxSpeed));
        }
        else if (paramName == "RETRACT_ACCEL") {
          retractAcceleration = value;
          Serial.println("Retract acceleration set to: " + String(retractAcceleration));
        }
        else if (paramName == "RETRACT_DELAY_BEFORE") {
          retractDelayBefore = (int)value;
          Serial.println("Retract delay before set to: " + String(retractDelayBefore) + "ms");
        }
        else if (paramName == "RETRACT_DELAY_AFTER") {
          retractDelayAfter = (int)value;
          Serial.println("Retract delay after set to: " + String(retractDelayAfter) + "ms");
        }
        else if (paramName == "RETRACT_ADJUSTMENT") {
          retractStepAdjustment = (int)value;
          Serial.println("Retract step adjustment set to: " + String(retractStepAdjustment));
        }
        else {
          Serial.println("Unknown parameter: " + paramName);
        }
      } else {
        Serial.println("Invalid format. Use: SET PARAMETER=VALUE");
      }
    }
    else if (command == "SHOW") {
      Serial.println("=== Current Configuration ===");
      Serial.println("EXTEND_SPEED=" + String(extendMaxSpeed));
      Serial.println("EXTEND_ACCEL=" + String(extendAcceleration));
      Serial.println("EXTEND_DELAY=" + String(extendDelayBefore));
      Serial.println("RETRACT_SPEED=" + String(retractMaxSpeed));
      Serial.println("RETRACT_ACCEL=" + String(retractAcceleration));
      Serial.println("RETRACT_DELAY_BEFORE=" + String(retractDelayBefore));
      Serial.println("RETRACT_DELAY_AFTER=" + String(retractDelayAfter));
      Serial.println("RETRACT_ADJUSTMENT=" + String(retractStepAdjustment));
      Serial.println("=============================");
    }
    else if (command == "HELP") {
      Serial.println("=== SerialCommander Help ===");
      Serial.println("SET EXTEND_SPEED=value        - Set extend motion speed");
      Serial.println("SET EXTEND_ACCEL=value        - Set extend acceleration");
      Serial.println("SET EXTEND_DELAY=value        - Set extend delay (ms)");
      Serial.println("SET RETRACT_SPEED=value       - Set retract motion speed");
      Serial.println("SET RETRACT_ACCEL=value       - Set retract acceleration");
      Serial.println("SET RETRACT_DELAY_BEFORE=val  - Set retract delay before (ms)");
      Serial.println("SET RETRACT_DELAY_AFTER=val   - Set retract delay after (ms)");
      Serial.println("SET RETRACT_ADJUSTMENT=val    - Set retract step adjustment");
      Serial.println("SHOW                          - Show current configuration");
      Serial.println("HELP                          - Show this help");
      Serial.println("============================");
    }
    else {
      Serial.println("Unknown command. Type HELP for available commands.");
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  
  Serial.println("OT Pack V3 - Serial Commander Ready");
  Serial.println("Type HELP for available commands");
}

void loop() {
  // Check for serial commands
  serialCommander();
  
  // Display sensor status (reduced frequency for readability)
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime > 500) {  // Update every 500ms
    Serial.print("| SENSOR_PIN: ");
    Serial.print(digitalRead(SENSOR_PIN));
    Serial.print(" | State: ");
    Serial.println(isExtended ? "Extended" : "Retracted");
    lastStatusTime = millis();
  }

  // Main motion control logic
  if (digitalRead(SENSOR_PIN) == HIGH && !isExtended) {
    Serial.println("Starting extend motion...");
    stepper.setMaxSpeed(extendMaxSpeed * MICROSTEPPING_RESOLUTION);
    stepper.setAcceleration(extendAcceleration * MICROSTEPPING_RESOLUTION);
    delay(extendDelayBefore);
    stepper.move(stepsPerRevolution);
    digitalWrite(ENABLE_PIN, HIGH);
    stepper.runToPosition();
    isExtended = true;
    Serial.println("Extend motion completed.");
  }
  
  if (digitalRead(SENSOR_PIN) == LOW && isExtended) {
    Serial.println("Starting retract motion...");
    stepper.setMaxSpeed(retractMaxSpeed * MICROSTEPPING_RESOLUTION);
    stepper.setAcceleration(retractAcceleration * MICROSTEPPING_RESOLUTION);
    delay(retractDelayBefore);
    stepper.move(-stepsPerRevolution + retractStepAdjustment);
    stepper.runToPosition();
    delay(retractDelayAfter);
    digitalWrite(ENABLE_PIN, LOW);
    isExtended = false;
    Serial.println("Retract motion completed.");
  }
}
