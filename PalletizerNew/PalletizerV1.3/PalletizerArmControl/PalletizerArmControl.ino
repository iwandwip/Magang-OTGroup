#include <AltSoftSerial.h>
#include <SoftwareSerial.h>


// ==================== CONSTANTS ====================
// Serial Communication
const int RS485_RX_PIN = 10;
const int RS485_TX_PIN = 11;
// RE/DE pin removed - hardwired to GND
const long SERIAL_BAUD_RATE = 9600;


// Output Pins
const int BUZZER_PIN = 4;
const int RED_LED_PIN = 5;
const int YELLOW_LED_PIN = 6;
const int GREEN_LED_PIN = 7;
const int COMMAND_ACTIVE_PIN = 13;


// Input Pins
const int START_OUTPUT_PIN = A0;
const int START_INPUT_PIN = A1;
const int STOP_OUTPUT_PIN = A2;
const int STOP_INPUT_PIN = A3;
const int MOTOR_DONE_PIN = 3;


//RETRY SYSTEM
const int MAX_MOTOR_RETRIES = 10;  // Maksimum 7x retry
int motorRetryCount = 0;


// Timing Constants
const unsigned long DEBOUNCE_DELAY_MS = 100;
const unsigned long DRIVER_TIMEOUT_MS = 10000;
const unsigned long BLINK_INTERVAL_MS = 200;


// Motor Commands terpisah
const char MOTOR_PARK_Z_COMMAND[] PROGMEM = "Z0";
const char MOTOR_PARK_XGT_COMMAND[] PROGMEM = "X0,T0,G0";
const char MOTOR_PARK_Y_COMMAND[] PROGMEM = "Y0";


// ==================== GLOBAL VARIABLES ====================
// State Machine
enum SystemState {
  STATE_ZEROING,
  STATE_SLEEPING,
  STATE_READY,
  STATE_RUNNING
};
bool stopRequested = false;
bool isCalibrationMode = false;  // Flag untuk membedakan CAL dari PARK


// PARK Sequence Variables
bool isParkSequenceActive = false;
unsigned long parkSequenceStartTime = 0;
byte parkSequenceStep = 0;
const unsigned long PARK_SEQUENCE_DELAY_MS = 2000;  // 2 detik


unsigned long lastMotorCheckTime = 0;
bool lastMotorState = HIGH;
const unsigned long MOTOR_CHECK_INTERVAL = 20;  // Check setiap 20ms


SystemState currentState = STATE_ZEROING;
unsigned long stateTimer = 0;


// Serial Communication
AltSoftSerial motorSerial;
SoftwareSerial rs485Serial(RS485_RX_PIN, RS485_TX_PIN);


uint8_t calculateXORChecksum(const char* data, int length) {
  uint8_t checksum = 0;
  for (int i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}


bool parseAndValidateMessage(const char* receivedMessage, char* cleanCommand) {
  // Cari separator '*' dari belakang
  const char* separator = strrchr(receivedMessage, '*');

  if (separator == NULL) {
    Serial.println(F("ERROR: No CRC separator found"));
    return false;
  }

  // Hitung panjang command (tanpa CRC)
  int commandLength = separator - receivedMessage;

  // Copy clean command
  strncpy(cleanCommand, receivedMessage, commandLength);
  cleanCommand[commandLength] = '\0';

  // Extract received CRC (skip '*')
  const char* receivedCRCStr = separator + 1;

  // Convert hex string to number
  uint8_t receivedCRC = (uint8_t)strtol(receivedCRCStr, NULL, 16);

  // Calculate CRC dari clean command
  uint8_t calculatedCRC = calculateXORChecksum(cleanCommand, commandLength);

  // Validasi
  if (calculatedCRC == receivedCRC) {
    return true;
  } else {
    Serial.print(F("CRC mismatch! Calc: "));
    Serial.print(calculatedCRC, HEX);
    Serial.print(F(", Recv: "));
    Serial.println(receivedCRC, HEX);
    return false;
  }
}


// Command Processing - Reduced buffer size
char commandBuffer[64];
byte commandIndex = 0;


// Multi-step command execution
enum CommandSequence {
  SEQ_NONE,
  SEQ_HOME,
  SEQ_GLAD
};


bool waitingForMotorReady = false;
unsigned long motorReadyWaitStart = 0;
const unsigned long MOTOR_READY_WAIT_MS = 100;  // 100ms delay


// Command timing control
unsigned long lastCommandSentTime = 0;
unsigned long lastMotorReadyTime = 0;
bool motorWasReady = false;
const unsigned long MIN_COMMAND_INTERVAL_MS = 100;  // Minimum 100ms between commands
const unsigned long MOTOR_STABILIZE_MS = 50;        // Wait after motor ready


struct ButtonState {
  bool lastReading;
  bool currentState;
  bool lastState;
  unsigned long lastDebounceTime;
  bool buttonPressed;  // Flag untuk edge detection
};
// Inisialisasi state untuk setiap button
ButtonState startButtonState = { HIGH, HIGH, HIGH, 0, false };
ButtonState stopButtonState = { HIGH, HIGH, HIGH, 0, false };


// Compact command structures using int16_t instead of int
struct HomeCommand {
  int16_t x, y, z, t, g;
  byte step;
};


struct GladCommand {
  int16_t xn, yn, zn, tn, dp, gp, za, zb, xa, ta;
  byte step;
};


CommandSequence currentSequence = SEQ_NONE;
HomeCommand homeCmd;
GladCommand gladCmd;


bool isARM2_device = false;


// ==================== SETUP ====================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println(F("Controller Starting... (Modified - Receive Only)"));

  initializePins();


  isARM2_device = isARM2();
  Serial.print(F("Device detected as: "));
  Serial.println(isARM2_device ? F("ARM2") : F("ARM1"));


  initializeSerial();

  // Initialize button states
  startButtonState = { HIGH, HIGH, HIGH, 0, false };
  stopButtonState = { HIGH, HIGH, HIGH, 0, false };


  turnOffAllOutputs();
  enterZeroingState();
  isCalibrationMode = false;  // Initialize flag
}


