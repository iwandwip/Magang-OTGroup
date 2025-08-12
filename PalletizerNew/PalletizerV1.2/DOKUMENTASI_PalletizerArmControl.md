# Dokumentasi PalletizerArmControl.ino

```
     ╔═══════════════════════════════════════════════════════════════╗
     ║                   PALLETIZER ARM CONTROL                     ║
     ║                   (Command Translator)                       ║
     ╠═══════════════════════════════════════════════════════════════╣
     ║  📡 RS485 Input     │  🎮 Multi-Step Execution             ║
     ║  🤖 Motor Control   │  🚦 State Management                 ║
     ║  🔘 Button Safety   │  ⚠️ Error Protection                 ║
     ╚═══════════════════════════════════════════════════════════════╝
```

## 📋 Deskripsi Program
**PalletizerArmControl** adalah program controller menengah yang menerima perintah dari CentralStateMachine dan mengubahnya menjadi perintah motor yang spesifik untuk mengontrol driver stepper motor. Program ini bertindak sebagai "translator" antara high-level commands dan low-level motor movements.

## 🎯 Fungsi Utama
- **Command Processing**: Menerima dan memproses perintah HOME, GLAD, PARK, CALI
- **Multi-Step Execution**: Memecah command kompleks menjadi langkah-langkah motor
- **Motor Coordination**: Mengkoordinasikan 5 motor axis (X, Y, Z, T, G)
- **State Management**: Mengelola state sistem (SLEEPING, READY, RUNNING, ZEROING)
- **Safety Features**: Button control, timeout protection, retry mechanism

## 🔧 Hardware Yang Digunakan

```
    ┌─────────────────────────────────────────────────────────────────┐
    │                       ARM CONTROLLER                           │
    │                     Arduino Uno/Nano                          │
    │                                                                 │
    │  D10,D11 ◄─► RS485 Module ◄─── CentralStateMachine            │
    │                                                                 │
    │  D8,D9 ───► AltSoftSerial ───► Motor Drivers                   │
    │                   │                                             │
    │                   ▼                                             │
    │           ┌─────────────────┐                                   │
    │           │  X Y Z T G      │                                   │
    │           │  Motor Drivers  │                                   │
    │           └─────────────────┘                                   │
    │                                                                 │
    │  LED Status:        Button Controls:                           │
    │  D5 ───► 🔴 RED      A0,A1 ◄─── 🔘 START (4s hold)            │
    │  D6 ───► 🟡 YELLOW   A2,A3 ◄─── 🔘 STOP (4s hold)             │
    │  D7 ───► 🟢 GREEN                                              │
    │                                                                 │
    │  D4 ───► 🔊 BUZZER   D3 ◄─── ⚡ MOTOR_DONE                     │
    │  D13 ──► 📍 CMD_ACTIVE                                         │
    │                                                                 │
    │  Device Detection: A4,A5 (ARM1 vs ARM2)                       │
    └─────────────────────────────────────────────────────────────────┘
```

- **Arduino Uno/Nano** sebagai ARM controller
- **RS485 Module** (Pin 10,11): Komunikasi dengan CentralStateMachine  
- **AltSoftSerial** (Pin 8,9): Komunikasi dengan motor drivers
- **LED Indicators**:
  - RED LED (Pin 5): Error/Sleeping state
  - YELLOW LED (Pin 6): Ready state  
  - GREEN LED (Pin 7): Running state
- **Button Controls**:
  - START Button (Pin A0,A1): 4-detik hold untuk mulai
  - STOP Button (Pin A2,A3): 4-detik hold untuk stop
- **BUZZER** (Pin 4): Audio feedback
- **MOTOR_DONE_PIN** (Pin 3): Feedback dari motor drivers
- **COMMAND_ACTIVE_PIN** (Pin 13): Status output untuk CentralStateMachine

## 🎮 State Machine Sistem

### 🔄 System States
```cpp
enum SystemState {
  STATE_ZEROING,   // Sistem melakukan homing/parking
  STATE_SLEEPING,  // Sistem idle, siap test commands  
  STATE_READY,     // Siap menerima HOME command
  STATE_RUNNING    // Sedang menjalankan operasi
}
```

### 🚦 State Transitions
```
ZEROING → SLEEPING (motor ready)
SLEEPING → READY (START button 4 detik)
READY → RUNNING (HOME command diterima)
RUNNING → READY (GLAD command selesai)
RUNNING → ZEROING (STOP button atau PARK command)
```

### 🎨 LED Indicators
- **ZEROING**: Blinking RED+YELLOW+GREEN+BUZZER
- **SLEEPING**: RED only
- **READY**: RED+YELLOW
- **RUNNING**: GREEN (atau blinking jika STOP pending)

## 📡 Protokol Komunikasi

