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

// EEPROM Header structure
struct EEPROMHeader {
  int magic;
  int version;
  int checksum;
};

// Configuration Parameters - Stored in SRAM for runtime access
struct Parameters {
  // HOME Parameters untuk ARM LEFT
  int x_L;   // Home X LEFT
  int y1_L;  // Home Y pick 2 LEFT
  int y2_L;  // Home Y pick 1 LEFT
  int z_L;   // Home Z before product ready LEFT
  int t_L;   // Home T LEFT
  int g_L;   // Home G LEFT

  // HOME Parameters untuk ARM RIGHT
  int x_R;   // Home X RIGHT
  int y1_R;  // Home Y pick 2 RIGHT
  int y2_R;  // Home Y pick 1 RIGHT
  int z_R;   // Home Z before product ready RIGHT
  int t_R;   // Home T RIGHT
  int g_R;   // Home G RIGHT

  // GLAD Parameters untuk ARM LEFT
  int gp_L;   // Grip LEFT
  int dp_L;   // Drop LEFT
  int za_L;   // Relative lift axis when pick product LEFT
  int zb_L;   // Home Z after product ready LEFT
  int Z1_L;   // Z layer 1 LEFT
  int T90_L;  // T90 value

  // GLAD Parameters untuk ARM RIGHT
  int gp_R;   // Grip RIGHT
  int dp_R;   // Drop RIGHT
  int za_R;   // Relative lift axis when pick product RIGHT
  int zb_R;   // Home Z after product ready RIGHT
  int Z1_R;   // Z layer 1 RIGHT
  int T90_R;  // T90 value

  // Global Parameters (sama untuk kedua ARM)
  int H;   // Tinggi Product
  int Ly;  // Jumlah layer

  // Base positions untuk ARM LEFT (32 parameters)
  int XO1_L, YO1_L, XE1_L, YE1_L;  // Task 1 LEFT
  int XO2_L, YO2_L, XE2_L, YE2_L;  // Task 2 LEFT
  int XO3_L, YO3_L, XE3_L, YE3_L;  // Task 3 LEFT
  int XO4_L, YO4_L, XE4_L, YE4_L;  // Task 4 LEFT
  int XO5_L, YO5_L, XE5_L, YE5_L;  // Task 5 LEFT
  int XO6_L, YO6_L, XE6_L, YE6_L;  // Task 6 LEFT
  int XO7_L, YO7_L, XE7_L, YE7_L;  // Task 7 LEFT
  int XO8_L, YO8_L, XE8_L, YE8_L;  // Task 8 LEFT

  // Base positions untuk ARM RIGHT (32 parameters)
  int XO1_R, YO1_R, XE1_R, YE1_R;  // Task 1 RIGHT
  int XO2_R, YO2_R, XE2_R, YE2_R;  // Task 2 RIGHT
  int XO3_R, YO3_R, XE3_R, YE3_R;  // Task 3 RIGHT
  int XO4_R, YO4_R, XE4_R, YE4_R;  // Task 4 RIGHT
  int XO5_R, YO5_R, XE5_R, YE5_R;  // Task 5 RIGHT
  int XO6_R, YO6_R, XE6_R, YE6_R;  // Task 6 RIGHT
  int XO7_R, YO7_R, XE7_R, YE7_R;  // Task 7 RIGHT
  int XO8_R, YO8_R, XE8_R, YE8_R;  // Task 8 RIGHT