// ==================== MAIN LOOP ====================
void loop() {
  processSerialCommands();
  processUSBCommands();  // Tambahkan baris ini
  updateStateMachine();
  updateCommandSequence();
  updateParkSequence();
}


// ==================== INITIALIZATION ====================
void initializePins() {
  // Configure output pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(COMMAND_ACTIVE_PIN, OUTPUT);

  // Configure input pins
  pinMode(START_OUTPUT_PIN, OUTPUT);
  pinMode(START_INPUT_PIN, INPUT_PULLUP);
  pinMode(STOP_OUTPUT_PIN, OUTPUT);
  pinMode(STOP_INPUT_PIN, INPUT_PULLUP);
  pinMode(MOTOR_DONE_PIN, INPUT_PULLUP);


  // Set output pins to HIGH initially (idle state)
  digitalWrite(START_OUTPUT_PIN, HIGH);
  digitalWrite(STOP_OUTPUT_PIN, HIGH);


  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, OUTPUT);
  digitalWrite(A5, LOW);  // Set A5 sebagai ground reference
}


bool isARM2() {
  return digitalRead(A4) == LOW;  // Jika A4 terhubung ke A5 (LOW)
}


void initializeSerial() {
  // Initialize motor control serial
  motorSerial.begin(SERIAL_BAUD_RATE);

  // Initialize RS485 communication (receive only)
  rs485Serial.begin(SERIAL_BAUD_RATE);
  // RE/DE pin removed - hardwired to GND for permanent receive mode
  Serial.println(F("RS485 in permanent receive mode (RE/DE = GND)"));
}


// ==================== STATE MACHINE ====================
void updateStateMachine() {
  switch (currentState) {
    case STATE_ZEROING:
      handleZeroingState();
      break;

    case STATE_SLEEPING:
      handleSleepingState();
      break;

    case STATE_READY:
      handleReadyState();
      break;

    case STATE_RUNNING:
      handleRunningState();
      break;
  }
}


void handleZeroingState() {
  updateBlinkingLeds();

  if (!isParkSequenceActive && isMotorReady()) {
    if (isCalibrationMode) {
      // Jika dari CAL command, langsung ke READY
      Serial.println(F("Calibration completed - entering READY state"));
      isCalibrationMode = false;  // Reset flag
      enterReadyState();
    } else {
      // Jika dari PARK atau startup, ke SLEEPING
      enterSleepingState();
    }
  }
}


void handleSleepingState() {
  setLedState(true, false, false);  // Red LED only
  setBuzzerState(false);


  // Indikasi sistem siap menerima USB test commands
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {  // Print setiap 5 detik
    Serial.println(F("SLEEPING - Ready for USB motor test commands"));
    lastPrint = millis();
  }

  if (isStartButtonPressed()) {
    enterReadyState();
  }
}


void handleReadyState() {
  // Indikasi visual berdasarkan status
  setLedState(true, true, false);  // Red & Yellow LEDs
  setBuzzerState(false);
  // Serial commands are processed in processSerialCommands()
  // READY -> RUNNING transition happens when HOME command is completed
}


