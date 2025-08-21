#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>


// Pin definitions
const int SENSOR1_PIN = A4;
const int SENSOR2_PIN = A5;
const int SENSOR3_PIN = 2;
const int ARM1_PIN = 7;       // Changed from 6 to 7
const int ARM2_PIN = 8;       // Changed from 7 to 8
const int CONVEYOR_PIN = 13;  // D13 - Conveyor control (active LOW)


// RS485 pins
const int RS485_RO = 10;
const int RS485_DI = 11;
// RE/DE connected to VCC (always enabled for transmission)


// DIP Switch pins for ARM1 layer reading
const int ARM1_DIP_1 = 5;  // D5
const int ARM1_DIP_2 = 6;  // D6
const int ARM1_DIP_4 = 3;  // D3
const int ARM1_DIP_8 = 4;  // D4


// DIP Switch pins for ARM2 layer reading
const int ARM2_DIP_1 = A0;
const int ARM2_DIP_2 = A1;
const int ARM2_DIP_4 = A3;  // Note: corrected order
const int ARM2_DIP_8 = A2;  // Note: corrected order


// EEPROM Configuration
const int EEPROM_START_ADDR = 0;
const int EEPROM_MAGIC = 0xABCD;  // Magic number to check if EEPROM is initialized
const int EEPROM_VERSION = 1;


// Create SoftwareSerial object for RS485
SoftwareSerial rs485(RS485_RO, RS485_DI);


// Definisi State untuk ARM
enum ArmState {
  ARM_IDLE,               // ARM dalam keadaan idle, siap menerima command
  ARM_MOVING_TO_CENTER,   // ARM sedang bergerak ke center (HOME command)
  ARM_IN_CENTER,          // ARM sudah di center, menunggu product
  ARM_PICKING,            // ARM sedang melakukan pickup (GLAD command)
  ARM_EXECUTING_SPECIAL,  // <- State baru untuk CAL dan PARK
  ARM_ERROR               // ARM dalam keadaan error
};


enum SpecialCommand {
  SPECIAL_NONE,
  SPECIAL_PARK,
  SPECIAL_CALI
};


// Struktur ARM Data yang sudah dimodifikasi dengan State Machine
struct ArmDataStateMachine {
  int current_pos;
  int total_commands;
  bool is_busy;
  bool is_initialized;
  byte arm_id;
  int start_layer;

  // State Machine Variables
  ArmState state;
  ArmState previous_state;
  unsigned long state_enter_time;
  unsigned long state_duration;

  // Timeout settings (dalam milliseconds)
  static const unsigned long MOVE_TIMEOUT = 15000;  // 10 detik max untuk movement
  static const unsigned long PICK_TIMEOUT = 15000;  // 15 detik max untuk picking

  // Debugging
  bool debug_state_changes;


  // Retry mechanism variables
  String last_command_sent;
  unsigned long command_sent_time;
  int retry_count;
  bool waiting_for_busy_response;

  // Retry settings
  static const unsigned long BUSY_RESPONSE_TIMEOUT = 500;  // 0.5 detik timeout
  static const int MAX_RETRY_COUNT = 7;                    // maksimal 7x retry
  static const unsigned long RETRY_DELAY = 200;            // delay 200ms antar retry


  // Special command handling
  SpecialCommand pending_special_command;
  bool need_special_command;


  // ADD THESE NEW FIELDS:
  bool was_busy_after_command;
  unsigned long min_wait_time;
  int busy_stable_count;
  static const unsigned long MIN_PICKING_TIME = 300;
  static const int BUSY_STABLE_THRESHOLD = 3;
};


//DELAY after ARM leave center
static const int LEAVE_CENTER_DELAY = 1;


// Replace global arm1 dan arm2 dengan versi state machine
ArmDataStateMachine arm1_sm, arm2_sm;


// System variables
bool sensor1_state = false;
bool sensor2_state = false;
bool sensor3_state = false;
bool arm1_response = false;
bool arm2_response = false;
byte arm_in_center = 0;  // 0=none, 1=ARM1, 2=ARM2
bool last_arm_sent = false;
bool sensor3_prev_state = false;


// Conveyor control variables
bool conveyor_active = true;  // true = ON, false = OFF
unsigned long conveyor_off_time = 0;
const unsigned long CONVEYOR_OFF_DURATION = 3000;  // 3 seconds


// Timing variables
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_READ_INTERVAL = 30;


// Global variables for USB commands
bool debug_mode = false;
bool monitor_mode = false;
unsigned long last_monitor_update = 0;
const unsigned long MONITOR_INTERVAL = 1000;  // 1 second


// Buffer for reading USB serial commands
char usbBuffer[32];
byte usbBufferIndex = 0;

// Force run flags untuk bypass busy check
bool force_run_arm1 = false;
bool force_run_arm2 = false;


// EEPROM Header structure
struct EEPROMHeader {
  int magic;
  int version;
  int checksum;
};


// Configuration Parameters - Stored in SRAM for runtime access
struct Parameters {
  // Global Parameters (13)
  int x;    //Home X
  int y1;   //Home Y pick 2
  int y2;   //Home Y pick 1
  int z;    //Home Z before product ready
  int t;    //Home T
  int g;    //Home G
  int gp;   //Grip
  int dp;   //Drop
  int za;   //Relative lift axis when pick product
  int zb;   //Home Z after product ready
  int H;    // Tinggi Product
  int Ly;   // Jumlah layer
  int T90;  //
  int Z1;   //Z layer 1

  // Base values for first positions (7)
  int XO1;  //X1 Odd
  int YO1;  //Y1 Odd
  int XE1;  //X1 Even
  int YE1;  //Y1 Even
  int XO2;  //X2 Odd
  int YO2;  //Y2 Odd
  int XE2;  //X2 Even
  int YE2;  //Y2 Even
  int XO3;  //X3 Odd
  int YO3;  //Y3 Odd
  int XE3;  //X3 Even
  int YE3;  //Y3 Even
  int XO4;  //X4 Odd
  int YO4;  //Y4 Odd
  int XE4;  //X4 Even
  int YE4;  //Y4 Even
  int XO5;  //X5 Odd
  int YO5;  //Y5 Odd
  int XE5;  //X5 Even
  int YE5;  //Y5 Even
  int XO6;  //X6 Odd
  int YO6;  //Y6 Odd
  int XE6;  //X6 Even
  int YE6;  //Y6 Even
  int XO7;  //X7 Odd
  int YO7;  //Y7 Odd
  int XE7;  //X7 Even
  int YE7;  //Y7 Even
  int XO8;  //X8 Odd
  int YO8;  //Y8 Odd
  int XE8;  //X8 Even
  int YE8;  //Y8 Even


  // ARM1 Offset (LEFT) (5)
  int xL;
  int yL;
  int zL;
  int tL;
  int gL;

  // ARM2 Offset (RIGHT) (5)
  int xR;
  int yR;
  int zR;
  int tR;
  int gR;

