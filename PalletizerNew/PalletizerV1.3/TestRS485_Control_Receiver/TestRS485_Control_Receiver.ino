#include <SoftwareSerial.h>

// RS485 pins (sama seperti di Control asli)
const int RS485_RX_PIN = 10;
const int RS485_TX_PIN = 11;

// LED untuk indikasi
const int LED_PIN = 13;

// Serial communication
SoftwareSerial rs485Serial(RS485_RX_PIN, RS485_TX_PIN);

// Command buffer
char commandBuffer[64];
byte commandIndex = 0;

// Statistics
unsigned long totalReceived = 0;
unsigned long validCommands = 0;
unsigned long invalidCommands = 0;
unsigned long lastStatsTime = 0;
const unsigned long STATS_INTERVAL = 5000; // 5 detik

// Device simulation (ARM1 atau ARM2)
bool isARM2_device = false;

uint8_t calculateXORChecksum(const char* data, int length) {
  uint8_t checksum = 0;
  for (int i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

bool parseAndValidateMessage(const char* receivedMessage, char* cleanCommand) {
  const char* separator = strrchr(receivedMessage, '*');
  
  if (separator == NULL) {
    Serial.println("ERROR: No checksum separator found");
    return false;
  }
  
  int commandLength = separator - receivedMessage;
  strncpy(cleanCommand, receivedMessage, commandLength);
  cleanCommand[commandLength] = '\0';
  
  const char* receivedCRCStr = separator + 1;
  uint8_t receivedCRC = (uint8_t)strtol(receivedCRCStr, NULL, 16);
  uint8_t calculatedCRC = calculateXORChecksum(cleanCommand, commandLength);
  
  if (calculatedCRC == receivedCRC) {
    return true;
  } else {
    Serial.print("CRC mismatch! Calc: ");
    Serial.print(calculatedCRC, HEX);
    Serial.print(", Recv: ");
    Serial.println(receivedCRC, HEX);
    return false;
  }
}

void processCommand(const char* command) {
  Serial.print("RAW: ");
  Serial.println(command);
  
  char cleanCommand[64];
  
  if (!parseAndValidateMessage(command, cleanCommand)) {
    Serial.println("INVALID - CRC failed");
    invalidCommands++;
    blinkLED(3, 100); // 3x blink cepat untuk error
    return;
  }
  
  validCommands++;
  blinkLED(1, 200); // 1x blink untuk sukses
  
  Serial.print("VALID: ");
  Serial.println(cleanCommand);
  
  // Simulate device checking
  const char* devicePrefix = isARM2_device ? "R" : "L";
  if (strncmp(cleanCommand, devicePrefix, 1) == 0) {
    Serial.print("FOR ME (");
    Serial.print(isARM2_device ? "ARM2" : "ARM1");
    Serial.print("): ");
    
    const char* separator = strchr(cleanCommand, '#');
    if (separator != NULL) {
      const char* action = separator + 1;
      Serial.print("Action = ");
      Serial.println(action);
      
      // Simulate command parsing
      if (strncmp(action, "H(", 2) == 0) {
        Serial.println("  -> HOME command detected");
      }
      else if (strncmp(action, "G(", 2) == 0) {
        Serial.println("  -> GLAD command detected");
      }
      else if (strncmp(action, "P", 1) == 0) {
        Serial.println("  -> PARK command detected");
      }
      else if (strncmp(action, "C", 1) == 0) {
        Serial.println("  -> CALIBRATION command detected");
      }
      else {
        Serial.println("  -> Unknown command");
      }
    }
  } else {
    Serial.print("NOT FOR ME - for ");
    Serial.println(cleanCommand[0] == 'L' ? "ARM1" : "ARM2");
  }
  
  Serial.println("---");
}

void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);  // LED ON
    delay(delayMs);
    digitalWrite(LED_PIN, HIGH); // LED OFF
    delay(delayMs);
  }
}

void printStats() {
  Serial.println("========== STATISTICS ==========");
  Serial.print("Total received: ");
  Serial.println(totalReceived);
  Serial.print("Valid commands: ");
  Serial.println(validCommands);
  Serial.print("Invalid commands: ");
  Serial.println(invalidCommands);
  
  if (totalReceived > 0) {
    float successRate = (float)validCommands / totalReceived * 100;
    Serial.print("Success rate: ");
    Serial.print(successRate, 1);
    Serial.println("%");
  }
  
  Serial.print("Current device: ");
  Serial.println(isARM2_device ? "ARM2 (R)" : "ARM1 (L)");
  Serial.println("===============================");
}

void setup() {
  Serial.begin(9600);
  rs485Serial.begin(9600);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED OFF initially
  
  // Simulate device detection (bisa diubah untuk testing)
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, OUTPUT);
  digitalWrite(A5, LOW);
  
  isARM2_device = (digitalRead(A4) == LOW);
  
  Serial.println("=== RS485 Control Receiver Test ===");
  Serial.print("Device simulated as: ");
  Serial.println(isARM2_device ? "ARM2 (receives R# commands)" : "ARM1 (receives L# commands)");
  Serial.println("Waiting for RS485 commands...");
  Serial.println("Commands:");
  Serial.println("  STATS  - Show statistics");
  Serial.println("  RESET  - Reset statistics");
  Serial.println("  TOGGLE - Toggle ARM1/ARM2 simulation");
  Serial.println("  HELP   - Show this help");
  Serial.println("=====================================");
  
  blinkLED(2, 500); // Startup blink
}

void loop() {
  // Process RS485 input
  if (rs485Serial.available()) {
    while (rs485Serial.available() && commandIndex < sizeof(commandBuffer) - 1) {
      char receivedChar = rs485Serial.read();
      
      if (receivedChar == '\n' || receivedChar == '\r') {
        commandBuffer[commandIndex] = '\0';
        if (commandIndex > 0) {
          totalReceived++;
          processCommand(commandBuffer);
        }
        commandIndex = 0;
        memset(commandBuffer, 0, sizeof(commandBuffer));
      } else {
        commandBuffer[commandIndex++] = receivedChar;
      }
    }
  }
  
  // Process USB Serial input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input == "STATS") {
      printStats();
    }
    else if (input == "RESET") {
      totalReceived = 0;
      validCommands = 0;
      invalidCommands = 0;
      Serial.println("Statistics reset.");
    }
    else if (input == "TOGGLE") {
      isARM2_device = !isARM2_device;
      Serial.print("Device switched to: ");
      Serial.println(isARM2_device ? "ARM2 (R)" : "ARM1 (L)");
      blinkLED(3, 200);
    }
    else if (input == "HELP") {
      Serial.println("Commands:");
      Serial.println("  STATS  - Show statistics");
      Serial.println("  RESET  - Reset statistics");
      Serial.println("  TOGGLE - Toggle ARM1/ARM2 simulation");
      Serial.println("  HELP   - Show this help");
    }
  }
  
  // Auto print stats every 5 seconds
  unsigned long currentTime = millis();
  if (currentTime - lastStatsTime >= STATS_INTERVAL) {
    if (totalReceived > 0) {
      Serial.print("Quick stats - Total: ");
      Serial.print(totalReceived);
      Serial.print(", Valid: ");
      Serial.print(validCommands);
      Serial.print(", Invalid: ");
      Serial.println(invalidCommands);
    }
    lastStatsTime = currentTime;
  }
}