void handleRunningState() {
  // Indikasi visual berdasarkan status
  if (stopRequested && currentSequence != SEQ_NONE) {
    // Blink green LED jika STOP pending dan masih ada sequence
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 300) {
      static bool blinkState = false;
      setLedState(false, false, blinkState);
      blinkState = !blinkState;
      lastBlink = millis();
    }
  } else {
    setLedState(false, false, true);  // Green LED only (normal)
  }

  // Check for STOP button press to transition to FINISHING
  if (isStopButtonPressed()) {
    if (!stopRequested) {  // Hanya proses sekali
      Serial.println(F("STOP button pressed in RUNNING state"));
      stopRequested = true;

      // Jika sedang ada sequence yang berjalan, biarkan selesai dulu
      if (currentSequence != SEQ_NONE) {
        Serial.println(F("STOP requested - will finish current GLAD sequence first"));
        return;  // Jangan langsung ke FINISHING
      }
    }
  }

  // Jika STOP sudah direquest dan tidak ada sequence, langsung ke FINISHING
  if (stopRequested && currentSequence == SEQ_NONE) {
    Serial.println(F("GLAD sequence completed - now proceeding to FINISHING state"));
    stopRequested = false;  // Reset flag
    enterZeroingState();
    return;
  }


  // Serial commands are processed in processSerialCommands()
  // RUNNING -> READY transition happens when GLAD command is completed
  setBuzzerState(false);
}


void updateParkSequence() {
  if (!isParkSequenceActive) return;

  switch (parkSequenceStep) {
    case 1:
      // Step 1: Z0 sudah dikirim di enterZeroingState()
      // Tunggu delay
      if (millis() - parkSequenceStartTime >= PARK_SEQUENCE_DELAY_MS) {
        parkSequenceStep = 2;
        parkSequenceStartTime = millis();  // Reset timer untuk step berikutnya
        sendMotorCommandPGM(MOTOR_PARK_XGT_COMMAND);
        Serial.println(F("PARK Step 2: Sending X0,T0,G0"));
      }
      break;


    case 2:
      // Tunggu delay setelah X0,T0,G0
      if (millis() - parkSequenceStartTime >= PARK_SEQUENCE_DELAY_MS) {
        parkSequenceStep = 3;
        parkSequenceStartTime = millis();  // Reset timer untuk step berikutnya
        sendMotorCommandPGM(MOTOR_PARK_Y_COMMAND);
        Serial.println(F("PARK Step 3: Sending Y0"));
      }
      break;

    case 3:
      // Sequence selesai
      isParkSequenceActive = false;
      parkSequenceStep = 0;
      Serial.println(F("PARK sequence completed"));
      break;
  }
}


// ==================== STATE TRANSITIONS ====================
void enterZeroingState() {
  currentState = STATE_ZEROING;
  stateTimer = millis();

  Serial.println(F("Entering ZEROING state"));

  // Set COMMAND_ACTIVE_PIN HIGH when entering ZEROING state
  digitalWrite(COMMAND_ACTIVE_PIN, HIGH);
  // Mulai PARK sequence
  isParkSequenceActive = true;
  parkSequenceStartTime = millis();
  parkSequenceStep = 1;

  // Kirim Z0 command terlebih dahulu
  sendMotorCommandPGM(MOTOR_PARK_Z_COMMAND);
}


void enterSleepingState() {
  currentState = STATE_SLEEPING;
  Serial.println(F("Entering SLEEPING state"));

  // Keep COMMAND_ACTIVE_PIN HIGH in SLEEPING state
  digitalWrite(COMMAND_ACTIVE_PIN, HIGH);
  Serial.println(F("Command active pin HIGH - SLEEPING state"));
}


void enterReadyState() {
  currentState = STATE_READY;
  stateTimer = millis();
  Serial.println(F("Entering READY state"));

  // Set COMMAND_ACTIVE_PIN LOW when entering READY state
  digitalWrite(COMMAND_ACTIVE_PIN, LOW);
  Serial.println(F("Command active pin LOW - READY state"));
}


void enterRunningState() {
  currentState = STATE_RUNNING;
  Serial.println(F("Entering RUNNING state"));


  // Set COMMAND_ACTIVE_PIN LOW when entering RUNNING state
  digitalWrite(COMMAND_ACTIVE_PIN, LOW);
}


// ==================== SERIAL COMMUNICATION ====================
void processSerialCommands() {
  // Only process serial commands in READY or RUNNING states
  if (currentState != STATE_READY && currentState != STATE_RUNNING) {
    return;
  }

  bool armBusy = !isMotorReady();
  if (armBusy) {
    return;
  }


  if (rs485Serial.available()) {
    while (rs485Serial.available() && commandIndex < sizeof(commandBuffer) - 1) {
      char receivedChar = rs485Serial.read();

      if (receivedChar == '\n' || receivedChar == '\r') {
        commandBuffer[commandIndex] = '\0';
        processCommand(commandBuffer);
        commandIndex = 0;
        memset(commandBuffer, 0, sizeof(commandBuffer));
      } else {
        commandBuffer[commandIndex++] = receivedChar;
      }
    }
  }
}