  // Y pattern for taskings (1 = y1, 2 = y2) (1)
  byte y_pattern[8];
};


Parameters params;


const int MULTIPLIER = 3;


byte serialIndex = 0;


// String constants in PROGMEM
const char msg_system_start[] PROGMEM = "=== ARM Control System Started ===";
const char msg_system_ready[] PROGMEM = "System ready with generated commands";
const char msg_dip_reading[] PROGMEM = "Reading DIP switch layer positions...";


// Buffer for building commands (reused)
char commandBuffer[150];


// Function to read DIP switch value for ARM1
int readArm1DipSwitch() {
  int value = 0;
  if (!digitalRead(ARM1_DIP_1)) value += 1;
  if (!digitalRead(ARM1_DIP_2)) value += 2;
  if (!digitalRead(ARM1_DIP_4)) value += 4;
  if (!digitalRead(ARM1_DIP_8)) value += 8;
  return value;
}


// Function to read DIP switch value for ARM2
int readArm2DipSwitch() {
  int value = 0;
  if (!digitalRead(ARM2_DIP_1)) value += 1;
  if (!digitalRead(ARM2_DIP_2)) value += 2;
  if (!digitalRead(ARM2_DIP_4)) value += 4;
  if (!digitalRead(ARM2_DIP_8)) value += 8;
  return value;
}


// Function to calculate Z values based on rules
int calculateZ(int layer) {
  if (layer == 0) return params.Z1;
  return params.Z1 - layer * params.H;
}


// Function to calculate XO values based on rules
int calculateXO(int task) {
  switch (task) {
    case 0: return params.XO1;  // XO1
    case 1: return params.XO2;  // XO2
    case 2: return params.XO3;  // XO3
    case 3: return params.XO4;  // XO4
    case 4: return params.XO5;  // XO5
    case 5: return params.XO6;  // XO6
    case 6: return params.XO7;  // XO7
    case 7: return params.XO8;  // XO8
    default: return params.XO1;
  }
}


// Function to calculate YO values based on rules
int calculateYO(int task) {
  switch (task) {
    case 0: return params.YO1;  // YO1
    case 1: return params.YO2;  // YO2
    case 2: return params.YO3;  // YO3
    case 3: return params.YO4;  // YO4
    case 4: return params.YO5;  // YO5
    case 5: return params.YO6;  // YO6
    case 6: return params.YO7;  // YO7
    case 7: return params.YO8;  // YO8
    default: return params.YO1;
  }
}


// Function to calculate TO values based on rules
int calculateTO(int task) {
  if (task >= 0 && task <= 3) {
    return params.T90;  // TO1 to TO4
  } else if (task >= 4 && task <= 7) {
    return params.t;  // TO5 to TO8
  }
}


// Function to calculate XE values based on rules
int calculateXE(int task) {
  switch (task) {
    case 0: return params.XE1;  // XE1
    case 1: return params.XE2;  // XE2
    case 2: return params.XE3;  // XE3
    case 3: return params.XE4;  // XE4
    case 4: return params.XE5;  // XE5
    case 5: return params.XE6;  // XE6
    case 6: return params.XE7;  // XE7
    case 7: return params.XE8;  // XE8
    default: return params.XE1;
  }
}


// Function to calculate YE values based on rules
int calculateYE(int task) {
  switch (task) {
    case 0: return params.YE1;  // YE1
    case 1: return params.YE2;  // YE2
    case 2: return params.YE3;  // YE3
    case 3: return params.YE4;  // YE4
    case 4: return params.YE5;  // YE5
    case 5: return params.YE6;  // YE6
    case 6: return params.YE7;  // YE7
    case 7: return params.YE8;  // YE8
    default: return params.YE1;
  }
}


// Function to calculate TE values based on rules
int calculateTE(int task) {
  if (task >= 0 && task <= 3) {
    return params.T90;  // TE1 to TE4
  } else if (task >= 4 && task <= 7) {
    return params.t;  // TE5 to TE8
  }
}


// Calculate checksum for parameters
int calculateChecksum(const Parameters& params) {
  int checksum = 0;
  const byte* ptr = (const byte*)&params;
  for (int i = 0; i < sizeof(Parameters); i++) {
    checksum += ptr[i];
  }
  return checksum;
}


// Save parameters to EEPROM
void saveParametersToEEPROM() {
  EEPROMHeader header;
  header.magic = EEPROM_MAGIC;
  header.version = EEPROM_VERSION;
  header.checksum = calculateChecksum(params);

  // Write header
  EEPROM.put(EEPROM_START_ADDR, header);

  // Write parameters
  EEPROM.put(EEPROM_START_ADDR + sizeof(EEPROMHeader), params);

  Serial.println(F("Parameters saved to EEPROM"));
}


// Load parameters from EEPROM
bool loadParametersFromEEPROM() {
  EEPROMHeader header;

  // Read header
  EEPROM.get(EEPROM_START_ADDR, header);

  // Check magic number and version
  if (header.magic != EEPROM_MAGIC || header.version != EEPROM_VERSION) {
    Serial.println(F("EEPROM not initialized or version mismatch"));
    return false;
  }

  // Read parameters
  Parameters tempParams;
  EEPROM.get(EEPROM_START_ADDR + sizeof(EEPROMHeader), tempParams);

  // Verify checksum
  if (calculateChecksum(tempParams) != header.checksum) {
    Serial.println(F("EEPROM checksum mismatch"));
    return false;
  }

  // Copy to global params
  params = tempParams;
  Serial.println(F("Parameters loaded from EEPROM"));
  return true;
}


// ===================================================================
// STATE MACHINE CORE FUNCTIONS
// ===================================================================


// Fungsi untuk mengubah state ARM
void changeArmState(ArmDataStateMachine* arm, ArmState newState) {
  if (arm->state != newState) {
    arm->previous_state = arm->state;
    arm->state = newState;
    arm->state_enter_time = millis();


    if (newState == ARM_PICKING) {
      arm->was_busy_after_command = false;
      arm->min_wait_time = millis() + arm->MIN_PICKING_TIME;
      arm->busy_stable_count = 0;
    }

    // Debug print state changes
    if (arm->debug_state_changes || debug_mode) {
      Serial.print(F("ARM"));
      Serial.print(arm->arm_id);
      Serial.print(F(" State: "));
      Serial.print(getStateString(arm->previous_state));
      Serial.print(F(" -> "));
      Serial.println(getStateString(newState));
    }
  }
}


// Helper function untuk convert state ke string (untuk debugging)
String getStateString(ArmState state) {
  switch (state) {
    case ARM_IDLE: return F("IDLE");
    case ARM_MOVING_TO_CENTER: return F("MOVING_TO_CENTER");
    case ARM_IN_CENTER: return F("IN_CENTER");
    case ARM_PICKING: return F("PICKING");
    case ARM_ERROR: return F("ERROR");
    case ARM_EXECUTING_SPECIAL: return F("CALIBRATION");
    default: return F("UNKNOWN");
  }
}


