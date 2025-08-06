#ifndef CONSTANTS_H
#define CONSTANTS_H

// ========================================
// OT Pack V3 - Constants Configuration
// ========================================

// Pin Definitions
const byte SENSOR_PIN = 3;     // Sensor input (pullup enabled)
const byte STEP_PIN = 10;      // Step pin to motor driver
const byte ENABLE_PIN = 9;     // Motor enable pin
const byte DIRECTION_PIN = 8;  // Direction pin to motor driver

// Motor Configuration Constants
const int MICROSTEPPING_RESOLUTION = 4;  // Microstepping multiplier
const int BASE_STEPS_PER_REV = 58;       // Base motor steps per revolution

// EEPROM Memory Mapping
const int EEPROM_SIGNATURE_ADDR = 0;       // 4 bytes - Signature to check if EEPROM is initialized
const int EXTEND_SPEED_ADDR = 4;           // 4 bytes - float extendMaxSpeed
const int EXTEND_ACCEL_ADDR = 8;           // 4 bytes - float extendAcceleration
const int EXTEND_DELAY_ADDR = 12;          // 4 bytes - int extendDelayBefore
const int RETRACT_SPEED_ADDR = 16;         // 4 bytes - float retractMaxSpeed
const int RETRACT_ACCEL_ADDR = 20;         // 4 bytes - float retractAcceleration
const int RETRACT_DELAY_BEFORE_ADDR = 24;  // 4 bytes - int retractDelayBefore
const int RETRACT_DELAY_AFTER_ADDR = 28;   // 4 bytes - int retractDelayAfter
const int RETRACT_ADJUSTMENT_ADDR = 32;    // 4 bytes - int retractStepAdjustment
// Total EEPROM usage: 36 bytes

const uint32_t EEPROM_SIGNATURE = 0x4F545033;  // "OTP3" in hex

// Default Motion Parameters
const float DEFAULT_EXTEND_MAX_SPEED = 1200.0;    // Base speed for extend motion (steps/sec)
const float DEFAULT_EXTEND_ACCELERATION = 600.0;  // Base acceleration for extend motion (steps/sec²)
const int DEFAULT_EXTEND_DELAY_BEFORE = 150;      // Delay before extend motion (ms)

const float DEFAULT_RETRACT_MAX_SPEED = 3000.0;     // Base speed for retract motion (steps/sec)
const float DEFAULT_RETRACT_ACCELERATION = 1900.0;  // Base acceleration for retract motion (steps/sec²)
const int DEFAULT_RETRACT_DELAY_BEFORE = 250;       // Delay before retract motion (ms)
const int DEFAULT_RETRACT_DELAY_AFTER = 100;        // Delay after retract motion (ms)
const int DEFAULT_RETRACT_STEP_ADJUSTMENT = 2;      // Step adjustment for retract motion

// Serial Communication Constants
const long SERIAL_BAUD_RATE = 9600;                // Serial communication baud rate
const unsigned long STATUS_UPDATE_INTERVAL = 500;  // Status update interval in milliseconds

// System Information (as string literals for F() macro compatibility)
#define SYSTEM_NAME "OT Pack V3"
#define VERSION "3.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// Operating Modes
const int MODE_NORMAL = 0;   // Normal operation mode
const int MODE_TESTING = 1;  // Testing/simulation mode

#endif  // CONSTANTS_H