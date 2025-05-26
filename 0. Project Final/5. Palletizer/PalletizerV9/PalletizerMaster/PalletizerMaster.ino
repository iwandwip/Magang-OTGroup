#define ENABLE_MODULE_SERIAL_ENHANCED
#define ENABLE_MODULE_DIGITAL_OUTPUT

#include "PalletizerMaster.h"
#include "PalletizerServer.h"
#include "LittleFS.h"
#include "ESPmDNS.h"

#define RX_PIN 16
#define TX_PIN 17
#define INDICATOR_PIN 26

#define WIFI_MODE PalletizerServer::MODE_AP
#define WIFI_SSID "PalletizerAP"
#define WIFI_PASSWORD ""

PalletizerMaster master(RX_PIN, TX_PIN, INDICATOR_PIN);
PalletizerServer server(&master, WIFI_MODE, WIFI_SSID, WIFI_PASSWORD);

void onSlaveData(const String& data) {
  Serial.println("SLAVE DATA: " + data);
}

void setup() {
  Serial.begin(9600);
  Serial.println("\nPalletizer System Starting...");

  if (!LittleFS.begin(true)) {
    Serial.println("Error mounting LittleFS! System will continue without file storage.");
  } else {
    Serial.println("LittleFS mounted successfully");
  }

  master.setSlaveDataCallback(onSlaveData);

  master.begin();
  Serial.println("Palletizer master initialized");

  server.begin();
  Serial.println("Web server initialized");

  Serial.println("System ready");
  Serial.println("Access at: http://palletizer.local");
}

void loop() {
  master.update();
  server.update();
}