### 📥 Input Commands (dari CentralStateMachine)
```
ARMx#COMMAND*CHECKSUM
```
Contoh:
```
ARML#HOME(3870,390,3840,240,-30)*5A
ARMR#GLAD(1620,2205,3975,240,60,270,750,3960,2340,240)*B7
ARML#PARK*1F
ARMR#CALI*2D
```

### 📤 Output Commands (ke Motor Drivers)
```
MOTOR_COMMAND*CHECKSUM
```
Contoh:
```
X3870,Y390,T240,G-30*4C
Z3840*A1
X1620,Y2205,T240*5E
```

### 🔐 Checksum Validation
```cpp
// Input validation
bool parseAndValidateMessage(const char* receivedMessage, char* cleanCommand);

// Output protection  
uint8_t checksum = calculateXORChecksum(command, strlen(command));
motorSerial.print(command + "*" + checksum);
```

## 🏠 HOME Command Processing

### 📋 HOME Command Format
```cpp
HOME(x, y, z, t, g)
```

### 🔄 HOME Sequence (2 Steps)
```cpp
case 1: // Step 1
  sendMotorCommand("X%d,Y%d,T%d,G%d", homeCmd.x, homeCmd.y, homeCmd.t, homeCmd.g);
  // Kirim semua axis kecuali Z terlebih dahulu
  
case 2: // Step 2  
  sendMotorCommand("Z%d", homeCmd.z);
  // Kirim Z axis terpisah untuk safety
```

### ⚡ Execution Flow
```
HOME diterima → Parse parameters → Step 1 (XYGT) → Wait motor ready → 
Step 2 (Z) → Wait motor ready → Sequence complete → READY→RUNNING transition
```

## 🤏 GLAD Command Processing

### 📋 GLAD Command Format
```cpp
GLAD(xn, yn, zn, tn, dp, gp, za, zb, xa, ta)
```

### 🔄 GLAD Sequence (9 Steps)

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                       GLAD SEQUENCE FLOW                         ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                                                                   ║
    ║  Step 1 ───► Z to Home      │ Z3960 (Move to safe height)        ║
    ║             Position        │                                     ║
    ║                 │           │                                     ║
    ║                 ▼           │                                     ║
    ║  Step 2 ───► Set Gripper    │ G270 (Open gripper)                ║
    ║             to Pick         │                                     ║
    ║                 │           │                                     ║
    ║                 ▼           │                                     ║
    ║  Step 3 ───► Lower Z for    │ Z3225 (zn-za = approach height)    ║
    ║             Approach        │                                     ║
    ║                 │           │                                     ║
    ║                 ▼           │                                     ║
    ║  Step 4 ───► Move to        │ X1620,Y2205,T240 (target position) ║
    ║             Target XYT      │                                     ║
    ║                 │           │                                     ║
    ║                 ▼           │                                     ║
    ║  Step 5 ───► Final Z        │ Z3975 (exact pickup height)        ║
    ║             Position        │                                     ║
    ║                 │           │                                     ║
    ║                 ▼           │                                     ║
    ║  Step 6 ───► Close Gripper  │ G60 (grip product)                 ║
    ║             (Grip)          │                                     ║
    ║                 │           │                                     ║
    ║                 ▼           │                                     ║
    ║  Step 7 ───► Lift Product   │ Z3225 (lift with product)          ║
    ║                 │           │                                     ║
    ║                 ▼           │                                     ║
    ║  Step 8 ───► Move to        │ X2340,T240 (standby position)      ║
    ║             Standby         │                                     ║
    ║                 │           │                                     ║
    ║                 ▼           │                                     ║
    ║  Step 9 ───► Complete       │ Return to READY state              ║
    ║                             │                                     ║
    ╚═══════════════════════════════════════════════════════════════════╝
```

#### **Step 1**: Move Z to home position
```cpp
sendMotorCommand("Z%d", gladCmd.zb);  // zb = 3960
```

#### **Step 2**: Set gripper to pick position  
```cpp
sendMotorCommand("G%d", gladCmd.gp);  // gp = 270
```

#### **Step 3**: Lower Z untuk pickup
```cpp
int z_position = gladCmd.zn - gladCmd.za;  // 3975 - 750 = 3225
sendMotorCommand("Z%d", z_position);
```

#### **Step 4**: Move to pickup position
```cpp
sendMotorCommand("X%d,Y%d,T%d", gladCmd.xn, gladCmd.yn, gladCmd.tn);
// "X1620,Y2205,T240"
```

#### **Step 5**: Final Z position untuk pickup
```cpp
sendMotorCommand("Z%d", gladCmd.zn);  // zn = 3975
```

#### **Step 6**: Close gripper untuk ambil produk
```cpp
sendMotorCommand("G%d", gladCmd.dp);  // dp = 60
```

#### **Step 7**: Lift product
```cpp
int z_position_final = gladCmd.zn - gladCmd.za;  // 3975 - 750 = 3225
sendMotorCommand("Z%d", z_position_final);
```

#### **Step 8**: Move to standby position sebelum return
```cpp
sendMotorCommand("X%d,T%d", gladCmd.xa, gladCmd.ta);  // "X2340,T240"
```

#### **Step 9**: Sequence complete
- Reset sequence counter
- Transition RUNNING → READY (normal)
- Atau RUNNING → ZEROING (jika STOP pressed)

### ⚡ GLAD Execution Flow
```
GLAD diterima → Parse 10 parameters → Step 1-9 dengan motor ready wait → 
Sequence complete → State transition
```

## ⚙️ Motor Control & Communication

### 📡 Motor Command Format
```cpp
// Single axis
"Z3840*A1"

