#ifndef PALLETIZER_SERVER_H
#define PALLETIZER_SERVER_H

#include "PalletizerMaster.h"
#include "PalletizerServerIndex.h"
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

class PalletizerServer {
public:
  PalletizerServer(PalletizerMaster* master, const char* ssid = "ESP32_Palletizer_AP", const char* password = "");
  void begin();
  void update();

private:
  PalletizerMaster* palletizerMaster;
  AsyncWebServer server;
  const char* ssid;
  const char* password;

  void setupRoutes();
  void handleUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
  void handleCommand(AsyncWebServerRequest* request);
  void handleWriteCommand(AsyncWebServerRequest* request);
};

#endif