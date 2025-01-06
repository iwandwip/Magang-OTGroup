#define ENABLE_MODULE_TIMER_DURATION
#define ENABLE_MODULE_TIMER_TASK
#define ENABLE_MODULE_DIGITAL_INPUT
#define ENABLE_MODULE_DIGITAL_OUTPUT
#define ENABLE_MODULE_SERIAL_SOFT
#define ENABLE_MODULE_SERIAL_HARD
#define ENABLE_MODULE_EEPROM_LIB

#include "Kinematrix.h"

#define SOLENOID_DELAY_ADDRESS 0
#define SOLENOID_PULSE_ADDRESS 2
#define SOLENOID_WAIT_ADDRESS 4

#define PROXIMITY_SENSOR_PIN 3
#define SOLENOID_OUTPUT_PIN 4

DigitalIn proximity(PROXIMITY_SENSOR_PIN, INPUT_PULLUP);
DigitalOut solenoid(SOLENOID_OUTPUT_PIN, true);
DigitalOut ledBuiltIn(LED_BUILTIN);

TimerDuration solenoidTimer;
HardSerial usbSerial;
EEPROMLib eeprom;

void (*arduinoReset)(void) = 0;

enum SystemState {
  SYSTEM_IDLE,
  SYSTEM_RUN,
  SYSTEM_WAIT,
};

int systemState = SYSTEM_IDLE;
int proximityState = 1;

int solenoidDelayMs;  // DELAY#700
int solenoidPulseMs;  // PULSE#800
int solenoidWaitMs;   // WAIT#1000