// Multiple axis  
"X1620,Y2205,T240*5E"

// Dengan retry mechanism
sendSafeMotorCommand(PSTR("X%d,Y%d,T%d"), xn, yn, tn);
```

### 🔄 Motor Response Detection
```cpp
bool isMotorReady() {
  // Triple reading dengan majority vote untuk noise filtering
  bool reading1 = digitalRead(MOTOR_DONE_PIN);
  delay(20);
  bool reading2 = digitalRead(MOTOR_DONE_PIN); 
  delay(20);
  bool reading3 = digitalRead(MOTOR_DONE_PIN);
  
  // Majority vote logic
  return stableReading == HIGH;  // HIGH = ready, LOW = busy
}
```

### ⏱️ Motor State Management
```cpp
const unsigned long MOTOR_STABILIZE_MS = 50;  // 50ms stabilization delay

// State detection logic
if (motorReady && !motorWasReady) {
  lastMotorReadyTime = millis();
  motorWasReady = true;
  return;  // Wait for stabilization
}

if (motorReady && motorWasReady) {
  if (millis() - lastMotorReadyTime >= MOTOR_STABILIZE_MS) {
    executeNextStep();  // Continue sequence after 50ms
    motorWasReady = false;
  }
}
```

## 🛡️ Safety Features

### 🔘 Button Control System

#### **4-Second Hold Requirement**
```cpp
const unsigned long HOLD_DURATION_MS = 4000;  // 4 detik hold

// Capacitive touch detection
digitalWrite(START_OUTPUT_PIN, LOW);
bool testLow = (digitalRead(START_INPUT_PIN) == LOW);
digitalWrite(START_OUTPUT_PIN, HIGH);  
bool testHigh = (digitalRead(START_INPUT_PIN) == HIGH);
bool currentlyPressed = testLow && testHigh;
```

#### **Feedback System**
```cpp
// Progress feedback setiap detik
Serial.print("START hold time: ");
Serial.print(holdTime / 1000);
Serial.println(" seconds");
```

### ⏰ Timeout Protection
```cpp
const unsigned long DRIVER_TIMEOUT_MS = 10000;  // 10 detik max
const int MAX_MOTOR_RETRIES = 10;               // 10x retry max
const unsigned long MIN_COMMAND_INTERVAL_MS = 100;  // 100ms antar command
```

### 🔄 Retry Mechanism
```cpp
for (int attempt = 1; attempt <= MAX_MOTOR_RETRIES; attempt++) {
  if (waitForMotorResponse(200)) {
    return;  // Success
  } else {
    Serial.print("Retry attempt: ");
    Serial.println(attempt);
    delay(50);  // 50ms delay sebelum retry
  }
}
handleMotorTimeout();  // Enter error state
```

## 🏭 Special Commands

### 🏠 PARK Command
Mengembalikan semua motor ke posisi home:
```cpp
// Step 1: Z0 (2 detik delay)
sendMotorCommand("Z0");

// Step 2: X0,Y0,T0,G0  
sendMotorCommand("X0,Y0,T0,G0");
```

### ⚙️ CALIBRATION Command
Sama dengan PARK tapi akan ke READY state (bukan SLEEPING):
```cpp
isCalibrationMode = true;  // Flag untuk tujuan state
enterZeroingState();       // Jalankan PARK sequence
// Setelah selesai → READY (bukan SLEEPING)
```

## 🎛️ Device Detection

### 🔍 ARM1 vs ARM2 Detection  
```cpp
bool isARM2() {
  return digitalRead(A4) == LOW;  // A4 connected to A5 (GND) = ARM2
}