void processCommand(const char* command) {
  Serial.print(F("Received command: "));
  Serial.println(command);


  // Buffer untuk clean command
  char cleanCommand[64];


  // Validasi CRC dulu
  if (!parseAndValidateMessage(command, cleanCommand)) {
    Serial.println(F("Command rejected - CRC invalid"));
    return;  // Tolak command jika CRC tidak valid
  }


  Serial.print(F("Valid command: "));
  Serial.println(cleanCommand);

  // Check if command is for ARM2
  const char* devicePrefix = isARM2_device ? "R" : "L";
  if (strncmp(command, devicePrefix, 1) == 0) {
    // Look for command separator
    const char* separator = strchr(command, '#');
    if (separator != NULL) {
      const char* action = separator + 1;
      executeCommand(action);
    }
  }
}


void executeCommand(const char* action) {
  Serial.print(F("Executing action: "));
  Serial.println(action);

  if (strncmp_P(action, PSTR("P"), 1) == 0) {
    // PARK command: go directly to ZEROING state
    Serial.println(F("PARK command received - entering ZEROING state"));

    isCalibrationMode = false;  // Reset flag untuk PARK
    enterZeroingState();

  } else if (strncmp_P(action, PSTR("C"), 1) == 0) {
    // CAL command: masuk ke ZEROING state, tapi akan ke READY setelah selesai
    Serial.println(F("CAL command received - entering ZEROING state"));

    isCalibrationMode = true;  // Set flag CAL
    enterZeroingState();

  } else if (strncmp_P(action, PSTR("H"), 1) == 0) {
    // HOME command: READY -> RUNNING transition
    if (currentState == STATE_READY) {
      if (parseHomeCommand(action)) {
        Serial.println(F("HOME command parsed successfully - keeping command active pin HIGH"));
        startHomeSequence();
      } else {
        digitalWrite(COMMAND_ACTIVE_PIN, HIGH);  // Turn off on error
      }
    } else {
      Serial.println(F("ERROR: HOME command only valid in READY state"));
      digitalWrite(COMMAND_ACTIVE_PIN, HIGH);  // Turn off on error
    }

  } else if (strncmp_P(action, PSTR("G"), 1) == 0) {
    // GLAD command: can be executed in RUNNING state (RUNNING -> READY transition)
    if (currentState == STATE_RUNNING) {
      if (parseGladCommand(action)) {
        Serial.println(F("GLAD command parsed successfully - keeping command active pin HIGH"));
        // Stay in RUNNING state but execute GLAD sequence
        // After completion, system will transition to READY
        startGladSequence();
      } else {
        digitalWrite(COMMAND_ACTIVE_PIN, HIGH);  // Turn off on error
      }
    } else {
      Serial.println(F("ERROR: GLAD command only valid in RUNNING state"));
      digitalWrite(COMMAND_ACTIVE_PIN, HIGH);  // Turn off on error
    }

  } else {
    Serial.print(F("ERROR: Unknown action: "));
    Serial.println(action);
    digitalWrite(COMMAND_ACTIVE_PIN, HIGH);  // Turn off on error
  }
}


void sendMotorCommand(const char* command) {
  unsigned long timeSinceLastCommand = millis() - lastCommandSentTime;
  if (timeSinceLastCommand < MIN_COMMAND_INTERVAL_MS) {
    delay(MIN_COMMAND_INTERVAL_MS - timeSinceLastCommand);
  }

  uint8_t checksum = calculateXORChecksum(command, strlen(command));
  digitalWrite(COMMAND_ACTIVE_PIN, HIGH);

  for (int attempt = 1; attempt <= MAX_MOTOR_RETRIES; attempt++) {
    Serial.print(F("Sending motor command (attempt "));
    Serial.print(attempt);
    Serial.print("): ");
    Serial.println(command);

    // 🔧 PERBAIKAN: Kirim perintah pada setiap attempt
    motorSerial.print(command);
    motorSerial.print("*");
    motorSerial.print(checksum, HEX);
    motorSerial.println();

    if (waitForMotorResponse(200)) {
      motorRetryCount = 0;
      lastCommandSentTime = millis();
      return;  // Berhasil
    }

    Serial.print("Retry attempt: ");
    Serial.println(attempt);

    if (attempt == MAX_MOTOR_RETRIES) {
      handleMotorTimeout();
    } else {
      delay(50);  // Delay sebelum retry berikutnya
    }
  }
}


void sendMotorCommandPGM(const char* command) {
  char buffer[64];
  strcpy_P(buffer, command);
  sendMotorCommand(buffer);
}


// ==================== INPUT HANDLING ====================


// ==================== INPUT HANDLING (MODIFIED FOR 4-SECOND HOLD) ====================