// Helper function untuk debounced busy check
bool isArmTrulyNotBusy(ArmDataStateMachine* arm, bool hardware_busy) {
  if (hardware_busy) {
    arm->busy_stable_count = 0;
    if (!arm->was_busy_after_command) {
      arm->was_busy_after_command = true;
      if (debug_mode) {
        Serial.print(F("ARM"));
        Serial.print(arm->arm_id);
        Serial.println(F(" detected as busy after command"));
      }
    }
    return false;
  } else {
    arm->busy_stable_count++;
    return (arm->busy_stable_count >= arm->BUSY_STABLE_THRESHOLD);
  }
}


// Fungsi untuk mengecek timeout
bool isStateTimeout(ArmDataStateMachine* arm, unsigned long timeout) {
  return (millis() - arm->state_enter_time) > timeout;
}


// ===================================================================
// STATE HANDLERS - Setiap state memiliki handler terpisah
// ===================================================================


void handleIdleState(ArmDataStateMachine* arm) {
  // ARM idle, siap menerima command
  // Transisi ke MOVING_TO_CENTER akan dilakukan dari handleSystemLogic()
}


void handleMovingToCenterState(ArmDataStateMachine* arm) {
  // Cek timeout
  if (isStateTimeout(arm, arm->MOVE_TIMEOUT)) {
    Serial.print(F("ARM"));
    Serial.print(arm->arm_id);
    Serial.println(F(" MOVE TIMEOUT - switching to ERROR"));
    changeArmState(arm, ARM_ERROR);
    return;
  }

  // Cek apakah ARM sudah sampai di center (sensor3 LOW dan ARM tidak busy)
  if (!sensor3_state && !arm->is_busy && arm_in_center == arm->arm_id) {
    changeArmState(arm, ARM_IN_CENTER);
  }
}


void handleInCenterState(ArmDataStateMachine* arm) {
  // ARM sudah di center, menunggu product
  // Transisi ke PICKING akan dilakukan dari handleProductPickup()

  // Safety check - jika ARM tidak lagi di center secara fisik
  if (sensor3_state) {
    Serial.print(F("ARM"));
    Serial.print(arm->arm_id);
    Serial.println(F(" not in center anymore - back to IDLE"));
    arm_in_center = 0;
    changeArmState(arm, ARM_IDLE);
  }
}


void handlePickingState(ArmDataStateMachine* arm) {
  // Cek timeout
  //if (isStateTimeout(arm, arm->PICK_TIMEOUT)) {
  //  Serial.print(F("ARM"));
  //  Serial.print(arm->arm_id);
  //  Serial.println(F(" PICK TIMEOUT - switching to ERROR"));
  //  changeArmState(arm, ARM_ERROR);
  //  return;
  //}

  // Check minimum wait time first
  if (millis() < arm->min_wait_time) {
    return;  // Still in minimum wait period
  }

  // Check if ARM was busy and now truly not busy
  bool hardware_busy = digitalRead((arm->arm_id == 1) ? ARM1_PIN : ARM2_PIN);
  bool truly_not_busy = isArmTrulyNotBusy(arm, hardware_busy);

  if (arm->was_busy_after_command && truly_not_busy) {
    // ARM has completed the task
    int current_layer = (arm->current_pos - 1) / 16;
    int position_in_layer = (arm->current_pos - 1) % 16;
    bool is_even_layer_number = ((current_layer + 1) % 2 == 0);
    bool layer_complete = (position_in_layer == 15);


    if (/*is_even_layer_number && layer_complete*/ (arm->current_pos % 64 == 0) && arm->current_pos < arm->total_commands) {
      arm->pending_special_command = SPECIAL_CALI;
      arm->need_special_command = true;
      changeArmState(arm, ARM_EXECUTING_SPECIAL);
    } else if (arm->current_pos >= arm->total_commands) {
      arm->pending_special_command = SPECIAL_PARK;
      arm->need_special_command = true;
      changeArmState(arm, ARM_EXECUTING_SPECIAL);
    } else {
      changeArmState(arm, ARM_IDLE);
    }
    //arm_in_center = 0;
  }
}


void handleExecutingSpecialState(ArmDataStateMachine* arm) {
  bool hardware_busy = digitalRead((arm->arm_id == 1) ? ARM1_PIN : ARM2_PIN);
  unsigned long elapsed_time = millis() - arm->state_enter_time;

  // Untuk PARK command - lebih strict timing
  if (arm->pending_special_command == SPECIAL_PARK) {
    if (elapsed_time >= 500 && !hardware_busy) {
      Serial.print(F("ARM"));
      Serial.print(arm->arm_id);
      Serial.println(F(" PARK completed"));
      changeArmState(arm, ARM_IDLE);
      return;
    }
  }

  // Untuk CALI command - lebih fleksibel
  else if (arm->pending_special_command == SPECIAL_CALI) {
    if (elapsed_time >= 500 && !hardware_busy) {
      Serial.print(F("ARM"));
      Serial.print(arm->arm_id);
      Serial.println(F(" CALIBRATION completed"));
      changeArmState(arm, ARM_IDLE);
      return;
    }
  }

  // Default case - jika pending_special_command sudah NONE
  else {
    if (elapsed_time >= 1500 && !hardware_busy) {
      changeArmState(arm, ARM_IDLE);
      return;
    }
  }
}


void handleErrorState(ArmDataStateMachine* arm) {
  // ARM dalam keadaan error
  // Bisa ditambahkan logic untuk recovery atau manual reset
  Serial.print(F("ARM"));
  Serial.print(arm->arm_id);
  Serial.println(F(" in ERROR state - manual intervention required"));

  // Auto recovery setelah timeout tertentu (optional)
  if (isStateTimeout(arm, 30000)) {  // 30 detik
    Serial.print(F("ARM"));
    Serial.print(arm->arm_id);
    Serial.println(F(" auto-recovery from ERROR"));
    changeArmState(arm, ARM_IDLE);
  }
}


// ===================================================================
// MAIN STATE MACHINE UPDATE FUNCTION
// ===================================================================


void updateArmStateMachine(ArmDataStateMachine* arm) {
  bool hardware_busy = digitalRead((arm->arm_id == 1) ? ARM1_PIN : ARM2_PIN);

  // Update debounced busy status for non-picking states
  if (arm->state != ARM_PICKING) {
    arm->is_busy = hardware_busy;
    if (hardware_busy && !arm->was_busy_after_command) {
      arm->was_busy_after_command = true;
    }
  } else {
    // For picking state, use the helper function
    arm->is_busy = !isArmTrulyNotBusy(arm, hardware_busy);
  }


  // Execute state handler
  switch (arm->state) {
    case ARM_IDLE:
      handleIdleState(arm);
      break;

    case ARM_MOVING_TO_CENTER:
      handleMovingToCenterState(arm);
      break;


    case ARM_EXECUTING_SPECIAL:
      handleExecutingSpecialState(arm);
      break;

    case ARM_IN_CENTER:
      handleInCenterState(arm);
      break;

    case ARM_PICKING:
      handlePickingState(arm);
      break;

    case ARM_ERROR:
      handleErrorState(arm);
      break;
  }
}


