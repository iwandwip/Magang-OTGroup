#ifndef PALLETIZER_SERVER_H
#define PALLETIZER_SERVER_H

#include "PalletizerMaster.h"
#include "WiFi.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "LittleFS.h"
#include "ESPmDNS.h"
#include "DNSServer.h"

class PalletizerServer {
public:
  enum WiFiMode {
    MODE_AP,
    MODE_STA
  };

  PalletizerServer(PalletizerMaster* master, WiFiMode mode = MODE_AP, const char* ssid = "PalletizerAP", const char* password = "");
  void begin();
  void update();

private:
  PalletizerMaster* palletizerMaster;
  AsyncWebServer server;
  AsyncEventSource events;
  DNSServer dnsServer;
  bool dnsRunning;

  WiFiMode wifiMode;
  const char* ssid;
  const char* password;

  void setupRoutes();
  void setupCaptivePortal();
  void loadSavedCommands();
  void handleUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
  void handleCommand(AsyncWebServerRequest* request);
  void handleWriteCommand(AsyncWebServerRequest* request);
  void handleGetStatus(AsyncWebServerRequest* request);
  void handleGetCommands(AsyncWebServerRequest* request);
  void handleDownloadCommands(AsyncWebServerRequest* request);
  void sendStatusEvent(const String& status);
};

#endif