#define ENABLE_MODULE_SERIAL_ENHANCED
#define ENABLE_MODULE_DIGITAL_OUTPUT

#include "PalletizerMaster.h"
#include "PalletizerServer.h"
#include "LittleFS.h"

// Pin definitions
#define RX_PIN 16
#define TX_PIN 17
#define INDICATOR_PIN 26

// WiFi configuration
#define WIFI_MODE PalletizerServer::MODE_STA  // Change to MODE_STA to connect to existing WiFi
#define WIFI_SSID "silenceAndSleep"           // AP mode: access point name, STA mode: WiFi to connect to
#define WIFI_PASSWORD "11111111"              // AP mode: access point password, STA mode: WiFi password

PalletizerMaster master(RX_PIN, TX_PIN, INDICATOR_PIN);
PalletizerServer server(&master, WIFI_MODE, WIFI_SSID, WIFI_PASSWORD);

void onSlaveData(const String& data) {
  // Forward slave data to any external system if needed
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

  // Set callback for slave data
  master.setSlaveDataCallback(onSlaveData);

  master.begin();
  Serial.println("Palletizer master initialized");

  server.begin();
  Serial.println("Web server initialized");

  Serial.println("System ready");
}

void loop() {
  master.update();
  server.update();
}