// ===================================================================
// MODIFIED SYSTEM FUNCTIONS FOR STATE MACHINE
// ===================================================================


// Modified initializeArmData untuk state machine
void initializeArmDataStateMachine() {
  Serial.println(F("Initializing ARM State Machines..."));


  // ARM1 initialization
  arm1_sm.total_commands = params.Ly * 8 * 2;
  arm1_sm.current_pos = arm1_sm.start_layer * 16;
  if (arm1_sm.current_pos >= arm1_sm.total_commands)
    arm1_sm.current_pos = arm1_sm.total_commands;

  arm1_sm.is_busy = false;
  arm1_sm.is_initialized = true;
  arm1_sm.arm_id = 1;
  arm1_sm.state = ARM_IDLE;
  arm1_sm.previous_state = ARM_IDLE;
  arm1_sm.state_enter_time = millis();
  arm1_sm.debug_state_changes = true;


  arm1_sm.waiting_for_busy_response = false;
  arm1_sm.retry_count = 0;
  arm1_sm.last_command_sent = "";


  arm1_sm.pending_special_command = SPECIAL_NONE;
  arm1_sm.need_special_command = false;


  arm1_sm.was_busy_after_command = false;
  arm1_sm.busy_stable_count = 0;


  // ARM2 initialization
  arm2_sm.total_commands = params.Ly * 8 * 2;
  arm2_sm.current_pos = arm2_sm.start_layer * 16;
  if (arm2_sm.current_pos >= arm2_sm.total_commands)
    arm2_sm.current_pos = arm2_sm.total_commands;

  arm2_sm.is_busy = false;
  arm2_sm.is_initialized = true;
  arm2_sm.arm_id = 2;
  arm2_sm.state = ARM_IDLE;
  arm2_sm.previous_state = ARM_IDLE;
  arm2_sm.state_enter_time = millis();
  arm2_sm.debug_state_changes = true;


  arm2_sm.waiting_for_busy_response = false;
  arm2_sm.retry_count = 0;
  arm2_sm.last_command_sent = "";


  arm2_sm.pending_special_command = SPECIAL_NONE;
  arm2_sm.need_special_command = false;


  arm2_sm.was_busy_after_command = false;
  arm2_sm.busy_stable_count = 0;


  Serial.print(F("ARM1 SM starts at position: "));
  Serial.print(arm1_sm.current_pos);
  Serial.print(F(" (Layer "));
  Serial.print(arm1_sm.start_layer + 1);
  Serial.println(F(")"));

  Serial.print(F("ARM2 SM starts at position: "));
  Serial.print(arm2_sm.current_pos);
  Serial.print(F(" (Layer "));
  Serial.print(arm2_sm.start_layer + 1);
  Serial.println(F(")"));

  Serial.println(F("ARM State Machines initialized"));
}


// Modified getNextCommand untuk state machine
String getNextCommandStateMachine(ArmDataStateMachine* arm) {
  // Cek apakah ada special command yang pending
  if (arm->need_special_command) {
    arm->need_special_command = false;

    if (arm->pending_special_command == SPECIAL_PARK) {
      arm->pending_special_command = SPECIAL_NONE;
      return "P";  // Return tanpa prefix ARM
    } else if (arm->pending_special_command == SPECIAL_CALI) {
      arm->pending_special_command = SPECIAL_NONE;
      return "C";  // Return tanpa prefix ARM
    }
  }


  // Cek apakah sudah selesai semua command
  if (arm->current_pos >= arm->total_commands) {
    // AUTO-RESET: Kembali ke layer 0 (posisi 0)
    Serial.print(F("ARM"));
    Serial.print(arm->arm_id);
    Serial.println(F(" completed all commands - RESETTING to Layer 0"));


    // Reset ke posisi 0
    arm->current_pos = 0;


    Serial.print(F("ARM"));
    Serial.print(arm->arm_id);
    Serial.println(F(" reset to position: 0 (Layer 1)"));
  }

  String command = generateCommand(arm->arm_id, arm->current_pos);
  arm->current_pos++;
  return command;
}


// Modified handleProductPickup untuk state machine
void handleProductPickupStateMachine() {
  // Safety checks
  if (arm_in_center == 0 || sensor3_state) {
    return;
  }

  ArmDataStateMachine* currentArm = (arm_in_center == 1) ? &arm1_sm : &arm2_sm;

  // Hanya proses jika ARM dalam state IN_CENTER dan tidak busy
  if (currentArm->state != ARM_IN_CENTER || currentArm->is_busy) {
    return;
  }

  String gladCommand = getNextCommandStateMachine(currentArm);

  if (gladCommand.length() == 0) {
    Serial.print(F("No more commands for ARM"));
    Serial.println(arm_in_center);
    return;
  }

  String armPrefix = (arm_in_center == 1) ? "L" : "R";
  String fullCommand = armPrefix + "#" + gladCommand;
  sendRS485CommandWithRetry(currentArm, fullCommand);
  turnOffConveyor();

  Serial.print(F("Sent GLAD: "));
  Serial.println(fullCommand);

  changeArmState(currentArm, ARM_PICKING);
}


