#define ENABLE_MODULE_SERIAL_ENHANCED

#include "StepperSlave.h"

#define SLAVE_ADDR Z_AXIS

#define CLK_PIN 10  // 10
#define CW_PIN 11   // 11
#define RX_PIN 8
#define TX_PIN 9
#define SENSOR_PIN 6
#define INDICATOR_PIN 13

#if (SLAVE_ADDR == X_AXIS) || (SLAVE_ADDR == Y_AXIS) || (SLAVE_ADDR == G_AXIS)
#define EN_PIN 12
#define BRAKE_PIN NOT_CONNECTED
#define INVERT_BRAKE HIGH_LOGIC_BRAKE
#define INVERT_ENABLE HIGH_LOGIC_BRAKE  // FIX: LOW_LOGIC_ENABLE
#define BRAKE_ENGAGE_DELAY NO_DELAY
#define BRAKE_RELEASE_DELAY NO_DELAY
#elif (SLAVE_ADDR == T_AXIS)
#define EN_PIN 12
#define BRAKE_PIN 7
#define INVERT_BRAKE HIGH_LOGIC_BRAKE
#define INVERT_ENABLE LOW_LOGIC_ENABLE
#define BRAKE_ENGAGE_DELAY 1500
#define BRAKE_RELEASE_DELAY 250
#elif (SLAVE_ADDR == Z_AXIS)
#define EN_PIN NOT_CONNECTED
#define BRAKE_PIN NOT_CONNECTED
#define INVERT_BRAKE HIGH_LOGIC_BRAKE
#define INVERT_ENABLE LOW_LOGIC_ENABLE
#define BRAKE_ENGAGE_DELAY NO_DELAY
#define BRAKE_RELEASE_DELAY NO_DELAY
#endif

// Command reference: z;1;100;2000;d2000;500;1000
// CMD_RUN      = 1 // z;1;200    - Start movement to position 200
// CMD_ZERO     = 2 // z;2        - Home the axis
// CMD_SETSPEED = 6 // z;6;2000   - Set speed to 2000

StepperSlave slave(
  SLAVE_ADDR,
  RX_PIN, TX_PIN,
  CLK_PIN, CW_PIN,
  EN_PIN,
  SENSOR_PIN,
  BRAKE_PIN,
  INVERT_BRAKE,
  INDICATOR_PIN,
  INVERT_ENABLE,
  BRAKE_RELEASE_DELAY,
  BRAKE_ENGAGE_DELAY);

void setup() {
  slave.begin();
}

void loop() {
  slave.update();
}