bool isStartButtonPressed() {
  static unsigned long pressStartTime = 0;
  static bool wasPressed = false;
  static bool holdActivated = false;
  static unsigned long lastValidActivation = 0;

  const unsigned long HOLD_DURATION_MS = 4000;           // 4 detik
  const unsigned long DEBOUNCE_AFTER_ACTIVATION = 1000;  // 1 detik debounce setelah aktivasi

  bool currentlyPressed = false;

  // Test tombol dengan metode capacitive
  digitalWrite(START_OUTPUT_PIN, LOW);
  delay(10);  // 10ms untuk capacitor
  bool testLow = (digitalRead(START_INPUT_PIN) == LOW);

  digitalWrite(START_OUTPUT_PIN, HIGH);
  delay(10);
  bool testHigh = (digitalRead(START_INPUT_PIN) == HIGH);

  currentlyPressed = testLow && testHigh;

  // State machine untuk hold detection
  if (currentlyPressed && !wasPressed) {
    // Tombol baru saja ditekan
    pressStartTime = millis();
    holdActivated = false;
    Serial.println(F("START button pressed - hold for 4 seconds..."));
  } else if (currentlyPressed && wasPressed) {
    // Tombol masih ditekan, cek durasi
    unsigned long holdTime = millis() - pressStartTime;

    // Berikan feedback setiap detik
    static unsigned long lastFeedback = 0;
    if (millis() - lastFeedback > 1000) {
      Serial.print(F("START hold time: "));
      Serial.print(holdTime / 1000);
      Serial.println(F(" seconds"));
      lastFeedback = millis();
    }

    // Cek apakah sudah mencapai 4 detik dan belum diaktivasi
    if (holdTime >= HOLD_DURATION_MS && !holdActivated) {
      holdActivated = true;
      lastValidActivation = millis();
      Serial.println(F("START button ACTIVATED after 4-second hold!"));
      return true;  // Langsung return true setelah 4 detik hold
    }
  } else if (!currentlyPressed && wasPressed) {
    // Tombol dilepas
    if (!holdActivated) {
      Serial.println(F("START button released before 4 seconds"));
    }
    wasPressed = false;
    holdActivated = false;
  }

  // Prevent multiple activations dalam periode debounce
  if (millis() - lastValidActivation < DEBOUNCE_AFTER_ACTIVATION) {
    wasPressed = currentlyPressed;
    return false;
  }

  wasPressed = currentlyPressed;
  return false;
}


bool isStopButtonPressed() {
  static unsigned long pressStartTime = 0;
  static bool wasPressed = false;
  static bool holdActivated = false;
  static unsigned long lastValidActivation = 0;

  const unsigned long HOLD_DURATION_MS = 4000;           // 4 detik
  const unsigned long DEBOUNCE_AFTER_ACTIVATION = 1000;  // 1 detik debounce setelah aktivasi

  bool currentlyPressed = false;

  // Test tombol dengan metode capacitive
  digitalWrite(STOP_OUTPUT_PIN, LOW);
  delay(10);  // 10ms untuk capacitor
  bool testLow = (digitalRead(STOP_INPUT_PIN) == LOW);

  digitalWrite(STOP_OUTPUT_PIN, HIGH);
  delay(10);
  bool testHigh = (digitalRead(STOP_INPUT_PIN) == HIGH);

  currentlyPressed = testLow && testHigh;

  // State machine untuk hold detection
  if (currentlyPressed && !wasPressed) {
    // Tombol baru saja ditekan
    pressStartTime = millis();
    holdActivated = false;
    Serial.println(F("STOP button pressed - hold for 4 seconds..."));
  } else if (currentlyPressed && wasPressed) {
    // Tombol masih ditekan, cek durasi
    unsigned long holdTime = millis() - pressStartTime;

    // Berikan feedback setiap detik
    static unsigned long lastFeedback = 0;
    if (millis() - lastFeedback > 1000) {
      Serial.print(F("STOP hold time: "));
      Serial.print(holdTime / 1000);
      Serial.println(F(" seconds"));
      lastFeedback = millis();
    }

    // Cek apakah sudah mencapai 4 detik dan belum diaktivasi
    if (holdTime >= HOLD_DURATION_MS && !holdActivated) {
      holdActivated = true;
      lastValidActivation = millis();
      Serial.println(F("STOP button ACTIVATED after 4-second hold!"));
      return true;  // Langsung return true setelah 4 detik hold
    }
  } else if (!currentlyPressed && wasPressed) {
    // Tombol dilepas
    if (!holdActivated) {
      Serial.println(F("STOP button released before 4 seconds"));
    }
    wasPressed = false;
    holdActivated = false;
  }

  // Prevent multiple activations dalam periode debounce
  if (millis() - lastValidActivation < DEBOUNCE_AFTER_ACTIVATION) {
    wasPressed = currentlyPressed;
    return false;
  }

  wasPressed = currentlyPressed;
  return false;
}