// Modified sendArmToCenterSmart untuk state machine
void sendArmToCenterSmartStateMachine() {
  // PRIORITAS 1: Cek ARM yang butuh special command (PARK/CALI) terlebih dahulu
  ArmDataStateMachine* specialArm = nullptr;
  byte specialArmId = 0;


  // ADD SAFETY CHECK:
  unsigned long current_time = millis();

  // Cari ARM yang butuh special command dan dalam state IDLE
  if (arm1_sm.state == ARM_IDLE && arm1_sm.need_special_command && !arm1_sm.is_busy) {
    if (current_time >= arm1_sm.min_wait_time) {
      specialArm = &arm1_sm;
      specialArmId = 1;
    }
  } else if (arm2_sm.state == ARM_IDLE && arm2_sm.need_special_command && !arm2_sm.is_busy) {
    if (current_time >= arm2_sm.min_wait_time) {
      specialArm = &arm2_sm;
      specialArmId = 2;
    }
  }

  // Jika ada ARM yang butuh special command, proses dulu
  if (specialArm != nullptr) {
    String command;
    String actionName;

    if (specialArm->pending_special_command == SPECIAL_PARK) {
      command = "P";
      actionName = "PARK";
    } else if (specialArm->pending_special_command == SPECIAL_CALI) {
      command = "C";
      actionName = "CALIBRATION";
    }

    Serial.print(F("ARM"));
    Serial.print(specialArmId);
    Serial.print(F(" executing "));
    Serial.print(actionName);
    Serial.println(F(" command"));

    // Kirim command
    String armPrefix = (specialArmId == 1) ? "L" : "R";
    String fullCommand = armPrefix + "#" + command;
    sendRS485CommandWithRetry(specialArm, fullCommand);
    delay(100);
    Serial.print(F("Sent special command: "));
    Serial.println(fullCommand);


    // Set state ke EXECUTING_SPECIAL
    changeArmState(specialArm, ARM_EXECUTING_SPECIAL);


    // PENTING: Reset flags SETELAH command dikirim
    specialArm->need_special_command = false;
    specialArm->pending_special_command = SPECIAL_NONE;


    arm_in_center = 0;  // Reset segera setelah mengirim special command
    Serial.print(F("arm_in_center reset to 0 after sending "));
    Serial.println(actionName);


    return;  // Exit function setelah mengirim special command
  }

  // PRIORITAS 2: Handle command normal (HOME) jika tidak ada special command
  bool arm1_ready = (arm1_sm.state == ARM_IDLE) && !arm1_sm.is_busy && !arm1_sm.need_special_command;
  bool arm2_ready = (arm2_sm.state == ARM_IDLE) && !arm2_sm.is_busy && !arm2_sm.need_special_command;

  // BYPASS: Force ready jika ada force flag
  if (force_run_arm1) {
    arm1_ready = true;
    Serial.println(F("ARM1 ready BYPASSED by force flag"));
  }
  if (force_run_arm2) {
    arm2_ready = true;
    Serial.println(F("ARM2 ready BYPASSED by force flag"));
  }

  if (!arm1_ready && !arm2_ready) {
    return;  // Tidak ada ARM yang ready untuk command normal
  }

  // Pilih ARM untuk command normal
  byte selectedArm = 0;
  ArmDataStateMachine* currentArm = nullptr;

  if (arm1_ready && arm2_ready) {
    last_arm_sent = !last_arm_sent;
    selectedArm = last_arm_sent ? 2 : 1;
  } else if (arm1_ready) {
    selectedArm = 1;
    last_arm_sent = false;
  } else if (arm2_ready) {
    selectedArm = 2;
    last_arm_sent = true;
  }

  currentArm = (selectedArm == 1) ? &arm1_sm : &arm2_sm;

  // Reset force flag setelah digunakan
  if (selectedArm == 1) force_run_arm1 = false;
  if (selectedArm == 2) force_run_arm2 = false;

  // Generate HOME command
  String command = getNextCommandStateMachine(currentArm);
  if (command.length() == 0) {
    Serial.print(F("No more commands for ARM"));
    Serial.println(selectedArm);
    return;
  }

  // Set ARM as in center dan ubah state
  arm_in_center = selectedArm;
  changeArmState(currentArm, ARM_MOVING_TO_CENTER);

  // Kirim HOME command
  String armPrefix = (selectedArm == 1) ? "L" : "R";
  String fullCommand = armPrefix + "#" + command;
  sendRS485CommandWithRetry(currentArm, fullCommand);

  Serial.print(F("Sent HOME to ARM"));
  Serial.print(selectedArm);
  Serial.print(F(": "));
  Serial.println(fullCommand);
}


// Modified handleSystemLogic untuk state machine
void handleSystemLogicStateMachine() {
  // PRIORITAS 1: Check for product pickup (jika ada ARM di center)
  if (!sensor1_state && !sensor2_state && !sensor3_state && arm_in_center != 0) {
    handleProductPickupStateMachine();
    return;  // Exit setelah handle pickup
  }


  // PRIORITAS 2: Handle special commands yang pending (tidak perlu sensor3 HIGH)
  bool hasSpecialCommand = (arm1_sm.need_special_command && arm1_sm.state == ARM_IDLE) || (arm2_sm.need_special_command && arm2_sm.state == ARM_IDLE);

  if (hasSpecialCommand) {
    sendArmToCenterSmartStateMachine();
    return;  // Exit setelah handle special command
  }


  // ===== TAMBAHAN: PRIORITAS 2.5 - ARM yang baru selesai special command =====
  // Cek ARM yang baru selesai special command dan perlu HOME berikutnya
  if (arm_in_center == 0) {
    bool arm1_needs_home = (arm1_sm.state == ARM_IDLE && !arm1_sm.is_busy && arm1_sm.current_pos < arm1_sm.total_commands);
    bool arm2_needs_home = (arm2_sm.state == ARM_IDLE && !arm2_sm.is_busy && arm2_sm.current_pos < arm2_sm.total_commands);

    if (arm1_needs_home || arm2_needs_home) {
      sendArmToCenterSmartStateMachine();
      return;
    }
  }


  /*
  // Deteksi transisi sensor3: dari ada ARM (LOW) ke tidak ada ARM (HIGH)
  if (sensor3_prev_state == false && sensor3_state == true) {
    Serial.println(F("ARM left center - delay"));
    delay(LEAVE_CENTER_DELAY);
  }


  // Update previous state
  sensor3_prev_state = sensor3_state;
*/


  // PRIORITAS 3: Send ARM to center untuk command normal (hanya jika sensor3 HIGH)
  if (sensor3_state && arm_in_center == 0) {
    sendArmToCenterSmartStateMachine();
  }
}


// Reset parameters to default values
void resetParametersToDefault() {
  // Default values (sesuai dengan yang sudah ada di struct)
  params.x = 1305;
  params.y1 = 130;
  params.y2 = 410;
  params.z = 1280;
  params.t = 80;  //before 104
  params.g = -10;
  params.gp = 90;
  params.dp = 40;
  params.za = 250;
  params.zb = 1320;
  params.T90 = 1600 + params.t;  //before 2080
  params.Z1 = 1325;


  params.H = 100;
  params.Ly = 11;

  params.XO1 = 645;  //Koordinat X1,X3 ganjil
  params.YO1 = 310;  //Koordinat Y1,Y2 ganjil
  params.XO2 = 250;  //Koordinat X2,X4 ganjil. Selisih dengan X1 ganjil adalah panjang produk
  params.YO2 = params.YO1;
  params.XO3 = params.XO1;
  params.YO3 = 65;  //Koordinat Y3,Y4 ganjil. Selisih dengan Y1 ganjil adalah lebar produk
  params.XO4 = params.XO2;
  params.YO4 = params.YO3;
  params.XO5 = 785;  //Koordinat X5 ganjil
  params.YO5 = 735;  //Koordinat Y5,Y6,Y7,Y8 ganjil
  params.XO6 = 545;  //Koordinat X6 ganjil
  params.YO6 = params.YO5;
  params.XO7 = 245;  //Koordinat X7 ganjil
  params.YO7 = params.YO5;
  params.XO8 = 5;  //Koordinat X8 ganjil
  params.YO8 = params.YO5;
  params.XE1 = params.XO1;  //Koordinat X1,X3 genap adalah sama dengan koordinat X1,X3 ganjil
  params.YE1 = 980;         //Koordinat Y1,Y2 genap
  params.XE2 = params.XO2;  //Koordinat X2,X4 genap adalah sama dengan koordinat X2,X4 ganjil
  params.YE2 = params.YE1;
  params.XE3 = params.XO1;
  params.YE3 = 735;  //Koordinat Y3,Y4 genap
  params.XE4 = params.XO2;
  params.YE4 = params.YE3;
  params.XE5 = params.XO5;  //Koordinat X5,X6,X7,X8 genap adalah sama dengan koordinat X5,X6,X7,X8 ganjil
  params.YE5 = 250;         //Koordinat Y5,Y6,Y7,Y8 genap
  params.XE6 = params.XO6;
  params.YE6 = params.YE5;
  params.XE7 = params.XO7;
  params.YE7 = params.YE5;
  params.XE8 = params.XO8;
  params.YE8 = params.YE5;

  params.xL = 0;
  params.yL = 0;
  params.zL = 0;
  params.tL = 0;
  params.gL = 0;

  params.xR = -20;
  params.yR = 0;
  params.zR = 0;
  params.tR = -30;
  params.gR = 5;

  // Y pattern
  params.y_pattern[0] = 2;
  params.y_pattern[1] = 1;
  params.y_pattern[2] = 2;
  params.y_pattern[3] = 1;
  params.y_pattern[4] = 1;
  params.y_pattern[5] = 1;
  params.y_pattern[6] = 1;
  params.y_pattern[7] = 1;

  Serial.println(F("Parameters reset to default values"));
}


