#define ENABLE_MODULE_TIMER_DURATION
#define ENABLE_MODULE_TIMER_TASK
#define ENABLE_MODULE_DIGITAL_INPUT
#define ENABLE_MODULE_DIGITAL_OUTPUT

#include "Kinematrix.h"

#define PROXIMITY_SENSOR_PIN 3
#define SOLENOID_OUTPUT_PIN 4

DigitalIn proximity(PROXIMITY_SENSOR_PIN, INPUT_PULLUP);
DigitalOut solenoid(SOLENOID_OUTPUT_PIN, true);
DigitalOut ledBuiltIn(LED_BUILTIN);

TimerDuration solenoidDelayOnTimer;

enum SystemState {
  SYSTEM_IDLE,
  SYSTEM_RUN,
  SYSTEM_STOP,
};

SystemState systemState = SYSTEM_IDLE;
int proximityState = 0;

constexpr uint32_t SOLENOID_DELAY_MS = 1000;
constexpr uint32_t SOLENOID_PULSE_MS = 500;

void setup() {
  Serial.begin(9600);
}

void loop() {
  switch (systemState) {
    case SYSTEM_IDLE:
      proximityState = proximity.getStateRaw();
      if (!proximityState) {
        ledBuiltIn.off();
      } else {
        ledBuiltIn.on();
        solenoidDelayOnTimer.setDuration(SOLENOID_DELAY_MS);
        solenoidDelayOnTimer.reset();
        solenoidDelayOnTimer.start();
        systemState = SYSTEM_RUN;
      }
      solenoid.off();
      break;
    case SYSTEM_RUN:
      if (solenoidDelayOnTimer.isExpired()) {
        solenoid.on();
        delay(SOLENOID_PULSE_MS);
        solenoid.off();
        systemState = SYSTEM_IDLE;
      }
      break;
  };

  DigitalIn::updateAll(&proximity, DigitalIn::stop());
  DigitalOut::updateAll(&solenoid, &ledBuiltIn, DigitalOut::stop());
}