bool isMotorReady() {
  // Hanya check setiap 20ms untuk mengurangi noise
  if (millis() - lastMotorCheckTime < MOTOR_CHECK_INTERVAL) {
    return lastMotorState == HIGH;
  }

  // Baca pin 3 kali dengan delay kecil
  bool reading1 = digitalRead(MOTOR_DONE_PIN);
  delay(20);
  bool reading2 = digitalRead(MOTOR_DONE_PIN);
  delay(20);
  bool reading3 = digitalRead(MOTOR_DONE_PIN);

  // Gunakan majority vote (2 dari 3 harus sama)
  bool stableReading;
  if (reading1 == reading2 || reading1 == reading3) {
    stableReading = reading1;
  } else {
    stableReading = reading2;
  }

  lastMotorState = stableReading;
  lastMotorCheckTime = millis();

  return stableReading == HIGH;
}


// ==================== OUTPUT CONTROL ====================
void setLedState(bool red, bool yellow, bool green) {
  // LEDs are active LOW
  digitalWrite(RED_LED_PIN, red ? LOW : HIGH);
  digitalWrite(YELLOW_LED_PIN, yellow ? LOW : HIGH);
  digitalWrite(GREEN_LED_PIN, green ? LOW : HIGH);
}


void setBuzzerState(bool active) {
  // Buzzer is active LOW
  digitalWrite(BUZZER_PIN, active ? LOW : HIGH);
}


void turnOffAllOutputs() {
  setLedState(false, false, false);
  setBuzzerState(false);
  digitalWrite(COMMAND_ACTIVE_PIN, HIGH);
}


void updateBlinkingLeds() {
  static unsigned long lastToggle = 0;
  static bool ledState = false;

  if (millis() - lastToggle >= BLINK_INTERVAL_MS) {
    ledState = !ledState;
    setLedState(ledState, ledState, ledState);
    setBuzzerState(ledState);
    lastToggle = millis();
  }
}


// ==================== COMMAND PARSING ====================
bool parseHomeCommand(const char* action) {
  // Format: HOME(x,y,z,t,g)
  const char* openParen = strchr(action, '(');
  const char* closeParen = strchr(action, ')');

  if (!openParen || !closeParen) {
    Serial.println(F("ERROR: Invalid HOME command format"));
    return false;
  }

  // Extract parameters
  int parsed = sscanf(openParen + 1, "%d,%d,%d,%d,%d",
                      &homeCmd.x, &homeCmd.y, &homeCmd.z, &homeCmd.t, &homeCmd.g);

  if (parsed != 5) {
    Serial.println(F("ERROR: HOME command requires 5 parameters"));
    return false;
  }

  Serial.print(F("Parsed HOME command: X="));
  Serial.print(homeCmd.x);
  Serial.print(F(", Y="));
  Serial.print(homeCmd.y);
  Serial.print(F(", Z="));
  Serial.print(homeCmd.z);
  Serial.print(F(", T="));
  Serial.print(homeCmd.t);
  Serial.print(F(", G="));
  Serial.println(homeCmd.g);

  return true;
}


bool parseGladCommand(const char* action) {
  // Format: GLAD(xn,yn,zn,tn,dp,gp,za,zb,xa)
  const char* openParen = strchr(action, '(');
  const char* closeParen = strchr(action, ')');

  if (!openParen || !closeParen) {
    Serial.println(F("ERROR: Invalid GLAD command format"));
    return false;
  }

  // Extract parameters - PERBAIKAN: Baca 10 parameter sesuai struct
  int parsed = sscanf(openParen + 1, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                      &gladCmd.xn, &gladCmd.yn, &gladCmd.zn, &gladCmd.tn,
                      &gladCmd.dp, &gladCmd.gp, &gladCmd.za, &gladCmd.zb,
                      &gladCmd.xa, &gladCmd.ta);

  // PERBAIKAN: Cek 10 parameter (sesuai dengan jumlah %d di sscanf)
  if (parsed != 10) {
    Serial.print(F("ERROR: GLAD command requires 12 parameters, got "));
    Serial.println(parsed);
    return false;
  }

  Serial.print(F("Parsed GLAD command: Xn="));
  Serial.print(gladCmd.xn);
  Serial.print(F(", Yn="));
  Serial.print(gladCmd.yn);
  Serial.print(F(", Zn="));
  Serial.print(gladCmd.zn);
  Serial.print(F(", Tn="));
  Serial.print(gladCmd.tn);
  Serial.print(F(", Dp="));
  Serial.print(gladCmd.dp);
  Serial.print(F(", Gp="));
  Serial.print(gladCmd.gp);
  Serial.print(F(", Za="));
  Serial.print(gladCmd.za);
  Serial.print(F(", Zb="));
  Serial.print(gladCmd.zb);
  Serial.print(F(", Xa="));
  Serial.print(gladCmd.xa);

  return true;
}


