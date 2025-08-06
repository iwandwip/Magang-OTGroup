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

// LED Timing Parameters EEPROM Addresses
const int LED_IDLE_PERIOD_ADDR = 36;     // 4 bytes - unsigned long ledIdlePeriod
const int LED_EXTEND_PERIOD_ADDR = 40;   // 4 bytes - unsigned long ledExtendPeriod
const int LED_RETRACT_PERIOD_ADDR = 44;  // 4 bytes - unsigned long ledRetractPeriod
const int LED_ERROR_PERIOD_ADDR = 48;    // 4 bytes - unsigned long ledErrorPeriod
const int LED_DEBUG_PERIOD_ADDR = 52;    // 4 bytes - unsigned long ledDebugPeriod
// Total EEPROM usage: 56 bytes

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

// Sensor Debouncing Configuration
const unsigned long DEBOUNCE_DELAY = 50;           // Debounce delay in milliseconds
const int DEBOUNCE_SAMPLES = 5;                    // Number of consistent readings required
const unsigned long DEBOUNCE_SAMPLE_INTERVAL = 2;  // Interval between samples in milliseconds

// Motion Smoothness Configuration
const int DEFAULT_JERK_REDUCTION_STEPS = 10;  // Number of steps for S-curve ramp
const int DEFAULT_ENABLE_RAMP_DELAY = 50;     // Gradual enable ramp delay (ms)
const int DEFAULT_DISABLE_RAMP_DELAY = 30;    // Gradual disable ramp delay (ms)
const float DEFAULT_SPEED_RAMP_FACTOR = 0.1;  // Speed ramping factor (0.1 = 10% steps)
const int DEFAULT_MOTION_SETTLE_DELAY = 25;   // Settling delay after motion (ms)

// Resonance Avoidance Configuration
const float RESONANCE_AVOID_MIN_SPEED = 100.0;  // Minimum speed to avoid resonance
const float RESONANCE_AVOID_MAX_SPEED = 350.0;  // Maximum speed to avoid resonance
const float RESONANCE_SAFE_SPEED = 500.0;       // Safe speed above resonance zone

// LED Indicator Configuration (TimerOne periods in microseconds)
const unsigned long LED_IDLE_PERIOD = 1000000;    // 1000ms - System idle/ready
const unsigned long LED_EXTEND_PERIOD = 100000;   // 100ms - Extending motion
const unsigned long LED_RETRACT_PERIOD = 500000;  // 500ms - Retracting motion
const unsigned long LED_ERROR_PERIOD = 50000;     // 50ms - Error condition
const unsigned long LED_DEBUG_PERIOD = 200000;    // 200ms - Debug mode active

// LED State Enumeration (Global scope for Arduino IDE)
typedef enum {
  LED_OFF,      // LED disabled
  LED_IDLE,     // 1000ms - System idle/ready
  LED_EXTEND,   // 100ms - Extending motion
  LED_RETRACT,  // 500ms - Retracting motion
  LED_ERROR,    // 50ms - Error condition
  LED_DEBUG     // 200ms - Debug mode active
} LedState;

// External variable declarations for cross-file access
extern volatile LedState currentLedState;
extern volatile bool ledEnabled;
extern volatile unsigned long ledBlinkCount;

// External LED configuration variables
extern unsigned long ledIdlePeriod;
extern unsigned long ledExtendPeriod;
extern unsigned long ledRetractPeriod;
extern unsigned long ledErrorPeriod;
extern unsigned long ledDebugPeriod;

#endif  // CONSTANTS_H