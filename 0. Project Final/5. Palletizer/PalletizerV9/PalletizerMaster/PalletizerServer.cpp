#include "PalletizerServer.h"

PalletizerServer::PalletizerServer(PalletizerMaster *master, WiFiMode mode, const char *ap_ssid, const char *ap_password)
  : palletizerMaster(master), server(80), events("/events"), wifiMode(mode), ssid(ap_ssid), password(ap_password), dnsRunning(false) {
}

void PalletizerServer::begin() {
  if (wifiMode == MODE_AP) {
    IPAddress apIP(192, 168, 4, 1);
    IPAddress netMsk(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP Mode - IP address: ");
    Serial.println(IP);

    if (MDNS.begin("palletizer")) {
      Serial.println("mDNS responder started. Access at: http://palletizer.local");
      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println("Error setting up mDNS responder!");
    }

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", apIP);
    dnsRunning = true;
    Serial.println("DNS Server started");
  } else {
    WiFi.begin(ssid, password);
    int attempts = 0;
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("STA Mode - Connected to WiFi. IP address: ");
      Serial.println(WiFi.localIP());

      if (MDNS.begin("palletizer")) {
        Serial.println("mDNS responder started. Access at: http://palletizer.local");
        MDNS.addService("http", "tcp", 80);
      } else {
        Serial.println("Error setting up mDNS responder!");
      }
    } else {
      Serial.println();
      Serial.println("Failed to connect to WiFi. Falling back to AP mode.");
      WiFi.disconnect();

      IPAddress apIP(192, 168, 4, 1);
      IPAddress netMsk(255, 255, 255, 0);
      WiFi.softAPConfig(apIP, apIP, netMsk);
      WiFi.softAP("ESP32_Palletizer_Fallback", "palletizer123");
      IPAddress IP = WiFi.softAPIP();
      Serial.print("Fallback AP Mode - IP address: ");
      Serial.println(IP);

      if (MDNS.begin("palletizer")) {
        Serial.println("mDNS responder started. Access at: http://palletizer.local");
        MDNS.addService("http", "tcp", 80);
      } else {
        Serial.println("Error setting up mDNS responder!");
      }

      dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
      dnsServer.start(53, "*", apIP);
      dnsRunning = true;
      Serial.println("DNS Server started");
    }
  }

  setupRoutes();
  setupCaptivePortal();
  server.begin();
  Serial.println("HTTP server started");

  loadSavedCommands();
}

void PalletizerServer::update() {
  if (dnsRunning) {
    dnsServer.processNextRequest();
  }

  if (wifiMode == MODE_STA && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.begin(ssid, password);
  }

  PalletizerMaster::SystemState currentState = palletizerMaster->getSystemState();
  String statusStr;

  switch (currentState) {
    case PalletizerMaster::STATE_IDLE:
      statusStr = "IDLE";
      break;
    case PalletizerMaster::STATE_RUNNING:
      statusStr = "RUNNING";
      break;
    case PalletizerMaster::STATE_PAUSED:
      statusStr = "PAUSED";
      break;
    case PalletizerMaster::STATE_STOPPING:
      statusStr = "STOPPING";
      break;
    default:
      statusStr = "UNKNOWN";
      break;
  }

  static PalletizerMaster::SystemState lastState = PalletizerMaster::STATE_IDLE;
  if (currentState != lastState) {
    sendStatusEvent(statusStr);
    lastState = currentState;
  }
}

void PalletizerServer::setupRoutes() {
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/command", HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->handleCommand(request);
  });

  server.on("/write", HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->handleWriteCommand(request);
  });

  server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetStatus(request);
  });

  server.on("/get_commands", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetCommands(request);
  });

  server.on("/download_commands", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleDownloadCommands(request);
  });

  server.on(
    "/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "File uploaded");
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      this->handleUpload(request, filename, index, data, len, final);
    });

  events.onConnect([this](AsyncEventSourceClient *client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID: %u\n", client->lastId());
    }

    String statusStr;
    switch (palletizerMaster->getSystemState()) {
      case PalletizerMaster::STATE_IDLE:
        statusStr = "IDLE";
        break;
      case PalletizerMaster::STATE_RUNNING:
        statusStr = "RUNNING";
        break;
      case PalletizerMaster::STATE_PAUSED:
        statusStr = "PAUSED";
        break;
      case PalletizerMaster::STATE_STOPPING:
        statusStr = "STOPPING";
        break;
      default:
        statusStr = "UNKNOWN";
        break;
    }

    sendStatusEvent(statusStr);
  });

  server.addHandler(&events);

  server.on("/wifi_info", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String info = "{";
    if (wifiMode == MODE_AP) {
      info += "\"mode\":\"AP\",";
      info += "\"ssid\":\"" + String(ssid) + "\",";
      info += "\"ip\":\"" + WiFi.softAPIP().toString() + "\"";
    } else {
      info += "\"mode\":\"STA\",";
      info += "\"ssid\":\"" + String(ssid) + "\",";
      info += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
      info += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
    }
    info += "}";
    request->send(200, "application/json", info);
  });
}