// ==================== COMMAND SEQUENCES ====================
void startHomeSequence() {
  Serial.println(F("Starting HOME sequence"));
  currentSequence = SEQ_HOME;
  homeCmd.step = 1;


  //  if (isMotorReady()) {
  //    Serial.println(F("Motor ready detected - adding safety delay"));
  //    waitingForMotorReady = true;
  //    motorReadyWaitStart = millis();
  //  } else {
  executeHomeStep();
  //  }
}


void startGladSequence() {
  Serial.println(F("Starting GLAD sequence"));
  currentSequence = SEQ_GLAD;
  gladCmd.step = 1;

  if (isMotorReady()) {
    Serial.println(F("Motor ready detected - adding safety delay"));
    waitingForMotorReady = true;
    motorReadyWaitStart = millis();
  } else {
    executeGladStep();
  }
}


void updateCommandSequence() {
  if (currentSequence == SEQ_NONE) {
    return;
  }


  if (waitingForMotorReady) {
    if (millis() - motorReadyWaitStart >= MOTOR_READY_WAIT_MS) {
      Serial.println(F("Safety delay completed - starting GLAD sequence"));
      waitingForMotorReady = false;
      executeGladStep();
    }
    return;
  }


  // Check if motor is ready for next step dengan stabilization delay
  bool motorReady = isMotorReady();
  if (motorReady && !motorWasReady) {
    // Motor baru saja ready, catat waktu
    lastMotorReadyTime = millis();
    motorWasReady = true;
    return;
  }

  if (motorReady && motorWasReady) {
    // Motor sudah ready, cek apakah sudah lewat stabilization time
    if (millis() - lastMotorReadyTime >= MOTOR_STABILIZE_MS) {
      if (currentSequence == SEQ_HOME) {
        executeHomeStep();
      } else if (currentSequence == SEQ_GLAD) {
        executeGladStep();
      }
      motorWasReady = false;  // Reset flag
    }
  }

  if (!motorReady) {
    motorWasReady = false;
  }
}


void executeHomeStep() {
  char command[64];  // Reduced buffer size

  switch (homeCmd.step) {
    case 1:
      // First command: Xx,Yy,Tt,Gg (Z parameter removed)
      snprintf_P(command, sizeof(command), PSTR("X%d,Y%d,T%d,G%d"),
                 homeCmd.x, homeCmd.y, homeCmd.t, homeCmd.g);
      sendMotorCommand(command);
      homeCmd.step = 2;
      Serial.println(F("HOME Step 1: Moving to XY position"));
      break;

    case 2:
      // Second command: Zz
      snprintf_P(command, sizeof(command), PSTR("Z%d"), homeCmd.z);
      sendMotorCommand(command);
      homeCmd.step = 3;
      Serial.println(F("HOME Step 2: Final Z position"));
      break;

    case 3:
      // Sequence complete
      Serial.println(F("HOME sequence completed"));
      currentSequence = SEQ_NONE;
      homeCmd.step = 0;
      // Cek apakah STOP sudah ditekan
      if (stopRequested) {
        Serial.println(F("STOP was requested - sequence completed, will proceed to FINISHING"));
        // Biarkan handleRunningState() yang menangani transisi ke FINISHING
      } else {
        // Normal completion: RUNNING -> READY transition
        if (currentState == STATE_READY) {
          enterRunningState();
        }
      }


      break;
  }
}


// Helper function to safely create and send motor commands
void sendSafeMotorCommand(const char* format, ...) {
  char command[64];

  // Clear buffer first
  memset(command, 0, sizeof(command));

  // Build command using variadic arguments
  va_list args;
  va_start(args, format);
  int result = vsnprintf_P(command, sizeof(command), format, args);
  va_end(args);

  // Check for overflow
  if (result >= 0 && result < sizeof(command)) {
    sendMotorCommand(command);
    Serial.print(F("Safe command sent: "));
    Serial.println(command);
  } else {
    Serial.println(F("ERROR: Command buffer overflow prevented"));
  }
}


