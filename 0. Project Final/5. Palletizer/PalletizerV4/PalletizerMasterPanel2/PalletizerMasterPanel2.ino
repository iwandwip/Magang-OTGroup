#define ENABLE_MODULE_SERIAL_ENHANCED
#define ENABLE_MODULE_DIGITAL_OUTPUT

#include "PalletizerMaster.h"

#define RX_PIN 8
#define TX_PIN 9
#define INDICATOR_PIN 2

PalletizerMaster master(RX_PIN, TX_PIN, INDICATOR_PIN);

void setup() {
  master.begin();
}

void loop() {
  master.update();
}