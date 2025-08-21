#include <SoftwareSerial.h>

// RS485 pins (sama seperti di StateMachine asli)
const int RS485_RO = 10;
const int RS485_DI = 11;

// Create SoftwareSerial object for RS485
SoftwareSerial rs485(RS485_RO, RS485_DI);

// Timing variables
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 1000;  // 1 detik

// Test counter
int testCounter = 0;

// Test commands array
String testCommands[] = {
  "L#H(3870,390,3840,240,-30)",
  "R#H(3850,390,3840,210,-25)",
  "L#G(1620,2205,3975,240,60,270,750,3960,2340,240)",
  "R#G(1600,2200,3970,210,65,275,750,3955,2335,210)",
  "L#P",
  "R#P",
  "L#C",
  "R#C"
};

const int NUM_COMMANDS = sizeof(testCommands) / sizeof(testCommands[0]);

uint8_t calculateXORChecksum(const char* data, int length) {
  uint8_t checksum = 0;
  for (int i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

void sendRS485Command(String command) {
  uint8_t checksum = calculateXORChecksum(command.c_str(), command.length());
  String fullCommand = command + "*" + String(checksum, HEX);

  rs485.println(fullCommand);
  rs485.flush();

  Serial.print("SENT: ");
  Serial.println(fullCommand);

  delay(50);
}

void setup() {
  Serial.begin(9600);
  rs485.begin(9600);

  Serial.println("=== RS485 StateMachine Sender Test ===");
  Serial.println("Sending test commands every 1 second...");
  Serial.println("Commands will cycle through:");

  for (int i = 0; i < NUM_COMMANDS; i++) {
    Serial.print("  ");
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.println(testCommands[i]);
  }

  Serial.println("=====================================");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastSendTime >= SEND_INTERVAL) {
    // Pilih command berdasarkan counter
    String commandToSend = testCommands[testCounter % NUM_COMMANDS];

    // Tambahkan informasi test number
    Serial.print("Test #");
    Serial.print(testCounter + 1);
    Serial.print(" - ");

    // Kirim command
    sendRS485Command(commandToSend);

    // Update counter dan timer
    testCounter++;
    lastSendTime = currentTime;

    // Reset counter jika sudah mencapai batas
    if (testCounter >= NUM_COMMANDS * 3) {  // Loop 3x semua commands
      testCounter = 0;
      Serial.println("--- Restarting command cycle ---");
    }
  }

  // Baca input serial untuk kontrol manual
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "STOP") {
      Serial.println("Test stopped. Send 'START' to resume.");
      while (true) {
        if (Serial.available()) {
          String cmd = Serial.readStringUntil('\n');
          cmd.trim();
          if (cmd == "START") {
            Serial.println("Test resumed.");
            lastSendTime = millis();
            break;
          }
        }
        delay(100);
      }
    } else if (input.startsWith("SEND:")) {
      // Manual send command: SEND:L#H(1,2,3,4,5)
      String manualCmd = input.substring(5);
      Serial.print("Manual - ");
      sendRS485Command(manualCmd);
    } else if (input == "HELP") {
      Serial.println("Commands:");
      Serial.println("  STOP    - Stop automatic sending");
      Serial.println("  START   - Resume automatic sending");
      Serial.println("  SEND:x  - Send manual command x");
      Serial.println("  HELP    - Show this help");
    }
  }
}