  // Y pattern untuk kedua ARM (tetap sama)
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

// Function to calculate Z values based on rules - sekarang terpisah
int calculateZ_L(int layer) {
  if (layer == 0) return params.Z1_L;
  return params.Z1_L - layer * params.H;
}

int calculateZ_R(int layer) {
  if (layer == 0) return params.Z1_R;
  return params.Z1_R - layer * params.H;
}

// Function to calculate XO values based on rules
// Function untuk ARM LEFT
int calculateXO_L(int task) {
  switch (task) {
    case 0: return params.XO1_L;
    case 1: return params.XO2_L;
    case 2: return params.XO3_L;
    case 3: return params.XO4_L;
    case 4: return params.XO5_L;
    case 5: return params.XO6_L;
    case 6: return params.XO7_L;
    case 7: return params.XO8_L;
    default: return params.XO1_L;
  }
}

// Function untuk ARM RIGHT
int calculateXO_R(int task) {
  switch (task) {
    case 0: return params.XO1_R;
    case 1: return params.XO2_R;
    case 2: return params.XO3_R;
    case 3: return params.XO4_R;
    case 4: return params.XO5_R;
    case 5: return params.XO6_R;
    case 6: return params.XO7_R;
    case 7: return params.XO8_R;
    default: return params.XO1_R;
  }
}

// Function to calculate YO values based on rules
// Function untuk ARM LEFT
int calculateYO_L(int task) {
  switch (task) {
    case 0: return params.YO1_L;
    case 1: return params.YO2_L;
    case 2: return params.YO3_L;
    case 3: return params.YO4_L;
    case 4: return params.YO5_L;
    case 5: return params.YO6_L;
    case 6: return params.YO7_L;
    case 7: return params.YO8_L;
    default: return params.YO1_L;
  }
}

// Function untuk ARM RIGHT
int calculateYO_R(int task) {
  switch (task) {
    case 0: return params.YO1_R;
    case 1: return params.YO2_R;
    case 2: return params.YO3_R;
    case 3: return params.YO4_R;
    case 4: return params.YO5_R;
    case 5: return params.YO6_R;
    case 6: return params.YO7_R;
    case 7: return params.YO8_R;
    default: return params.YO1_R;
  }
}

// Function untuk ARM LEFT
int calculateXE_L(int task) {
  switch (task) {
    case 0: return params.XE1_L;
    case 1: return params.XE2_L;
    case 2: return params.XE3_L;
    case 3: return params.XE4_L;
    case 4: return params.XE5_L;
    case 5: return params.XE6_L;
    case 6: return params.XE7_L;
    case 7: return params.XE8_L;
    default: return params.XE1_L;
  }
}

// Function untuk ARM RIGHT
int calculateXE_R(int task) {
  switch (task) {
    case 0: return params.XE1_R;
    case 1: return params.XE2_R;
    case 2: return params.XE3_R;
    case 3: return params.XE4_R;
    case 4: return params.XE5_R;
    case 5: return params.XE6_R;
    case 6: return params.XE7_R;
    case 7: return params.XE8_R;
    default: return params.XE1_R;
  }
}

// Function untuk ARM LEFT
int calculateYE_L(int task) {
  switch (task) {
    case 0: return params.YE1_L;
    case 1: return params.YE2_L;
    case 2: return params.YE3_L;
    case 3: return params.YE4_L;
    case 4: return params.YE5_L;
    case 5: return params.YE6_L;
    case 6: return params.YE7_L;
    case 7: return params.YE8_L;
    default: return params.YE1_L;
  }
}

// Function untuk ARM RIGHT
int calculateYE_R(int task) {
  switch (task) {
    case 0: return params.YE1_R;
    case 1: return params.YE2_R;
    case 2: return params.YE3_R;
    case 3: return params.YE4_R;
    case 4: return params.YE5_R;
    case 5: return params.YE6_R;
    case 6: return params.YE7_R;
    case 7: return params.YE8_R;
    default: return params.YE1_R;
  }
}

// Function to calculate TO values based on rules - FIXED: Add return statement
int calculateTO_L(int task) {
  if (task >= 0 && task <= 3) {
    return params.T90_L;  // TO1 to TO4
  } else if (task >= 4 && task <= 7) {
    return params.t_L;  // TO5 to TO8
  }
  return params.t_L;  // FIXED: Default return value
}

int calculateTO_R(int task) {
  if (task >= 0 && task <= 3) {
    return params.T90_R;  // TO1 to TO4
  } else if (task >= 4 && task <= 7) {
    return params.t_R;  // TO5 to TO8
  }
  return params.t_R;  // FIXED: Default return value
}

// Function to calculate TE values based on rules - FIXED: Add return statement
int calculateTE_L(int task) {
  if (task >= 0 && task <= 3) {
    return params.T90_L;  // TE1 to TE4
  } else if (task >= 4 && task <= 7) {
    return params.t_L;  // TE5 to TE8
  }
  return params.t_L;  // FIXED: Default return value
}

int calculateTE_R(int task) {
  if (task >= 0 && task <= 3) {
    return params.T90_R;  // TE1 to TE4
  } else if (task >= 4 && task <= 7) {
    return params.t_R;  // TE5 to TE8
  }
  return params.t_R;  // FIXED: Default return value
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
  // FIXED: Default values matching V1 exactly with separate parameters
  params.H = 100;
  params.Ly = 11;
  
  // ARM LEFT (base dari V1: x=1305, offset xL=0)
  params.x_L = 1305;    // V1: x + xL = 1305 + 0 = 1305
  params.y1_L = 130;    // V1: y1 + yL = 130 + 0 = 130  
  params.y2_L = 410;    // V1: y2 + yL = 410 + 0 = 410
  params.z_L = 1280;    // V1: z + zL = 1280 + 0 = 1280
  params.t_L = 80;      // V1: t + tL = 80 + 0 = 80
  params.g_L = -10;     // V1: g + gL = -10 + 0 = -10
  params.gp_L = 90;     // V1: gp + gL = 90 + 0 = 90
  params.dp_L = 40;     // FIXED: V1: dp + gL = 40 + 0 = 40
  params.za_L = 250;    // V1: za (no offset) = 250
  params.zb_L = 1320;   // FIXED: V1: zb + zL = 1320 + 0 = 1320
  params.T90_L = 1680;  // FIXED: V1: T90 + tL = (1600 + 80) + 0 = 1680
  params.Z1_L = 1325;   // FIXED: V1: Z1 + zL = 1325 + 0 = 1325
  
  // ARM RIGHT (base dari V1: x=1305, offset xR=-20)
  params.x_R = 1285;    // V1: x + xR = 1305 + (-20) = 1285
  params.y1_R = 130;    // V1: y1 + yR = 130 + 0 = 130
  params.y2_R = 410;    // V1: y2 + yR = 410 + 0 = 410
  params.z_R = 1280;    // V1: z + zR = 1280 + 0 = 1280
  params.t_R = 50;      // FIXED: V1: t + tR = 80 + (-30) = 50
  params.g_R = -5;      // V1: g + gR = -10 + 5 = -5
  params.gp_R = 95;     // FIXED: V1: gp + gR = 90 + 5 = 95
  params.dp_R = 45;     // FIXED: V1: dp + gR = 40 + 5 = 45
  params.za_R = 250;    // V1: za (no offset) = 250
  params.zb_R = 1320;   // V1: zb + zR = 1320 + 0 = 1320
  params.T90_R = 1650;  // FIXED: V1: T90 + tR = (1600 + 80) + (-30) = 1650
  params.Z1_R = 1325;   // V1: Z1 + zR = 1325 + 0 = 1325
  // ARM LEFT positions (base dari V1 + xL=0, yL=0)
  params.XO1_L = 645;  // V1: XO1 + xL = 645 + 0 = 645
  params.YO1_L = 310;  // V1: YO1 + yL = 310 + 0 = 310
  params.XO2_L = 250;  // V1: XO2 + xL = 250 + 0 = 250
  params.YO2_L = 310;  // V1: YO2 + yL = 310 + 0 = 310
  params.XO3_L = 645;  // V1: XO3 + xL = 645 + 0 = 645
  params.YO3_L = 65;   // V1: YO3 + yL = 65 + 0 = 65
  params.XO4_L = 250;  // V1: XO4 + xL = 250 + 0 = 250
  params.YO4_L = 65;   // V1: YO4 + yL = 65 + 0 = 65
  params.XO5_L = 785;  // FIXED: V1: XO5 + xL = 785 + 0 = 785
  params.YO5_L = 735;  // FIXED: V1: YO5 + yL = 735 + 0 = 735
  params.XO6_L = 545;  // FIXED: V1: XO6 + xL = 545 + 0 = 545
  params.YO6_L = 735;  // V1: YO6 + yL = 735 + 0 = 735
  params.XO7_L = 245;  // FIXED: V1: XO7 + xL = 245 + 0 = 245
  params.YO7_L = 735;  // V1: YO7 + yL = 735 + 0 = 735
  params.XO8_L = 5;    // FIXED: V1: XO8 + xL = 5 + 0 = 5
  params.YO8_L = 735;  // V1: YO8 + yL = 735 + 0 = 735
  params.XE1_L = 645;  // V1: XE1 + xL = 645 + 0 = 645
  params.YE1_L = 980;  // V1: YE1 + yL = 980 + 0 = 980
  params.XE2_L = 250;  // V1: XE2 + xL = 250 + 0 = 250
  params.YE2_L = 980;  // V1: YE2 + yL = 980 + 0 = 980
  params.XE3_L = 645;  // V1: XE3 + xL = 645 + 0 = 645
  params.YE3_L = 735;  // V1: YE3 + yL = 735 + 0 = 735
  params.XE4_L = 250;  // V1: XE4 + xL = 250 + 0 = 250
  params.YE4_L = 735;  // V1: YE4 + yL = 735 + 0 = 735
  params.XE5_L = 785;  // V1: XE5 + xL = 785 + 0 = 785
  params.YE5_L = 250;  // FIXED: V1: YE5 + yL = 250 + 0 = 250
  params.XE6_L = 545;  // V1: XE6 + xL = 545 + 0 = 545
  params.YE6_L = 250;  // V1: YE6 + yL = 250 + 0 = 250
  params.XE7_L = 245;  // V1: XE7 + xL = 245 + 0 = 245
  params.YE7_L = 250;  // V1: YE7 + yL = 250 + 0 = 250
  params.XE8_L = 5;    // V1: XE8 + xL = 5 + 0 = 5
  params.YE8_L = 250;  // V1: YE8 + yL = 250 + 0 = 250
  // ARM RIGHT positions (base dari V1 + xR=-20, yR=0)
  params.XO1_R = 625;  // V1: XO1 + xR = 645 + (-20) = 625
  params.YO1_R = 310;  // V1: YO1 + yR = 310 + 0 = 310
  params.XO2_R = 230;  // V1: XO2 + xR = 250 + (-20) = 230
  params.YO2_R = 310;  // V1: YO2 + yR = 310 + 0 = 310
  params.XO3_R = 625;  // V1: XO3 + xR = 645 + (-20) = 625
  params.YO3_R = 65;   // V1: YO3 + yR = 65 + 0 = 65
  params.XO4_R = 230;  // V1: XO4 + xR = 250 + (-20) = 230
  params.YO4_R = 65;   // V1: YO4 + yR = 65 + 0 = 65
  params.XO5_R = 765;  // FIXED: V1: XO5 + xR = 785 + (-20) = 765
  params.YO5_R = 735;  // V1: YO5 + yR = 735 + 0 = 735
  params.XO6_R = 525;  // FIXED: V1: XO6 + xR = 545 + (-20) = 525
  params.YO6_R = 735;  // V1: YO6 + yR = 735 + 0 = 735
  params.XO7_R = 225;  // FIXED: V1: XO7 + xR = 245 + (-20) = 225
  params.YO7_R = 735;  // V1: YO7 + yR = 735 + 0 = 735
  params.XO8_R = -15;  // FIXED: V1: XO8 + xR = 5 + (-20) = -15
  params.YO8_R = 735;  // V1: YO8 + yR = 735 + 0 = 735
  params.XE1_R = 625;  // V1: XE1 + xR = 645 + (-20) = 625
  params.YE1_R = 980;  // V1: YE1 + yR = 980 + 0 = 980
  params.XE2_R = 230;  // V1: XE2 + xR = 250 + (-20) = 230
  params.YE2_R = 980;  // V1: YE2 + yR = 980 + 0 = 980
  params.XE3_R = 625;  // V1: XE3 + xR = 645 + (-20) = 625
  params.YE3_R = 735;  // V1: YE3 + yR = 735 + 0 = 735
  params.XE4_R = 230;  // V1: XE4 + xR = 250 + (-20) = 230
  params.YE4_R = 735;  // V1: YE4 + yR = 735 + 0 = 735
  params.XE5_R = 765;  // V1: XE5 + xR = 785 + (-20) = 765
  params.YE5_R = 250;  // V1: YE5 + yR = 250 + 0 = 250
  params.XE6_R = 525;  // V1: XE6 + xR = 545 + (-20) = 525
  params.YE6_R = 250;  // V1: YE6 + yR = 250 + 0 = 250
  params.XE7_R = 225;  // V1: XE7 + xR = 245 + (-20) = 225
  params.YE7_R = 250;  // V1: YE7 + yR = 250 + 0 = 250
  params.XE8_R = -15;  // V1: XE8 + xR = 5 + (-20) = -15
  params.YE8_R = 250;  // V1: YE8 + yR = 250 + 0 = 250

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

  bool isOdd = (layer % 2 == 0);

  if (isHomeCommand) {
    // Generate HOME command - FIXED: Declare all variables properly
    int homeX, homeY, homeZ, homeT, homeG;

    if (armId == 1) {  // ARM LEFT
      homeX = params.x_L * MULTIPLIER;
      int yParam = (params.y_pattern[task] == 1) ? params.y1_L : params.y2_L;
      homeY = yParam * MULTIPLIER;  // FIXED: Remove 'int'
      homeZ = params.z_L * MULTIPLIER;  // FIXED: Remove 'int'
      homeT = params.t_L * MULTIPLIER;  // FIXED: Remove 'int'
      homeG = params.g_L * MULTIPLIER;  // FIXED: Remove 'int'
    } else {  // ARM RIGHT
      homeX = params.x_R * MULTIPLIER;
      int yParam = (params.y_pattern[task] == 1) ? params.y1_R : params.y2_R;
      homeY = yParam * MULTIPLIER;  // FIXED: Remove 'int'
      homeZ = params.z_R * MULTIPLIER;  // FIXED: Remove 'int'
      homeT = params.t_R * MULTIPLIER;  // FIXED: Remove 'int'
      homeG = params.g_R * MULTIPLIER;  // FIXED: Remove 'int'
    }
    sprintf(commandBuffer, "H(%d,%d,%d,%d,%d)", homeX, homeY, homeZ, homeT, homeG);
  } else {
    // Generate GLAD command - FIXED: Proper variable declaration
    int gladXn, gladYn, gladTn, gladZn, gladDp, gladGp, gladZa, gladZb, gladXa, gladTa;

    if (armId == 1) {  // ARM LEFT
      if (isOdd) {
        gladXn = calculateXO_L(task) * MULTIPLIER;
        gladYn = calculateYO_L(task) * MULTIPLIER;
        gladTn = calculateTO_L(task) * MULTIPLIER;
      } else {
        gladXn = calculateXE_L(task) * MULTIPLIER;
        gladYn = calculateYE_L(task) * MULTIPLIER;
        gladTn = calculateTE_L(task) * MULTIPLIER;
      }
      gladZn = calculateZ_L(layer) * MULTIPLIER;
      gladDp = params.dp_L * MULTIPLIER;
      gladGp = params.gp_L * MULTIPLIER;
      gladZa = params.za_L * MULTIPLIER;
      gladZb = params.zb_L * MULTIPLIER;
      gladXa = params.XO5_L * MULTIPLIER;
      gladTa = params.t_L * MULTIPLIER;
    } else {  // ARM RIGHT
      if (isOdd) {
        gladXn = calculateXO_R(task) * MULTIPLIER;
        gladYn = calculateYO_R(task) * MULTIPLIER;
        gladTn = calculateTO_R(task) * MULTIPLIER;
      } else {
        gladXn = calculateXE_R(task) * MULTIPLIER;
        gladYn = calculateYE_R(task) * MULTIPLIER;
        gladTn = calculateTE_R(task) * MULTIPLIER;
      }
      gladZn = calculateZ_R(layer) * MULTIPLIER;
      gladDp = params.dp_R * MULTIPLIER;
      gladGp = params.gp_R * MULTIPLIER;
      gladZa = params.za_R * MULTIPLIER;
      gladZb = params.zb_R * MULTIPLIER;
      gladXa = params.XO5_R * MULTIPLIER;
      gladTa = params.t_R * MULTIPLIER;
    }

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