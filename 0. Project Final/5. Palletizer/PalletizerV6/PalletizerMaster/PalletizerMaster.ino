#define ENABLE_MODULE_SERIAL_ENHANCED
#define ENABLE_MODULE_DIGITAL_OUTPUT

#include "PalletizerMaster.h"

#if defined(ESP32)
#define RX_PIN 16
#define TX_PIN 17
#define INDICATOR_PIN 26
#else
#define RX_PIN 8
#define TX_PIN 9
#define INDICATOR_PIN 2
#endif

// RST, 18, 19, 23, 5(Tidak Terpakai)

PalletizerMaster master(RX_PIN, TX_PIN, INDICATOR_PIN);

void setup() {
  master.begin();
}

void loop() {
  master.update();
}