void PalletizerServer::setupCaptivePortal() {
  server.on("/generate_204", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: generate_204");
    request->redirect("http://palletizer.local/");
  });

  server.on("/gen_204", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: gen_204");
    request->redirect("http://palletizer.local/");
  });

  server.on("/mobile/status.php", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: status.php");
    request->redirect("http://palletizer.local/");
  });

  server.on("/fwlink", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: fwlink");
    request->redirect("http://palletizer.local/");
  });

  server.on("/connecttest.txt", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: connecttest.txt");
    request->send(200, "text/plain", "Microsoft NCSI");
  });

  server.on("/redirect", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: redirect");
    request->redirect("http://palletizer.local/");
  });

  server.on("/hotspot-detect.html", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: hotspot-detect.html");
    request->send(200, "text/html", "<!DOCTYPE html><html><head><title>Success</title></head><body>Success</body></html>");
  });

  server.on("/library/test/success.html", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: library/test/success.html");
    request->send(200, "text/html", "<!DOCTYPE html><html><head><title>Success</title></head><body>Success</body></html>");
  });

  server.on("/success.txt", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: success.txt");
    request->send(200, "text/plain", "success\n");
  });

  server.on("/ncsi.txt", HTTP_ANY, [](AsyncWebServerRequest *request) {
    Serial.println("Captive portal: ncsi.txt");
    request->send(200, "text/plain", "Microsoft NCSI");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("Captive portal - Unknown: ");
    Serial.print(request->host());
    Serial.print(" ");
    Serial.println(request->url());

    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      if (request->host() != "palletizer.local" && request->host() != "192.168.4.1") {
        request->redirect("http://palletizer.local/");
      } else {
        request->send(LittleFS, "/index.html", "text/html");
      }
    }
  });
}

void PalletizerServer::loadSavedCommands() {
  if (!LittleFS.exists("/queue.txt")) {
    Serial.println("No saved commands found");
    return;
  }

  File file = LittleFS.open("/queue.txt", "r");
  if (!file) {
    Serial.println("Failed to open queue.txt");
    return;
  }

  Serial.println("Loading saved commands from queue.txt");

  String commands = "";
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      commands += line + "\n";
    }
  }
  file.close();

  if (commands.length() > 0) {
    palletizerMaster->processCommand("IDLE");
    palletizerMaster->processCommand(commands);
    palletizerMaster->processCommand("END_QUEUE");
    Serial.println("All saved commands loaded successfully");
  } else {
    Serial.println("No valid commands found in queue.txt");
  }
}

void PalletizerServer::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static String tempBuffer = "";

  if (!index) {
    tempBuffer = "";
    if (LittleFS.exists("/queue.txt")) {
      LittleFS.remove("/queue.txt");
    }
    File file = LittleFS.open("/queue.txt", "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }
    file.close();
  }

  String dataStr = String((char *)data, len);
  tempBuffer += dataStr;

  if (final) {
    File file = LittleFS.open("/queue.txt", "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }

    file.print(tempBuffer);
    file.close();

    palletizerMaster->processCommand("IDLE");
    palletizerMaster->processCommand(tempBuffer);
    palletizerMaster->processCommand("END_QUEUE");

    tempBuffer = "";
  }
}

void PalletizerServer::handleCommand(AsyncWebServerRequest *request) {
  String command = "";

  if (request->hasParam("cmd", true)) {
    command = request->getParam("cmd", true)->value();
  }

  if (command.length() > 0) {
    palletizerMaster->processCommand(command);
    request->send(200, "text/plain", "Command sent: " + command);
  } else {
    request->send(400, "text/plain", "No command provided");
  }
}

void PalletizerServer::handleWriteCommand(AsyncWebServerRequest *request) {
  String commands = "";

  if (request->hasParam("text", true)) {
    commands = request->getParam("text", true)->value();
  }

  if (commands.length() > 0) {
    if (LittleFS.exists("/queue.txt")) {
      LittleFS.remove("/queue.txt");
    }

    File file = LittleFS.open("/queue.txt", "w");
    if (!file) {
      request->send(500, "text/plain", "Failed to open file for writing");
      return;
    }

    file.print(commands);
    file.close();

    palletizerMaster->processCommand("IDLE");
    palletizerMaster->processCommand(commands);
    palletizerMaster->processCommand("END_QUEUE");
    request->send(200, "text/plain", "Commands saved and loaded");
  } else {
    request->send(400, "text/plain", "No commands provided");
  }
}

void PalletizerServer::handleGetStatus(AsyncWebServerRequest *request) {
  String statusStr;

  switch (palletizerMaster->getSystemState()) {
    case PalletizerMaster::STATE_IDLE:
      statusStr = "IDLE";
      break;
    case PalletizerMaster::STATE_RUNNING:
      statusStr = "RUNNING";
      break;
    case PalletizerMaster::STATE_PAUSED:
      statusStr = "PAUSED";
      break;
    case PalletizerMaster::STATE_STOPPING:
      statusStr = "STOPPING";
      break;
    default:
      statusStr = "UNKNOWN";
      break;
  }

  String response = "{\"status\":\"" + statusStr + "\"}";
  request->send(200, "application/json", response);
}

void PalletizerServer::handleGetCommands(AsyncWebServerRequest *request) {
  File file = LittleFS.open("/queue.txt", "r");
  if (!file) {
    request->send(404, "text/plain", "File not found");
    return;
  }

  String content = "";
  while (file.available()) {
    content += file.readStringUntil('\n');
    if (file.available()) {
      content += "\n";
    }
  }
  file.close();

  request->send(200, "text/plain", content);
}

void PalletizerServer::handleDownloadCommands(AsyncWebServerRequest *request) {
  File file = LittleFS.open("/queue.txt", "r");
  if (!file) {
    request->send(404, "text/plain", "File not found");
    return;
  }

  String content = "";
  while (file.available()) {
    content += file.readStringUntil('\n');
    if (file.available()) {
      content += "\n";
    }
  }
  file.close();

  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", content);
  response->addHeader("Content-Disposition", "attachment; filename=palletizer_commands.txt");
  request->send(response);
}

void PalletizerServer::sendStatusEvent(const String &status) {
  String eventData = "{\"type\":\"status\",\"value\":\"" + status + "\"}";
  events.send(eventData.c_str(), "message", millis());
}