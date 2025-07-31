#define ENABLE_MODULE_SERIAL_ENHANCED

#include "PalletizerMaster.h"

#define RX_PIN 8
#define TX_PIN 9

PalletizerMaster master(RX_PIN, TX_PIN);

void setup() {
  master.begin();
}

void loop() {
  master.update();
}