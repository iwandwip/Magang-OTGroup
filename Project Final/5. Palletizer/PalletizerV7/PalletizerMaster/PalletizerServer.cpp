#include "PalletizerServer.h"

PalletizerServer::PalletizerServer(PalletizerMaster *master, const char *ap_ssid, const char *ap_password)
  : palletizerMaster(master), server(80), events("/events"), ssid(ap_ssid), password(ap_password) {
}

void PalletizerServer::begin() {
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  setupRoutes();

  server.begin();
  Serial.println("HTTP server started");
}

void PalletizerServer::update() {
  // Any periodic tasks needed
}

void PalletizerServer::setupRoutes() {
  // Set up file server for LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Command routes
  server.on("/command", HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->handleCommand(request);
  });

  // Write command route
  server.on("/write", HTTP_POST, [this](AsyncWebServerRequest *request) {
    this->handleWriteCommand(request);
  });

  // Status route
  server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetStatus(request);
  });

  // Upload route
  server.on(
    "/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "File uploaded");
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      this->handleUpload(request, filename, index, data, len, final);
    });

  // Set up event source for server-sent events
  events.onConnect([this](AsyncEventSourceClient *client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID: %u\n", client->lastId());
    }

    // Send current status when client connects
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

  // Handle 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
}

void PalletizerServer::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    // Open the file for writing on first call
    File file = LittleFS.open("/queue.txt", "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }
    file.close();
  }

  File file = LittleFS.open("/queue.txt", "a");
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  // Convert data to string and process
  String dataStr = String((char *)data);

  // Split by lines and add each command
  int startPos = 0;
  int endPos;

  while ((endPos = dataStr.indexOf('\n', startPos)) != -1) {
    String command = dataStr.substring(startPos, endPos);
    command.trim();
    if (command.length() > 0) {
      file.println(command);
    }
    startPos = endPos + 1;
  }

  // Handle last line if there is no final newline
  if (startPos < dataStr.length()) {
    String command = dataStr.substring(startPos);
    command.trim();
    if (command.length() > 0) {
      file.println(command);
    }
  }

  file.close();

  if (final) {
    // After file is uploaded, load the commands to the palletizer
    File readFile = LittleFS.open("/queue.txt", "r");
    if (readFile) {
      // Clear current queue in the palletizer
      // Send "IDLE" to clear queue and then add each command
      String command = "IDLE";
      PalletizerMaster::onBluetoothDataWrapper(command);

      while (readFile.available()) {
        String line = readFile.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
          // Simulate sending over Bluetooth
          PalletizerMaster::onBluetoothDataWrapper(line);
        }
      }

      // End queue
      PalletizerMaster::onBluetoothDataWrapper("END_QUEUE");

      readFile.close();
    }
  }
}

void PalletizerServer::handleCommand(AsyncWebServerRequest *request) {
  String command = "";

  // Get command from parameters
  if (request->hasParam("cmd", true)) {
    command = request->getParam("cmd", true)->value();
  }

  if (command.length() > 0) {
    // Simulate sending over Bluetooth
    PalletizerMaster::onBluetoothDataWrapper(command);
    request->send(200, "text/plain", "Command sent: " + command);
  } else {
    request->send(400, "text/plain", "No command provided");
  }
}

void PalletizerServer::handleWriteCommand(AsyncWebServerRequest *request) {
  String commands = "";

  // Get commands from parameters
  if (request->hasParam("text", true)) {
    commands = request->getParam("text", true)->value();
  }

  if (commands.length() > 0) {
    // Open file for writing
    File file = LittleFS.open("/queue.txt", "w");
    if (!file) {
      request->send(500, "text/plain", "Failed to open file for writing");
      return;
    }

    // Write commands to file
    file.print(commands);
    file.close();

    // Clear current queue in the palletizer
    String command = "IDLE";
    PalletizerMaster::onBluetoothDataWrapper(command);

    // Split commands by lines and add each one
    int startPos = 0;
    int endPos;

    while ((endPos = commands.indexOf('\n', startPos)) != -1) {
      String cmd = commands.substring(startPos, endPos);
      cmd.trim();
      if (cmd.length() > 0) {
        // Simulate sending over Bluetooth
        PalletizerMaster::onBluetoothDataWrapper(cmd);
      }
      startPos = endPos + 1;
    }

    // Handle last line if there is no final newline
    if (startPos < commands.length()) {
      String cmd = commands.substring(startPos);
      cmd.trim();
      if (cmd.length() > 0) {
        PalletizerMaster::onBluetoothDataWrapper(cmd);
      }
    }

    // End queue
    PalletizerMaster::onBluetoothDataWrapper("END_QUEUE");

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

void PalletizerServer::sendStatusEvent(const String &status) {
  String eventData = "{\"type\":\"status\",\"value\":\"" + status + "\"}";
  events.send(eventData.c_str(), "message", millis());
}