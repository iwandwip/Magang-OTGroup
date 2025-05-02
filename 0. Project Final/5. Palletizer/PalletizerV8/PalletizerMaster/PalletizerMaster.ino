#define ENABLE_MODULE_SERIAL_ENHANCED
#define ENABLE_MODULE_DIGITAL_OUTPUT

#include "PalletizerMaster.h"
#include "PalletizerServer.h"
#include "LittleFS.h"

#define RX_PIN 16
#define TX_PIN 17
#define INDICATOR_PIN 26

PalletizerMaster master(RX_PIN, TX_PIN, INDICATOR_PIN);
PalletizerServer server(&master);

void setup() {
  Serial.begin(9600);

  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  master.begin();
  server.begin();
}

void loop() {
  master.update();
  server.update();
}