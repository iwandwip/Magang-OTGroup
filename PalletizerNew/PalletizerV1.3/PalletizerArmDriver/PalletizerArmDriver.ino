#include <AccelStepper.h>
#include <AltSoftSerial.h>


#define CHECKSUM_SEPARATOR '*'


// Pin definitions
const int STEPPER_ENABLE_PIN = 12;
const int STEPPER_DIR_PIN = 11;
const int STEPPER_STEP_PIN = 10;
const int STRAP_COMMON_PIN = 6;
const int STRAP_1_PIN = 3;
const int STRAP_2_PIN = 4;
const int STRAP_3_PIN = 5;
const int LED_STATUS_PIN = 13;
const int LIMIT_SWITCH_PIN = A0;
const int SPEED_SELECT_PIN_1 = A1;  // First pin for speed selection
const int SPEED_SELECT_PIN_2 = A2;  // Second pin for speed selection


// Motor configuration constants
const int HOMING_STEPS = 10000;
const int HOMING_OFFSET = 5000;


// Driver ID mapping
const uint8_t DRIVER_ID_X = 0b111;
const uint8_t DRIVER_ID_Y = 0b110;
const uint8_t DRIVER_ID_Z = 0b101;
const uint8_t DRIVER_ID_G = 0b100;
const uint8_t DRIVER_ID_T = 0b011;


// Communication constants
const int SERIAL_BAUD_RATE = 9600;
const char COMMAND_DELIMITER = ',';
const char LINE_DELIMITER = '\n';
const int LED_MINIMUM_DELAY = 200;


// Hardware objects
AccelStepper stepperMotor(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIR_PIN);
AltSoftSerial serialComm;


// Global variables
char driverID = '\0';
String commandBuffer = "";
int MOVE_MAX_SPEED;     // Variable speed based on A1-A2 connection
int MOVE_ACCELERATION;  // Always half of MOVE_MAX_SPEED
int MOVE_HOME_SPEED;
int MOVE_HOME_ACCELERATION;
unsigned long ledOffTime = 0;  // Waktu ketika LED dimatikan
int TOLERANCE = 5;


