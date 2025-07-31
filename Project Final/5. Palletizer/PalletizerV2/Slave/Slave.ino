#define ENABLE_MODULE_SERIAL_ENHANCED

#include "StepperSlave.h"

#define CLK_PIN 10
#define CW_PIN 11
#define EN_PIN 12
#define RX_PIN 8
#define TX_PIN 9
#define SENSOR_PIN 6

StepperSlave slave('x', RX_PIN, TX_PIN, CLK_PIN, CW_PIN, EN_PIN, SENSOR_PIN);  // x;1;5000;d5000;2000;7000;0
void setup() {
  slave.begin();
}

void loop() {
  slave.update();
}