void executeGladStep() {
  Serial.print(F("=== executeGladStep() called - Current step: "));
  Serial.print(gladCmd.step);
  Serial.println(F(" ==="));


  switch (gladCmd.step) {
    case 1:
      // First command: Zzb
      sendSafeMotorCommand(PSTR("Z%d"), gladCmd.zb);
      gladCmd.step = 2;
      Serial.print(F("GLAD Step 1: Move Z to "));
      Serial.println(gladCmd.zb);
      break;

    case 2:
      // Second command: Ggp
      sendSafeMotorCommand(PSTR("G%d"), gladCmd.gp);
      gladCmd.step = 3;
      Serial.println(F("GLAD Step 2: Set G parameter"));
      break;

    case 3:
      // Third command: Zz (z = zn - za)
      {
        int z_position = gladCmd.zn - gladCmd.za;
        sendSafeMotorCommand(PSTR("Z%d"), z_position);
        gladCmd.step = 4;
        Serial.print(F("GLAD Step 3: Move Z to "));
        Serial.println(z_position);
      }
      break;

    case 4:
      // Try multi-parameter command with automatic overflow protection
      sendSafeMotorCommand(PSTR("X%d,Y%d,T%d"), gladCmd.xn, gladCmd.yn, gladCmd.tn);
      gladCmd.step = 5;
      Serial.println(F("GLAD Step 4: Move to XY position - COMPLETED"));
      break;

    case 5:
      // Fifth command: Zzn
      sendSafeMotorCommand(PSTR("Z%d"), gladCmd.zn);
      gladCmd.step = 6;
      Serial.println(F("GLAD Step 5: Move to final Z position"));
      break;

    case 6:
      // Sixth command: Gdp
      sendSafeMotorCommand(PSTR("G%d"), gladCmd.dp);
      gladCmd.step = 7;
      Serial.println(F("GLAD Step 6: Set G parameter"));
      break;

    case 7:
      // Seventh command: Zz (same as step 3)
      {
        int z_position_final = gladCmd.zn - gladCmd.za;
        sendSafeMotorCommand(PSTR("Z%d"), z_position_final);
        gladCmd.step = 8;
        Serial.print(F("GLAD Step 7: Z move to "));
        Serial.println(z_position_final);
      }
      break;


    case 8:
      // Eigth command: Ta
      sendSafeMotorCommand(PSTR("X%d,T%d"), gladCmd.xa, gladCmd.ta);
      gladCmd.step = 9;
      Serial.println(F("GLAD Step 8: Standby XT position before Homing "));
      break;


      //    case 9:
      //      // Ninth command: Xa
      //      sendSafeMotorCommand(PSTR("X%d"), gladCmd.xa);
      //      gladCmd.step = 10;
      //      Serial.println(F("GLAD Step 9: Standby X position before Homing "));
      //      break;


    case 9:
      // Sequence complete
      Serial.println(F("GLAD sequence completed"));
      currentSequence = SEQ_NONE;
      gladCmd.step = 0;

      // Cek apakah STOP sudah ditekan
      if (stopRequested) {
        Serial.println(F("STOP requested - going directly to ZEROING"));
        stopRequested = false;
        enterZeroingState();  // Langsung ke ZEROING (HIGH)
      } else {
        // Normal completion: RUNNING -> READY transition
        if (currentState == STATE_RUNNING) {
          enterReadyState();
        }
      }
      break;

    default:
      Serial.print(F("ERROR: Unknown GLAD step: "));
      Serial.println(gladCmd.step);
      currentSequence = SEQ_NONE;
      gladCmd.step = 0;
      break;
  }
}


bool waitForMotorResponse(int delay1) {
  unsigned long startTime = millis();

  while (millis() - startTime < delay1) {
    if (digitalRead(MOTOR_DONE_PIN) == LOW) {
      Serial.println(F("Motor response detected"));
      return true;  // Success
    }
    delay(10);  // Small delay to prevent busy waiting
  }

  Serial.println(F("ERROR: Motor timeout - no response detected"));
  return false;
}


void handleMotorTimeout() {
  Serial.println(F("CRITICAL: Motor driver timeout - entering error state"));

  // Error indication: Yellow LED + Buzzer
  while (true) {
    setLedState(false, true, false);
    setBuzzerState(true);
    delay(500);
    setLedState(false, false, false);
    setBuzzerState(false);
    delay(500);
  }
}


void processUSBCommands() {
  // Hanya aktif dalam state SLEEPING
  if (currentState != STATE_SLEEPING) {
    return;
  }

  static char usbCommandBuffer[64];
  static byte usbCommandIndex = 0;

  if (Serial.available()) {
    while (Serial.available() && usbCommandIndex < sizeof(usbCommandBuffer) - 1) {
      char receivedChar = Serial.read();

      if (receivedChar == '\n' || receivedChar == '\r') {
        usbCommandBuffer[usbCommandIndex] = '\0';

        // Langsung kirim ke motor tanpa parsing
        if (usbCommandIndex > 0) {
          Serial.print(F("USB Test Command: "));
          Serial.println(usbCommandBuffer);
          sendMotorCommand(usbCommandBuffer);
        }

        usbCommandIndex = 0;
      } else {
        usbCommandBuffer[usbCommandIndex++] = receivedChar;
      }
    }
  }
}
