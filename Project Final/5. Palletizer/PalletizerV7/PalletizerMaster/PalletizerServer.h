#ifndef PALLETIZER_SERVER_H
#define PALLETIZER_SERVER_H

#include "PalletizerMaster.h"
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h"

class PalletizerServer {
public:
  PalletizerServer(PalletizerMaster* master, const char* ssid = "ESP32_Palletizer_AP", const char* password = "palletizer123");
  void begin();
  void update();

private:
  PalletizerMaster* palletizerMaster;
  AsyncWebServer server;
  AsyncEventSource events;
  const char* ssid;
  const char* password;

  void setupRoutes();
  void handleUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
  void handleCommand(AsyncWebServerRequest* request);
  void handleWriteCommand(AsyncWebServerRequest* request);
  void handleGetStatus(AsyncWebServerRequest* request);
  void sendStatusEvent(const String& status);
};

#endif