// Device prefix
const char* devicePrefix = isARM2_device ? "ARMR" : "ARML";
```

### 📡 Command Filtering
```cpp
if (strncmp(command, devicePrefix, 4) == 0) {
  // Command untuk device ini
  executeCommand(action);
} else {
  // Ignore command untuk device lain
}
```

## 🛠️ Development & Testing

### 💻 USB Debug Commands (SLEEPING State)
```cpp
void processUSBCommands() {
  if (currentState != STATE_SLEEPING) return;
  
  // Direct motor command testing
  // Contoh: "X1000,Y500" langsung dikirim ke motor
}
```

### 📊 Debug Output
Program menyediakan verbose logging untuk:
- Command parsing
- Step progression  
- Motor responses
- State transitions
- Error conditions

### 🔧 Buffer Management
```cpp
char commandBuffer[64];    // Input command buffer
char command[64];          // Motor command buffer

// Overflow protection
if (result >= 0 && result < sizeof(command)) {
  sendMotorCommand(command);
} else {
  Serial.println("ERROR: Command buffer overflow prevented");
}
```

## ⚠️ Error Handling

### 🚨 Critical Error States
```cpp
void handleMotorTimeout() {
  // Yellow LED + Buzzer continuous
  while (true) {
    setLedState(false, true, false);  // Yellow only
    setBuzzerState(true);
    delay(500);
    setBuzzerState(false);
    delay(500);
  }
}
```

### 🔄 Recovery Mechanisms
- **Automatic retry**: Up to 10x dengan delay
- **State reset**: Return ke safe state pada error
- **Manual intervention**: Error state untuk human debugging
- **Command validation**: Reject invalid/corrupted commands

## 📊 Command Flow Summary

### 🏠 HOME Flow
```
CentralStateMachine → "ARML#HOME(3870,390,3840,240,-30)*5A"
↓ (checksum validation)
ArmControl → Parse parameters → homeCmd struct
↓ (motor ready wait)  
Step 1: "X3870,Y390,T240,G-30*4C" → Motors
↓ (motor response + 50ms wait)
Step 2: "Z3840*A1" → Motors  
↓ (motor response + 50ms wait)
Sequence Complete → READY→RUNNING
```

### 🤏 GLAD Flow  
```
CentralStateMachine → "ARMR#GLAD(1620,2205,3975,240,60,270,750,3960,2340,240)*B7"
↓ (checksum validation)
ArmControl → Parse 10 parameters → gladCmd struct
↓ (9-step motor sequence)
Steps 1-9: Various motor commands
↓ (motor response wait per step)
Sequence Complete → RUNNING→READY
```

---

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                     ARM CONTROL OVERVIEW                         ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                                                                   ║
    ║  📡 RS485 INPUT: "ARML#GLAD(1620,2205,3975,240,60,270,750,3960,2340,240)*B7" ║
    ║                               │                                   ║
    ║                               ▼                                   ║
    ║  🔐 CHECKSUM VALIDATION & PARSING                                 ║
    ║                               │                                   ║
    ║                               ▼                                   ║
    ║  🎮 ARM CONTROLLER (Arduino Uno)                                  ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ State: RUNNING     │ 🔴🟡🟢 LED Status                      │  ║
    ║  │ Sequence: GLAD     │ 🔘 Button Safety (4s hold)            │  ║
    ║  │ Step: 5/9          │ 🔊 Audio Feedback                      │  ║
    ║  │ Motor: READY       │ ⚡ Motor Done Detection                │  ║
    ║  └─────────────────────────────────────────────────────────────┘  ║
    ║                               │                                   ║
    ║                               ▼                                   ║
    ║  📤 MOTOR COMMANDS (with checksum):                               ║
    ║  ├─ Step 1: "Z3960*A1"                                           ║
    ║  ├─ Step 2: "G270*4C"                                            ║
    ║  ├─ Step 3: "Z3225*B5"                                           ║
    ║  ├─ Step 4: "X1620,Y2205,T240*5E"                               ║
    ║  ├─ Step 5: "Z3975*A1"  ◄── Current                             ║
    ║  ├─ Step 6: "G60*C7"                                             ║
    ║  ├─ Step 7: "Z3225*B5"                                           ║
    ║  ├─ Step 8: "X2340,T240*8F"                                     ║
    ║  └─ Step 9: Complete → READY                                     ║
    ║                               │                                   ║
    ║                               ▼                                   ║
    ║  🤖 MOTOR DRIVERS (X, Y, Z, T, G)                                ║
    ║                                                                   ║
    ║  ⚡ MOTOR RESPONSE: D3 (LOW=Busy, HIGH=Ready)                     ║
    ║  ⏱️ STABILIZATION: 50ms delay before next step                   ║
    ║                                                                   ║
    ╚═══════════════════════════════════════════════════════════════════╝
```

**💡 Tips**: Program ini menggunakan robust error handling, state machine management, dan safety features untuk memastikan operasi yang aman dan reliable dalam environment industri. Semua commands memiliki checksum protection dan retry mechanism.