#define ENABLE_MODULE_SERIAL_ENHANCED

#include "StepperSlave.h"

#define SLAVE_ADDR 'y'
#define CLK_PIN 10
#define CW_PIN 11
#define EN_PIN 12
#define RX_PIN 8
#define TX_PIN 9
#define SENSOR_PIN 6

#if SLAVE_ADDR == 'x'
#define BRAKE_PIN 12
#define INVERT_BRAKE LOW_LOGIC_BRAKE
#elif SLAVE_ADDR == 't'
#define BRAKE_PIN 7
#define INVERT_BRAKE HIGH_LOGIC_BRAKE
#else
#define BRAKE_PIN NOT_CONNECTED
#define INVERT_BRAKE HIGH_LOGIC_BRAKE
#endif

#define INDICATOR_PIN 13

StepperSlave slave(SLAVE_ADDR, RX_PIN, TX_PIN, CLK_PIN, CW_PIN, EN_PIN, SENSOR_PIN, BRAKE_PIN, INVERT_BRAKE, INDICATOR_PIN);

// CMD_NONE     = 0
// CMD_STAR     = 1 // x;1;200
// CMD_ZERO     = 2 // x;2
// CMD_PAUSE    = 3
// CMD_RESUME   = 4
// CMD_RESET    = 5
// CMD_SETSPEED = 6

void setup() {
  slave.begin();
}

void loop() {
  slave.update();
}