void setup() {
  Serial.begin(9600);
  rs485.begin(9600);

  // Configure sensor pins
  pinMode(SENSOR1_PIN, INPUT_PULLUP);
  pinMode(SENSOR2_PIN, INPUT_PULLUP);
  pinMode(SENSOR3_PIN, INPUT_PULLUP);
  pinMode(ARM1_PIN, INPUT_PULLUP);
  pinMode(ARM2_PIN, INPUT_PULLUP);

  // Configure DIP switch pins
  pinMode(ARM1_DIP_1, INPUT_PULLUP);
  pinMode(ARM1_DIP_2, INPUT_PULLUP);
  pinMode(ARM1_DIP_4, INPUT_PULLUP);
  pinMode(ARM1_DIP_8, INPUT_PULLUP);
  pinMode(ARM2_DIP_1, INPUT_PULLUP);
  pinMode(ARM2_DIP_2, INPUT_PULLUP);
  pinMode(ARM2_DIP_4, INPUT_PULLUP);
  pinMode(ARM2_DIP_8, INPUT_PULLUP);


  // Configure conveyor pin
  pinMode(CONVEYOR_PIN, OUTPUT);
  digitalWrite(CONVEYOR_PIN, LOW);  // Start with conveyor ON (active LOW)

  // Print startup messages
  printProgmemString(msg_system_start);

  // Load parameters from EEPROM or use defaults
  if (!loadParametersFromEEPROM()) {
    resetParametersToDefault();
    Serial.println(F("Using default parameters"));
  }

  printProgmemString(msg_dip_reading);

  // Read DIP switch values for layer positions
  readDipSwitchLayers();

  // Initialize ARM data
  initializeArmDataStateMachine();

  printProgmemString(msg_system_ready);

  Serial.print(F("ARM1 Commands: "));
  Serial.println(arm1_sm.total_commands);
  Serial.print(F("ARM2 Commands: "));
  Serial.println(arm2_sm.total_commands);
  Serial.println(F("=== USB SERIAL COMMANDS READY ==="));
  Serial.println(F("Type 'HELP' for available commands"));
}


void loop() {
  unsigned long currentTime = millis();

  // Read sensors
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    readSensors();
    lastSensorRead = currentTime;
  }

  // Update ARM state machines
  updateArmStateMachine(&arm1_sm);
  updateArmStateMachine(&arm2_sm);


  // Check command retry untuk kedua ARM
  checkCommandRetry(&arm1_sm);
  checkCommandRetry(&arm2_sm);

  // Handle system logic with state machine
  handleSystemLogicStateMachine();


  // Control conveyor
  controlConveyor();

  // Process USB commands
  processUSBCommands();

  //delay(10);
}


void readDipSwitchLayers() {
  // Read ARM1 layer position from DIP switch
  int arm1_layer = readArm1DipSwitch();
  if (arm1_layer >= 0 && arm1_layer <= 9) {
    arm1_sm.start_layer = arm1_layer;
  } else {
    arm1_sm.start_layer = 0;  // Default to 0 if invalid
  }

  // Read ARM2 layer position from DIP switch
  int arm2_layer = readArm2DipSwitch();
  if (arm2_layer >= 0 && arm2_layer <= 9) {
    arm2_sm.start_layer = arm2_layer;
  } else {
    arm2_sm.start_layer = 0;  // Default to 0 if invalid
  }

  Serial.print(F("ARM1 DIP Switch Layer: "));
  Serial.print(arm1_sm.start_layer);
  Serial.print(F(" (Layer "));
  Serial.print(arm1_sm.start_layer + 1);
  Serial.println(F(")"));

  Serial.print(F("ARM2 DIP Switch Layer: "));
  Serial.print(arm2_sm.start_layer);
  Serial.print(F(" (Layer "));
  Serial.print(arm2_sm.start_layer + 1);
  Serial.println(F(")"));
}


void printProgmemString(const char* str) {
  char buffer[60];
  strcpy_P(buffer, str);
  Serial.println(buffer);
}


// Generate command on-demand instead of storing all commands
String generateCommand(byte armId, int commandIndex) {
  // Calculate which layer and task this command belongs to
  int commandPair = commandIndex / 2;  // Each pair has HOME and GLAD
  int layer = commandPair / 8;
  int task = commandPair % 8;
  bool isHomeCommand = (commandIndex % 2 == 0);

  if (layer >= params.Ly || task >= 8) return "";

  // Select offset based on ARM
  int xOffset = (armId == 1) ? params.xL : params.xR;
  int yOffset = (armId == 1) ? params.yL : params.yR;
  int zOffset = (armId == 1) ? params.zL : params.zR;
  int tOffset = (armId == 1) ? params.tL : params.tR;
  int gOffset = (armId == 1) ? params.gL : params.gR;

  bool isOdd = (layer % 2 == 0);
  int zValue = calculateZ(layer);

  int yParam = (params.y_pattern[task] == 1) ? params.y1 : params.y2;

  if (isHomeCommand) {
    // Generate HOME command
    int homeX = (params.x + xOffset) * MULTIPLIER;
    int homeY = (yParam + yOffset) * MULTIPLIER;
    int homeZ = (params.z + zOffset) * MULTIPLIER;
    int homeT = (params.t + tOffset) * MULTIPLIER;
    int homeG = (params.g + gOffset) * MULTIPLIER;

    sprintf(commandBuffer, "H(%d,%d,%d,%d,%d)", homeX, homeY, homeZ, homeT, homeG);
  } else {
    // Generate GLAD command
    int gladXn, gladYn, gladTn;
    if (isOdd) {
      gladXn = (calculateXO(task) + xOffset) * MULTIPLIER;
      gladYn = (calculateYO(task) + yOffset) * MULTIPLIER;
      gladTn = (calculateTO(task) + tOffset) * MULTIPLIER;
    } else {
      gladXn = (calculateXE(task) + xOffset) * MULTIPLIER;
      gladYn = (calculateYE(task) + yOffset) * MULTIPLIER;
      gladTn = (calculateTE(task) + tOffset) * MULTIPLIER;
    }

    int gladZn = (zValue + zOffset) * MULTIPLIER;
    int gladDp = (params.dp + gOffset) * MULTIPLIER;
    int gladGp = (params.gp + gOffset) * MULTIPLIER;
    int gladZa = params.za * MULTIPLIER;
    int gladZb = (params.zb + zOffset) * MULTIPLIER;
    int gladXa = (params.XO5 + xOffset - 0) * MULTIPLIER;
    int gladTa = (params.t + tOffset) * MULTIPLIER;

    sprintf(commandBuffer, "G(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)",
            gladXn, gladYn, gladZn, gladTn, gladDp, gladGp, gladZa, gladZb, gladXa, gladTa);
  }

  return String(commandBuffer);
}


