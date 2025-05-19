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

      dnsServer.start(53, "*", apIP);
      dnsRunning = true;
      Serial.println("DNS Server started");
    }
  }

  setupRoutes();
  setupCaptivePortal();
  server.begin();
  Serial.println("HTTP server started");
}

void PalletizerServer::update() {
  if (dnsRunning) {
    dnsServer.processNextRequest();
  }

  // #if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2)
  //   MDNS.update();
  // #endif

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

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });
}

void PalletizerServer::setupCaptivePortal() {
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "success");
  });
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

    int startPos = 0;
    int endPos;

    while ((endPos = tempBuffer.indexOf('\n', startPos)) != -1) {
      String command = tempBuffer.substring(startPos, endPos);
      command.trim();
      if (command.length() > 0) {
        file.println(command);
      }
      startPos = endPos + 1;
    }

    if (startPos < tempBuffer.length()) {
      String command = tempBuffer.substring(startPos);
      command.trim();
      if (command.length() > 0) {
        file.println(command);
      }
    }

    file.close();

    File readFile = LittleFS.open("/queue.txt", "r");
    if (readFile) {
      String commands = "";
      while (readFile.available()) {
        String line = readFile.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
          commands += line + "\n";
        }
      }
      readFile.close();

      palletizerMaster->processCommand("IDLE");

      startPos = 0;
      while ((endPos = commands.indexOf('\n', startPos)) != -1) {
        String command = commands.substring(startPos, endPos);
        command.trim();
        if (command.length() > 0) {
          palletizerMaster->processCommand(command);
        }
        startPos = endPos + 1;
      }

      palletizerMaster->processCommand("END_QUEUE");
    }

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

    int startPos = 0;
    int endPos;

    while ((endPos = commands.indexOf('\n', startPos)) != -1) {
      String cmd = commands.substring(startPos, endPos);
      cmd.trim();
      if (cmd.length() > 0) {
        file.println(cmd);
      }
      startPos = endPos + 1;
    }

    if (startPos < commands.length()) {
      String cmd = commands.substring(startPos);
      cmd.trim();
      if (cmd.length() > 0) {
        file.println(cmd);
      }
    }

    file.close();

    palletizerMaster->processCommand("IDLE");

    File readFile = LittleFS.open("/queue.txt", "r");
    if (readFile) {
      String fileContents = "";
      while (readFile.available()) {
        String line = readFile.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
          fileContents += line + "\n";
        }
      }
      readFile.close();

      startPos = 0;
      while ((endPos = fileContents.indexOf('\n', startPos)) != -1) {
        String cmd = fileContents.substring(startPos, endPos);
        cmd.trim();
        if (cmd.length() > 0) {
          palletizerMaster->processCommand(cmd);
        }
        startPos = endPos + 1;
      }
    }

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