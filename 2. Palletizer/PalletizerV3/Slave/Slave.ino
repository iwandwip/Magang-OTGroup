#define ENABLE_MODULE_SERIAL_ENHANCED

#include "StepperSlave.h"

#define CLK_PIN 10
#define CW_PIN 11
#define EN_PIN 12
#define RX_PIN 8
#define TX_PIN 9
#define SENSOR_PIN 6
#define BRAKE_PIN -1
#define INVERT_BRAKE false

StepperSlave slave('g', RX_PIN, TX_PIN, CLK_PIN, CW_PIN, EN_PIN, SENSOR_PIN, BRAKE_PIN, INVERT_BRAKE);

// Example command format:
// x;1;5000;d5000;2000;7000;0
// x;1;200

// g;1;0
// g;1;2000;d2000;0
// g;1;2000;0;3000;0;4000

void setup() {
  slave.begin();
}

void loop() {
  slave.update();
}