void readSensors() {
  // Store previous sensor3 state BEFORE reading new state
  bool prev_sensor3 = sensor3_state;

  // Sensor 1 and 2: HIGH when object detected (blocked)
  sensor1_state = digitalRead(SENSOR1_PIN);
  sensor2_state = digitalRead(SENSOR2_PIN);

  // Sensor 3: LOW when ARM is in center (blocked)
  sensor3_state = digitalRead(SENSOR3_PIN);

  // ARM pins: HIGH when busy
  arm1_response = digitalRead(ARM1_PIN);
  arm2_response = digitalRead(ARM2_PIN);

  arm1_sm.is_busy = arm1_response;
  arm2_sm.is_busy = arm2_response;


  if (prev_sensor3 == false && sensor3_state == true && arm_in_center != 0) {
    Serial.print(F("ARM"));
    Serial.print(arm_in_center);
    Serial.println(F(" left center - delay"));

    delay(LEAVE_CENTER_DELAY);
    arm_in_center = 0;

    Serial.println(F("arm_in_center reset to 0"));
  }

  // Update global previous state untuk keperluan lain
  sensor3_prev_state = sensor3_state;
}


// Hitung checksum XOR
uint8_t calculateXORChecksum(const char* data, int length) {
  uint8_t checksum = 0;
  for (int i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}


void sendRS485CommandWithRetry(ArmDataStateMachine* arm, String command) {
  arm->last_command_sent = command;
  arm->command_sent_time = millis();
  arm->retry_count = 0;
  arm->waiting_for_busy_response = true;

  sendRS485Command(command);  // fungsi existing
}


void sendRS485Command(String command) {
  // Since RE/DE is connected to VCC, no need to control it
  uint8_t checksum = calculateXORChecksum(command.c_str(), command.length());
  String fullCommand = command + "*" + String(checksum, HEX);
  rs485.println(fullCommand);
  rs485.flush();
  delay(50);  // Small delay to ensure transmission
}


void checkCommandRetry(ArmDataStateMachine* arm) {
  if (!arm->waiting_for_busy_response) return;

  unsigned long elapsed = millis() - arm->command_sent_time;

  // Jika ARM sudah busy, command berhasil
  if (arm->is_busy) {
    arm->waiting_for_busy_response = false;
    arm->retry_count = 0;
    return;
  }

  // Jika timeout dan belum max retry
  if (elapsed > arm->BUSY_RESPONSE_TIMEOUT) {
    if (arm->retry_count < arm->MAX_RETRY_COUNT) {
      arm->retry_count++;
      arm->command_sent_time = millis();

      Serial.print(F("ARM"));
      Serial.print(arm->arm_id);
      Serial.print(F(" retry #"));
      Serial.print(arm->retry_count);
      Serial.print(F(": "));
      Serial.println(arm->last_command_sent);

      sendRS485Command(arm->last_command_sent);
    } else {
      // Max retry tercapai, masuk error state
      Serial.print(F("ARM"));
      Serial.print(arm->arm_id);
      Serial.println(F(" failed to respond after retries - ERROR state"));

      arm->waiting_for_busy_response = false;
      arm->retry_count = 0;
      changeArmState(arm, ARM_ERROR);
    }
  }
}


// Control conveyor after GLAD command
void controlConveyor() {
  unsigned long currentTime = millis();

  // Check if conveyor should be turned back ON
  if (!conveyor_active && currentTime >= conveyor_off_time) {
    conveyor_active = true;
    digitalWrite(CONVEYOR_PIN, LOW);  // Turn ON (active LOW)
    Serial.println(F("Conveyor turned ON"));
  }
}


// Turn OFF conveyor for specified duration
void turnOffConveyor() {
  conveyor_active = false;
  conveyor_off_time = millis() + CONVEYOR_OFF_DURATION;
  digitalWrite(CONVEYOR_PIN, HIGH);  // Turn OFF (active LOW)
  Serial.println(F("Conveyor turned OFF for 3 seconds"));
}

// ===================================================================
// USB COMMAND INTERFACE
// ===================================================================

void processUSBCommands() {
  if (Serial.available()) {
    while (Serial.available() && usbBufferIndex < sizeof(usbBuffer) - 1) {
      char receivedChar = Serial.read();

      if (receivedChar == '\n' || receivedChar == '\r') {
        usbBuffer[usbBufferIndex] = '\0';
        if (usbBufferIndex > 0) {
          executeUSBCommand(String(usbBuffer));
        }
        usbBufferIndex = 0;
        memset(usbBuffer, 0, sizeof(usbBuffer));
      } else {
        usbBuffer[usbBufferIndex++] = receivedChar;
      }
    }
  }
}

void executeUSBCommand(String command) {
  command.trim();
  command.toUpperCase();

  Serial.print(F("USB Command: "));
  Serial.println(command);

  if (command == "HELP") {
    printUSBHelp();
  } else if (command == "STATUS") {
    printSystemStatus();
  } else if (command == "RUN_ARM1") {
    forceRunARM(1);
  } else if (command == "RUN_ARM2") {
    forceRunARM(2);
  } else if (command == "HOME_ARM1") {
    forceHomeCommand(1);
  } else if (command == "HOME_ARM2") {
    forceHomeCommand(2);
  } else if (command == "GLAD_ARM1") {
    forceGladCommand(1);
  } else if (command == "GLAD_ARM2") {
    forceGladCommand(2);
  } else if (command == "RESET_ARM1") {
    resetARM(1);
  } else if (command == "RESET_ARM2") {
    resetARM(2);
  } else if (command == "FORCE_READY_ARM1") {
    forceArmState(1, ARM_IDLE);
  } else if (command == "FORCE_READY_ARM2") {
    forceArmState(2, ARM_IDLE);
  } else if (command.startsWith("SEND_")) {
    String armCommand = command.substring(5);
    forceDirectCommand(armCommand);
  } else {
    Serial.println(F("Unknown command. Type HELP for available commands."));
  }
}

void printUSBHelp() {
  Serial.println(F("========== USB COMMAND INTERFACE =========="));
  Serial.println(F("Available commands:"));
  Serial.println(F("  HELP           - Show this help"));
  Serial.println(F("  STATUS         - Show system status"));
  Serial.println(F("  RUN_ARM1       - Force run ARM1 (bypass busy check)"));
  Serial.println(F("  RUN_ARM2       - Force run ARM2 (bypass busy check)"));
  Serial.println(F("  HOME_ARM1      - Send HOME command to ARM1"));
  Serial.println(F("  HOME_ARM2      - Send HOME command to ARM2"));
  Serial.println(F("  GLAD_ARM1      - Send GLAD command to ARM1"));
  Serial.println(F("  GLAD_ARM2      - Send GLAD command to ARM2"));
  Serial.println(F("  RESET_ARM1     - Reset ARM1 state machine"));
  Serial.println(F("  RESET_ARM2     - Reset ARM2 state machine"));
  Serial.println(F("  FORCE_READY_ARM1 - Force ARM1 to IDLE state"));
  Serial.println(F("  FORCE_READY_ARM2 - Force ARM2 to IDLE state"));
  Serial.println(F("  SEND_<cmd>     - Send direct command (e.g., SEND_R#H(1,2,3,4,5))"));
  Serial.println(F("=========================================="));
}

void printSystemStatus() {
  Serial.println(F("========== SYSTEM STATUS =========="));

  Serial.print(F("ARM1 State: "));
  Serial.print(getStateString(arm1_sm.state));
  Serial.print(F(", Busy: "));
  Serial.print(arm1_sm.is_busy ? F("YES") : F("NO"));
  Serial.print(F(", Pos: "));
  Serial.print(arm1_sm.current_pos);
  Serial.print(F("/"));
  Serial.println(arm1_sm.total_commands);

  Serial.print(F("ARM2 State: "));
  Serial.print(getStateString(arm2_sm.state));
  Serial.print(F(", Busy: "));
  Serial.print(arm2_sm.is_busy ? F("YES") : F("NO"));
  Serial.print(F(", Pos: "));
  Serial.print(arm2_sm.current_pos);
  Serial.print(F("/"));
  Serial.println(arm2_sm.total_commands);

  Serial.print(F("Hardware - ARM1_PIN: "));
  Serial.print(digitalRead(ARM1_PIN));
  Serial.print(F(", ARM2_PIN: "));
  Serial.println(digitalRead(ARM2_PIN));

  bool arm1_ready = (arm1_sm.state == ARM_IDLE) && !arm1_sm.is_busy && !arm1_sm.need_special_command;
  bool arm2_ready = (arm2_sm.state == ARM_IDLE) && !arm2_sm.is_busy && !arm2_sm.need_special_command;

  Serial.print(F("ARM1 Ready: "));
  Serial.print(arm1_ready ? F("YES") : F("NO"));
  Serial.print(F(", ARM2 Ready: "));
  Serial.println(arm2_ready ? F("YES") : F("NO"));

  Serial.print(F("Force flags - ARM1: "));
  Serial.print(force_run_arm1 ? F("ON") : F("OFF"));
  Serial.print(F(", ARM2: "));
  Serial.println(force_run_arm2 ? F("ON") : F("OFF"));

  Serial.println(F("=================================="));
}

void forceRunARM(byte armId) {
  if (armId == 1) {
    force_run_arm1 = true;
    Serial.println(F("ARM1 force run ENABLED - will bypass busy check"));
  } else if (armId == 2) {
    force_run_arm2 = true;
    Serial.println(F("ARM2 force run ENABLED - will bypass busy check"));
  }
}

void forceHomeCommand(byte armId) {
  ArmDataStateMachine* arm = (armId == 1) ? &arm1_sm : &arm2_sm;
  String command = getNextCommandStateMachine(arm);

  if (command.length() > 0) {
    String armPrefix = (armId == 1) ? "L" : "R";
    String fullCommand = armPrefix + "#" + command;

    Serial.print(F("Force sending HOME to ARM"));
    Serial.print(armId);
    Serial.print(F(": "));
    Serial.println(fullCommand);

    sendRS485CommandWithRetry(arm, fullCommand);
    changeArmState(arm, ARM_MOVING_TO_CENTER);
    arm_in_center = armId;
  } else {
    Serial.print(F("No more commands for ARM"));
    Serial.println(armId);
  }
}

void forceGladCommand(byte armId) {
  ArmDataStateMachine* arm = (armId == 1) ? &arm1_sm : &arm2_sm;
  String command = getNextCommandStateMachine(arm);

  if (command.length() > 0) {
    String armPrefix = (armId == 1) ? "L" : "R";
    String fullCommand = armPrefix + "#" + command;

    Serial.print(F("Force sending GLAD to ARM"));
    Serial.print(armId);
    Serial.print(F(": "));
    Serial.println(fullCommand);

    sendRS485CommandWithRetry(arm, fullCommand);
    changeArmState(arm, ARM_PICKING);
    turnOffConveyor();
  } else {
    Serial.print(F("No more commands for ARM"));
    Serial.println(armId);
  }
}

void resetARM(byte armId) {
  ArmDataStateMachine* arm = (armId == 1) ? &arm1_sm : &arm2_sm;

  arm->current_pos = arm->start_layer * 16;
  arm->state = ARM_IDLE;
  arm->is_busy = false;
  arm->need_special_command = false;
  arm->pending_special_command = SPECIAL_NONE;

  if (armId == 1) force_run_arm1 = false;
  else force_run_arm2 = false;

  Serial.print(F("ARM"));
  Serial.print(armId);
  Serial.println(F(" reset to initial state"));
}

void forceArmState(byte armId, ArmState newState) {
  ArmDataStateMachine* arm = (armId == 1) ? &arm1_sm : &arm2_sm;

  changeArmState(arm, newState);
  arm->is_busy = false;

  Serial.print(F("ARM"));
  Serial.print(armId);
  Serial.print(F(" forced to state: "));
  Serial.println(getStateString(newState));
}

void forceDirectCommand(String command) {
  Serial.print(F("Force sending direct command: "));
  Serial.println(command);

  byte armId = (command.charAt(0) == 'L') ? 1 : 2;
  ArmDataStateMachine* arm = (armId == 1) ? &arm1_sm : &arm2_sm;

  sendRS485CommandWithRetry(arm, command);

  if (command.indexOf("#H(") > 0) {
    changeArmState(arm, ARM_MOVING_TO_CENTER);
    arm_in_center = armId;
  } else if (command.indexOf("#G(") > 0) {
    changeArmState(arm, ARM_PICKING);
    turnOffConveyor();
  }
}
