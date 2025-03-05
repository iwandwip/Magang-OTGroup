#define ENABLE_MODULE_SERIAL_ENHANCED

#include "StepperSlave.h"

#define SLAVE_ADDR 'x'
#define CLK_PIN 10
#define CW_PIN 11
#define EN_PIN 12
#define RX_PIN 8
#define TX_PIN 9
#define SENSOR_PIN 6

#if SLAVE_ADDR == 't'
#define BRAKE_PIN -1  // 12
#else
#define BRAKE_PIN -1
#endif

#define INVERT_BRAKE false
#define INDICATOR_PIN 13

StepperSlave slave('t', RX_PIN, TX_PIN, CLK_PIN, CW_PIN, EN_PIN, SENSOR_PIN, BRAKE_PIN, INVERT_BRAKE, INDICATOR_PIN);

// t;1;0
// t;1;2000;d2000;0
// t;1;2000;0;3000;0;4000

// CMD_NONE     = 0
// CMD_STAR     = 1 // t;1;200
// CMD_ZERO     = 2 // t;2
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