uint8_t calculateXORChecksum(const char* data, int length) {
  uint8_t checksum = 0;
  for (int i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}


uint8_t hexStringToUint8(String hexStr) {
  return (uint8_t)strtol(hexStr.c_str(), NULL, 16);
}


// Fungsi untuk memvalidasi dan mengeksekusi command dengan checksum
bool validateAndExecuteCommand(String receivedData) {
  // Cari separator antara command dan checksum
  int separatorIndex = receivedData.indexOf(CHECKSUM_SEPARATOR);

  if (separatorIndex == -1) {
    Serial.println("Error: No checksum separator found");
    return false;
  }

  // Pisahkan command dan checksum
  String command = receivedData.substring(0, separatorIndex);
  String checksumStr = receivedData.substring(separatorIndex + 1);

  // Hapus whitespace
  command.trim();
  checksumStr.trim();

  // Konversi checksum dari hex string ke uint8_t
  uint8_t receivedChecksum = hexStringToUint8(checksumStr);

  // Hitung checksum dari command yang diterima
  uint8_t calculatedChecksum = calculateXORChecksum(command.c_str(), command.length());

  // Validasi checksum
  if (receivedChecksum == calculatedChecksum) {
    executeCommand(command);
    return true;
  } else {
    Serial.println("Checksum mismatch - command rejected");
    return false;
  }
}


void setup() {
  initializePins();
  initializeSerial();
  setDriverID();
  setSpeedParameters();
  initializeStepperMotor();

  Serial.print("Driver ID: ");
  Serial.println(driverID);
  Serial.print("Move Max Speed: ");
  Serial.println(MOVE_MAX_SPEED);
  Serial.print("Move Acceleration: ");
  Serial.println(MOVE_ACCELERATION);
}


void loop() {
  processSerialInput();
}


void initializePins() {
  // Motor control pins
  pinMode(STEPPER_ENABLE_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(STEPPER_ENABLE_PIN, LOW);  // Enable motor (active LOW)
  digitalWrite(LED_STATUS_PIN, HIGH);     // LED ON = idle state

  // Strap pins configuration
  pinMode(STRAP_COMMON_PIN, OUTPUT);
  digitalWrite(STRAP_COMMON_PIN, LOW);
  pinMode(STRAP_1_PIN, INPUT_PULLUP);
  pinMode(STRAP_2_PIN, INPUT_PULLUP);
  pinMode(STRAP_3_PIN, INPUT_PULLUP);

  // Limit switch pin
  pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);  // Active LOW

  // Speed selection pins
  pinMode(SPEED_SELECT_PIN_1, INPUT_PULLUP);  // A1 with pullup
  pinMode(SPEED_SELECT_PIN_2, INPUT_PULLUP);  // A2 with pullup
}


void initializeSerial() {
  Serial.begin(SERIAL_BAUD_RATE);
  serialComm.begin(SERIAL_BAUD_RATE);
}


void setDriverID() {
  uint8_t strapCode = (digitalRead(STRAP_3_PIN) << 2) | (digitalRead(STRAP_2_PIN) << 1) | (digitalRead(STRAP_1_PIN));

  switch (strapCode) {
    case DRIVER_ID_X: driverID = 'X'; break;
    case DRIVER_ID_Y: driverID = 'Y'; break;
    case DRIVER_ID_Z: driverID = 'Z'; break;
    case DRIVER_ID_G: driverID = 'G'; break;
    case DRIVER_ID_T: driverID = 'T'; break;
    default: driverID = '?'; break;
  }
}


void setSpeedParameters() {
  // Check if A1 is connected to A2 (both will read the same value when connected)
  // We'll use a method to detect if they're connected together

  // First, set A1 as OUTPUT LOW and A2 as INPUT_PULLUP
  pinMode(SPEED_SELECT_PIN_1, OUTPUT);
  digitalWrite(SPEED_SELECT_PIN_1, LOW);
  pinMode(SPEED_SELECT_PIN_2, INPUT_PULLUP);
  delay(10);  // Small delay for stabilization

  bool a2_when_a1_low = digitalRead(SPEED_SELECT_PIN_2);

  // Now set A1 as OUTPUT HIGH and check A2 again
  digitalWrite(SPEED_SELECT_PIN_1, HIGH);
  delay(10);  // Small delay for stabilization

  bool a2_when_a1_high = digitalRead(SPEED_SELECT_PIN_2);

  // Restore both pins to INPUT_PULLUP mode
  pinMode(SPEED_SELECT_PIN_1, INPUT_PULLUP);
  pinMode(SPEED_SELECT_PIN_2, INPUT_PULLUP);


  if (driverID == 'X') {
    MOVE_MAX_SPEED = 2600;
    MOVE_ACCELERATION = MOVE_MAX_SPEED * 0.5;
    MOVE_HOME_SPEED = 300;
    MOVE_HOME_ACCELERATION = 0.1 * MOVE_HOME_SPEED;
    TOLERANCE = 3;
  } else if (driverID == 'Y') {
    MOVE_MAX_SPEED = 4000;
    MOVE_ACCELERATION = MOVE_MAX_SPEED * 0.5;
    MOVE_HOME_SPEED = 300;
    MOVE_HOME_ACCELERATION = 0.5 * MOVE_HOME_SPEED;
    TOLERANCE = 10;
  } else if (driverID == 'Z') {
    MOVE_MAX_SPEED = 4000;
    MOVE_ACCELERATION = MOVE_MAX_SPEED * 0.5;
    MOVE_HOME_SPEED = 300;
    MOVE_HOME_ACCELERATION = 0.1 * MOVE_HOME_SPEED;
    TOLERANCE = 0;
  } else if (driverID == 'T') {
    MOVE_MAX_SPEED = 5000;
    MOVE_ACCELERATION = MOVE_MAX_SPEED * 0.3;
    MOVE_HOME_SPEED = 4000;
    MOVE_HOME_ACCELERATION = 0.5 * MOVE_HOME_SPEED;
    TOLERANCE = 100;
  } else if (driverID == 'G') {
    MOVE_MAX_SPEED = 4000;
    MOVE_ACCELERATION = MOVE_MAX_SPEED * 0.5;
    MOVE_HOME_SPEED = 150;
    MOVE_HOME_ACCELERATION = 0.5 * MOVE_HOME_SPEED;
    TOLERANCE = 10;
  }


  // If A1 and A2 are connected, A2 should follow A1's state
  // If they're not connected, A2 will remain HIGH due to pullup
  if (a2_when_a1_low == LOW && a2_when_a1_high == HIGH) {
    // A1 and A2 are connected - use high speed
    Serial.println("Speed Selection: A1-A2 connected - HIGH SPEED");
    MOVE_MAX_SPEED = MOVE_MAX_SPEED;
  } else {
    // A1 and A2 are not connected - use normal speed
    Serial.println("Speed Selection: A1-A2 not connected - NORMAL SPEED");
    MOVE_MAX_SPEED = 0.5 * MOVE_MAX_SPEED;
  }

  // MOVE_ACCELERATION is always half of MOVE_MAX_SPEED
}


void initializeStepperMotor() {
  stepperMotor.setMaxSpeed(MOVE_HOME_SPEED);
  stepperMotor.setAcceleration(MOVE_HOME_ACCELERATION);
  stepperMotor.setCurrentPosition(0);
}


void processSerialInput() {
  while (serialComm.available()) {
    char receivedChar = serialComm.read();


    if (receivedChar == LINE_DELIMITER || receivedChar == '\r') {
      if (commandBuffer.length() > 0) {
        // Validasi dan eksekusi command dengan checksum
        validateAndExecuteCommand(commandBuffer);

        commandBuffer = "";
      }
    } else if (receivedChar != '\r') {  // Ignore carriage return
      commandBuffer += receivedChar;
    }
  }
}


void executeCommand(String command) {
  Serial.print("Processing command: ");
  Serial.println(command);

  // Parse command yang bisa berisi multiple driver commands
  // Format: X1000,Y500,Z300 atau X1000

  int startIndex = 0;
  int commaIndex = 0;

  do {
    // Cari koma berikutnya atau akhir string
    commaIndex = command.indexOf(',', startIndex);

    String singleCommand;
    if (commaIndex != -1) {
      // Ada koma, ambil substring sampai koma
      singleCommand = command.substring(startIndex, commaIndex);
    } else {
      // Tidak ada koma, ambil sisa string
      singleCommand = command.substring(startIndex);
    }

    // Trim whitespace
    singleCommand.trim();

    Serial.print("Parsing single command: ");
    Serial.println(singleCommand);

    // Proses single command jika panjang minimal 2 karakter
    if (singleCommand.length() >= 2) {
      char targetDriverID = singleCommand.charAt(0);

      Serial.print("Target Driver ID: ");
      Serial.print(targetDriverID);
      Serial.print(", My Driver ID: ");
      Serial.println(driverID);

      // Cek apakah command ini untuk driver ini
      if (targetDriverID == driverID) {
        Serial.println("Command is for this driver - executing...");

        // Ambil posisi target dari karakter kedua sampai akhir
        long targetPosition = singleCommand.substring(1).toInt();

        Serial.print("Target position: ");
        Serial.println(targetPosition);

        if (targetPosition == 0) {
          performHoming();
        } else {
          setStatusLED(false);  // Turn off LED during movement
          moveToPosition(targetPosition);
          setStatusLED(true);  // Turn on LED when idle
        }

        // Break setelah menemukan command untuk driver ini
        // (asumsi hanya ada satu command per driver dalam satu perintah)
        break;
      } else {
        Serial.println("Command not for this driver - skipping");
      }
    }

    // Update startIndex untuk parsing berikutnya
    startIndex = commaIndex + 1;

  } while (commaIndex != -1 && startIndex < command.length());
}


void moveToPosition(long targetPosition) {
  Serial.print("Moving to position: ");
  Serial.println(targetPosition);


  stepperMotor.setMaxSpeed(MOVE_MAX_SPEED);
  stepperMotor.setAcceleration(MOVE_ACCELERATION);


  stepperMotor.moveTo(targetPosition);


  while (stepperMotor.distanceToGo() != 0) {
    stepperMotor.run();
    if (stepperMotor.distanceToGo() <= TOLERANCE) {
      setStatusLED(true);
    }
  }
}


void performHoming() {
  Serial.println("Starting homing sequence...");

  setStatusLED(false);  // Turn off LED during homing

  // Set homing speed parameters (same for all drivers now)
  stepperMotor.setMaxSpeed(MOVE_HOME_SPEED);
  stepperMotor.setAcceleration(MOVE_HOME_ACCELERATION);

  if (isAtHomePosition()) {
    moveAwayFromHome();
    returnToHome();
  } else {
    returnToHome();
  }

  stepperMotor.setCurrentPosition(0);
  stepperMotor.stop();

  // Ensure motor has stopped
  while (stepperMotor.isRunning()) {
    stepperMotor.run();
  }

  Serial.println("Returning to position 0 after homing");
  stepperMotor.moveTo(0);
  stepperMotor.runToPosition();

  setStatusLED(true);  // Turn on LED when idle
}


bool isAtHomePosition() {
  return digitalRead(LIMIT_SWITCH_PIN) == HIGH;
}


void moveAwayFromHome() {
  stepperMotor.move(HOMING_STEPS);

  do {
    stepperMotor.run();
  } while (digitalRead(LIMIT_SWITCH_PIN) == HIGH);

  stepperMotor.stop();
  stepperMotor.runToPosition();

  stepperMotor.move(-HOMING_STEPS - HOMING_OFFSET);

  do {
    stepperMotor.run();
  } while (digitalRead(LIMIT_SWITCH_PIN) == LOW);
}


void returnToHome() {
  stepperMotor.move(-HOMING_STEPS);

  do {
    stepperMotor.run();
  } while (digitalRead(LIMIT_SWITCH_PIN) == LOW);
}


void setStatusLED(bool state) {
  if (!state) {
    // LED dimatikan, catat waktunya
    ledOffTime = millis();
    digitalWrite(LED_STATUS_PIN, LOW);
  } else {
    // LED akan dinyalakan, cek apakah sudah lewat 100ms
    unsigned long elapsedTime = millis() - ledOffTime;
    if (elapsedTime < LED_MINIMUM_DELAY) {
      delay(LED_MINIMUM_DELAY - elapsedTime);
    }
    digitalWrite(LED_STATUS_PIN, HIGH);
  }
}
