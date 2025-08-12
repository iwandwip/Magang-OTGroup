# Dokumentasi PalletizerArmDriver.ino

```
     ╔═══════════════════════════════════════════════════════════════╗
     ║                    PALLETIZER ARM DRIVER                     ║
     ║                   (Individual Motor Control)                 ║
     ╠═══════════════════════════════════════════════════════════════╣
     ║  🎛️ Single Axis     │  🏠 Homing Control                   ║
     ║  ⚡ AccelStepper    │  🔧 Speed Management                  ║
     ║  🆔 Driver ID       │  📍 Position Tracking                ║
     ╚═══════════════════════════════════════════════════════════════╝
```

## 📋 Deskripsi Program
**PalletizerArmDriver** adalah program driver level terendah yang mengontrol individual stepper motor untuk setiap axis (X, Y, Z, T, G). Setiap Arduino menjalankan program ini untuk mengontrol satu motor axis saja. Program ini menerima perintah dari ArmControl dan mengkonversinya menjadi gerakan motor yang presisi.

## 🎯 Fungsi Utama
- **Motor Control**: Mengontrol satu stepper motor dengan AccelStepper library
- **Position Management**: Mengelola posisi absolut motor dengan encoder feedback
- **Homing Operations**: Melakukan homing sequence dengan limit switch
- **Speed Management**: Dynamic speed berdasarkan jarak dan axis type
- **Communication**: Menerima perintah dari ArmControl dengan checksum validation

## 🔧 Hardware Yang Digunakan
- **Arduino Uno/Nano** sebagai driver controller
- **Stepper Motor Driver** (Pin 10,11,12):
  - STEP_PIN (10): Pulse untuk step motor
  - DIR_PIN (11): Direction control  
  - ENABLE_PIN (12): Enable/disable motor
- **AltSoftSerial** (Pin 8,9): Komunikasi dengan ArmControl
- **Strap Pins** (Pin 3,4,5,6): Driver ID configuration
- **Limit Switch** (Pin A0): Homing reference position
- **Speed Select** (Pin A1,A2): High/normal speed selection
- **Status LED** (Pin 13): Motor status indicator

## 🏷️ Driver Identification System

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                      DRIVER ID CONFIGURATION                     ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                                                                   ║
    ║  🎛️ STRAP PINS (Physical Jumpers):                               ║
    ║                                                                   ║
    ║  ┌─────┬─────┬─────┬─────┬─────────┬─────────┬─────────────────┐  ║
    ║  │Axis │ ID  │ D3  │ D4  │   D5    │  Binary │     Function    │  ║
    ║  ├─────┼─────┼─────┼─────┼─────────┼─────────┼─────────────────┤  ║
    ║  │  X  │111  │ ●●● │ ●●● │  ●●●    │  0b111  │ X-Axis Movement │  ║
    ║  │  Y  │110  │ ○○○ │ ●●● │  ●●●    │  0b110  │ Y-Axis Movement │  ║
    ║  │  Z  │101  │ ●●● │ ○○○ │  ●●●    │  0b101  │ Z-Axis Movement │  ║
    ║  │  G  │100  │ ○○○ │ ○○○ │  ●●●    │  0b100  │ Gripper Control │  ║
    ║  │  T  │011  │ ●●● │ ●●● │  ○○○    │  0b011  │ Turret Rotation │  ║
    ║  └─────┴─────┴─────┴─────┴─────────┴─────────┴─────────────────┘  ║
    ║                                                                   ║
    ║  ●●● = Jumper CLOSED (Connected to GND)                          ║
    ║  ○○○ = Jumper OPEN (Pull-up HIGH)                                ║
    ║                                                                   ║
    ╚═══════════════════════════════════════════════════════════════════╝
```

### 🔢 Driver ID Mapping
```cpp
const uint8_t DRIVER_ID_X = 0b111;  // Strap: 3,4,5 = HIGH,HIGH,HIGH
const uint8_t DRIVER_ID_Y = 0b110;  // Strap: 3,4,5 = LOW,HIGH,HIGH  
const uint8_t DRIVER_ID_Z = 0b101;  // Strap: 3,4,5 = HIGH,LOW,HIGH
const uint8_t DRIVER_ID_G = 0b100;  // Strap: 3,4,5 = LOW,LOW,HIGH
const uint8_t DRIVER_ID_T = 0b011;  // Strap: 3,4,5 = HIGH,HIGH,LOW
```

### ⚙️ Strap Pin Configuration
```cpp
void setDriverID() {
  uint8_t strapCode = (digitalRead(STRAP_3_PIN) << 2) | 
                      (digitalRead(STRAP_2_PIN) << 1) | 
                      (digitalRead(STRAP_1_PIN));
  
  switch (strapCode) {
    case DRIVER_ID_X: driverID = 'X'; break;
    case DRIVER_ID_Y: driverID = 'Y'; break;
    case DRIVER_ID_Z: driverID = 'Z'; break;
    case DRIVER_ID_G: driverID = 'G'; break;
    case DRIVER_ID_T: driverID = 'T'; break;
  }
}
```

## 🏃 Speed Configuration System

### ⚡ Speed Selection (Pin A1-A2)
```cpp
// Detection logic: Test if A1 and A2 are connected
pinMode(SPEED_SELECT_PIN_1, OUTPUT);
digitalWrite(SPEED_SELECT_PIN_1, LOW);
bool a2_when_a1_low = digitalRead(SPEED_SELECT_PIN_2);

digitalWrite(SPEED_SELECT_PIN_1, HIGH);  
bool a2_when_a1_high = digitalRead(SPEED_SELECT_PIN_2);

if (a2_when_a1_low == LOW && a2_when_a1_high == HIGH) {
  // A1-A2 connected = HIGH SPEED
  MOVE_MAX_SPEED = MOVE_MAX_SPEED;
} else {
  // A1-A2 not connected = NORMAL SPEED  
  MOVE_MAX_SPEED = 0.5 * MOVE_MAX_SPEED;
}
```

### 🎛️ Speed Parameters per Axis
```cpp
// X Axis
MOVE_MAX_SPEED = 2500;  // Base speed
MOVE_HOME_SPEED = 300;  // Homing speed
MOVE_HOME_ACCELERATION = 0.3 * MOVE_HOME_SPEED;

// Y Axis  
MOVE_MAX_SPEED = 4000;
MOVE_HOME_SPEED = 300;
MOVE_HOME_ACCELERATION = 0.5 * MOVE_HOME_SPEED;

// Z Axis (dengan dynamic speed)
MOVE_MAX_SPEED = 4000;
MOVE_HOME_SPEED = 300; 
MOVE_HOME_ACCELERATION = 0.1 * MOVE_HOME_SPEED;

// T Axis
MOVE_MAX_SPEED = 4000;
MOVE_HOME_SPEED = 4000;  // Fast homing
MOVE_HOME_ACCELERATION = 0.5 * MOVE_HOME_SPEED;

// G Axis  
MOVE_MAX_SPEED = 4000;
MOVE_HOME_SPEED = 150;   // Slow/precise homing
MOVE_HOME_ACCELERATION = 0.5 * MOVE_HOME_SPEED;
```

## 📡 Communication Protocol

### 📥 Input Command Format
```
MOTOR_COMMAND*CHECKSUM
```
Contoh:
```
X1620,Y2205,T240*5E    // Multi-axis command
Z3975*A1               // Single axis command  
X0*B3                  // Homing command
```

### 🔐 Checksum Validation
```cpp
bool validateAndExecuteCommand(String receivedData) {
  int separatorIndex = receivedData.indexOf('*');
  String command = receivedData.substring(0, separatorIndex);
  String checksumStr = receivedData.substring(separatorIndex + 1);
  
  uint8_t receivedChecksum = hexStringToUint8(checksumStr);
  uint8_t calculatedChecksum = calculateXORChecksum(command.c_str(), command.length());
  
  if (receivedChecksum == calculatedChecksum) {
    executeCommand(command);
    return true;
  } else {
    Serial.println("Checksum mismatch - command rejected");
    return false;
  }
}
```

### 🔍 Command Parsing
```cpp
void executeCommand(String command) {
  // Parse multiple commands: "X1620,Y2205,T240"
  String singleCommand = extractSingleCommand(command);
  
  if (singleCommand.length() >= 2) {
    char targetDriverID = singleCommand.charAt(0);  // 'X', 'Y', 'Z', 'T', 'G'
    
    if (targetDriverID == driverID) {
      long targetPosition = singleCommand.substring(1).toInt();
      
      if (targetPosition == 0) {
        performHoming();  // Position 0 = homing
      } else {
        moveToPosition(targetPosition);
      }
    }
  }
}
```

## 🏠 Homing Operations

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                       HOMING SEQUENCE FLOW                       ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                                                                   ║
    ║  Start Homing                                                     ║
    ║       │                                                           ║
    ║       ▼                                                           ║
    ║  ┌─────────────────┐   YES   ┌─────────────────────────────────┐  ║
    ║  │ At Home Already?├────────►│ Move Away from Limit Switch     │  ║
    ║  │ (Limit = HIGH)  │         │ (10000 steps away)              │  ║
    ║  └─────┬───────────┘         └─────────────┬───────────────────┘  ║
    ║        │ NO                                │                     ║
    ║        ▼                                   ▼                     ║
    ║  ┌─────────────────────────────────────────┴───────────────────┐  ║
    ║  │ Return to Home (Move until Limit = HIGH)                   │  ║
    ║  │ Direction: Negative (-10000 steps max)                     │  ║
    ║  └─────────────────────┬───────────────────────────────────────┘  ║
    ║                        │                                         ║
    ║                        ▼                                         ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ Set Current Position = 0                                    │  ║
    ║  │ (Limit switch = Reference point)                           │  ║
    ║  └─────────────────────┬───────────────────────────────────────┘  ║
    ║                        │                                         ║
    ║                        ▼                                         ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ Move to Position 0 (Working position offset)               │  ║
    ║  │ Ready for normal operations                                 │  ║
    ║  └─────────────────────────────────────────────────────────────┘  ║
    ║                                                                   ║
    ╚═══════════════════════════════════════════════════════════════════╝
```

### 🔄 Homing Sequence
```cpp
void performHoming() {
  Serial.println("Starting homing sequence...");
  
  // Set speed untuk homing
  stepperMotor.setMaxSpeed(MOVE_HOME_SPEED);
  stepperMotor.setAcceleration(MOVE_HOME_ACCELERATION);
  
  if (isAtHomePosition()) {
    moveAwayFromHome();    // Keluar dari limit switch
    returnToHome();        // Kembali ke limit switch
  } else {
    returnToHome();        // Langsung ke limit switch
  }
  
  // Set position = 0 setelah homing
  stepperMotor.setCurrentPosition(0);
  
  // Return to position 0 (offset dari limit switch)
  stepperMotor.moveTo(0);
  stepperMotor.runToPosition();
}
```

### 🎯 Limit Switch Detection
```cpp
bool isAtHomePosition() {
  return digitalRead(LIMIT_SWITCH_PIN) == HIGH;  // Active HIGH limit switch
}

void moveAwayFromHome() {
  stepperMotor.move(HOMING_STEPS);  // 10000 steps away
  
  // Move sampai limit switch release
  do {
    stepperMotor.run();
  } while (digitalRead(LIMIT_SWITCH_PIN) == HIGH);
  
  stepperMotor.stop();
  stepperMotor.runToPosition();
}

void returnToHome() {
  stepperMotor.move(-HOMING_STEPS);  // 10000 steps back
  
  // Move sampai limit switch triggered  
  do {
    stepperMotor.run();
  } while (digitalRead(LIMIT_SWITCH_PIN) == LOW);
}
```

## 🎮 Motor Movement Control

### 📍 Position Movement
```cpp
void moveToPosition(long targetPosition) {
  Serial.print("Moving to position: ");
  Serial.println(targetPosition);
  
  setStatusLED(false);  // LED OFF during movement
  
  stepperMotor.setMaxSpeed(MOVE_MAX_SPEED);
  stepperMotor.setAcceleration(MOVE_ACCELERATION);
  stepperMotor.moveTo(targetPosition);
  stepperMotor.runToPosition();  // Blocking movement
  
  setStatusLED(true);   // LED ON when idle
}
```

### ⚡ Dynamic Z-Axis Speed
```cpp
if (driverID == 'Z') {
  long distance = abs(targetPosition);
  float speed = constrain(100 * sqrt(distance), 300, 3000);
  stepperMotor.setMaxSpeed(speed);
  stepperMotor.setAcceleration(0.5 * speed);
  
  Serial.print("Z-axis dynamic speed: ");
  Serial.println(speed);
}
```

### 🔄 AccelStepper Configuration
```cpp
AccelStepper stepperMotor(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIR_PIN);

// Setup
stepperMotor.setMaxSpeed(MOVE_HOME_SPEED);
stepperMotor.setAcceleration(MOVE_HOME_ACCELERATION);
stepperMotor.setCurrentPosition(0);
```

## 🚦 Status Indicators

### 💡 LED Status System
```cpp
void setStatusLED(bool state) {
  digitalWrite(LED_STATUS_PIN, state ? HIGH : LOW);
}

// Usage:
setStatusLED(true);   // Motor idle = LED ON
setStatusLED(false);  // Motor moving = LED OFF
```

### 📊 Serial Debug Output
```cpp
Serial.print("Driver ID: ");
Serial.println(driverID);
Serial.print("Target position: ");
Serial.println(targetPosition);
Serial.print("Z-axis dynamic speed: ");
Serial.println(speed);
```

## ⚙️ Motor Configuration Constants

### 📏 Movement Parameters
```cpp
const int HOMING_STEPS = 10000;     // Max steps untuk homing
const int HOMING_OFFSET = 5000;     // Offset dari limit switch
const int SERIAL_BAUD_RATE = 9600;  // Communication speed
```

### 🎛️ Pin Definitions
```cpp
// Motor control pins
const int STEPPER_ENABLE_PIN = 12;  // Enable motor (active LOW)
const int STEPPER_DIR_PIN = 11;     // Direction control
const int STEPPER_STEP_PIN = 10;    // Step pulse

// Communication pins  
const int STRAP_COMMON_PIN = 6;     // Common ground untuk strap
const int STRAP_1_PIN = 3;          // LSB bit
const int STRAP_2_PIN = 4;          // Middle bit
const int STRAP_3_PIN = 5;          // MSB bit
```

## 🔄 Program Flow

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                        PROGRAM FLOW                              ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                                                                   ║
    ║  🚀 SETUP PHASE:                                                  ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ 1. Initialize Hardware Pins                                │  ║
    ║  │    ├─ Strap pins (D3,D4,D5) for ID                        │  ║
    ║  │    ├─ Limit switch (A0)                                   │  ║
    ║  │    ├─ Motor control (D10,D11,D12)                         │  ║
    ║  │    └─ Speed select (A1,A2)                                │  ║
    ║  └─────────┬───────────────────────────────────────────────────┘  ║
    ║            ▼                                                     ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ 2. Initialize Serial Communication (9600 baud)             │  ║
    ║  └─────────┬───────────────────────────────────────────────────┘  ║
    ║            ▼                                                     ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ 3. Read Strap Configuration                                │  ║
    ║  │    ├─ Determine driver ID (X,Y,Z,T,G)                     │  ║
    ║  │    └─ Print identification result                          │  ║
    ║  └─────────┬───────────────────────────────────────────────────┘  ║
    ║            ▼                                                     ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ 4. Check Speed Selection                                   │  ║
    ║  │    ├─ Test A1-A2 connection                               │  ║
    ║  │    └─ Set high/normal speed mode                          │  ║
    ║  └─────────┬───────────────────────────────────────────────────┘  ║
    ║            ▼                                                     ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ 5. Configure Motor Parameters                              │  ║
    ║  │    ├─ Set axis-specific speeds                            │  ║
    ║  │    └─ Calculate acceleration values                        │  ║
    ║  └─────────┬───────────────────────────────────────────────────┘  ║
    ║            ▼                                                     ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ 6. Initialize AccelStepper Library                         │  ║
    ║  │    ├─ Set initial speed & acceleration                     │  ║
    ║  │    └─ Set current position = 0                            │  ║
    ║  └─────────┬───────────────────────────────────────────────────┘  ║
    ║            ▼                                                     ║
    ║  🔄 MAIN LOOP:                                                   ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ processSerialInput()                                        │  ║
    ║  │    ├─ Listen for commands from ArmControl                  │  ║
    ║  │    ├─ Validate checksum                                    │  ║
    ║  │    ├─ Parse command for this driver ID                     │  ║
    ║  │    └─ Execute movement or homing                           │  ║
    ║  └─────────────────────────────────────────────────────────────┘  ║
    ║                                                                   ║
    ╚═══════════════════════════════════════════════════════════════════╝
```

### 🚀 Setup Sequence
```
1. Initialize pins (strap, limit switch, motor control)
2. Initialize serial communication (9600 baud)
3. Read strap pins → Determine driver ID
4. Check speed selection (A1-A2 connection)
5. Set speed parameters based on driver ID
6. Initialize stepper motor with AccelStepper
7. Ready untuk menerima commands
```

### 🔄 Main Loop
```cpp
void loop() {
  processSerialInput();  // Check for incoming commands
}

void processSerialInput() {
  while (serialComm.available()) {
    char receivedChar = serialComm.read();
    
    if (receivedChar == '\n' || receivedChar == '\r') {
      validateAndExecuteCommand(commandBuffer);
      commandBuffer = "";
    } else {
      commandBuffer += receivedChar;
    }
  }
}
```

## 🔧 Command Examples

### 📍 Position Commands
```cpp
// Single axis movement
"X1620*5A" → Move X axis to position 1620
"Z3975*B1" → Move Z axis to position 3975
"G270*4C"  → Move gripper to position 270

// Multi-axis movement (hanya axis yang sesuai driver ID yang dijalankan)
"X1620,Y2205,T240*5E" → X driver akan jalankan X1620, yang lain ignore
```

### 🏠 Homing Commands
```cpp
"X0*B3" → X axis homing
"Y0*A4" → Y axis homing  
"Z0*C5" → Z axis homing
"T0*D6" → T axis homing
"G0*E7" → G axis homing
```

## ⚠️ Error Handling

### 🚨 Checksum Mismatch
```cpp
if (receivedChecksum != calculatedChecksum) {
  Serial.println("Checksum mismatch - command rejected");
  return false;
}
```

### 🔍 Invalid Commands
```cpp
if (singleCommand.length() < 2) {
  Serial.println("Invalid command format");
  return;
}
```

### 🎯 Target Driver Filtering
```cpp
if (targetDriverID != driverID) {
  Serial.println("Command not for this driver - skipping");
  return;
}
```

## 📊 Performance Characteristics

### ⚡ Speed Profiles per Axis
| Axis | Max Speed | Home Speed | Acceleration Factor |
|------|-----------|------------|-------------------|
| X    | 2500      | 300        | 0.3               |
| Y    | 4000      | 300        | 0.5               |
| Z    | 4000      | 300        | 0.1               |
| T    | 4000      | 4000       | 0.5               |
| G    | 4000      | 150        | 0.5               |

### 🔄 Z-Axis Dynamic Speed Formula
```cpp
float speed = constrain(100 * sqrt(distance), 300, 3000);

// Contoh:
distance = 1000 → speed = 100 * sqrt(1000) = 3162 → constrained to 3000
distance = 100  → speed = 100 * sqrt(100) = 1000  
distance = 9    → speed = 100 * sqrt(9) = 300     → minimum
```

## 🛠️ Troubleshooting Guide

### 🔍 Common Issues

1. **Motor tidak bergerak**:
   - Check strap pins configuration
   - Verify driver ID assignment
   - Check enable pin (should be LOW)

2. **Checksum errors**:
   - Verify baud rate (9600)
   - Check cable connections
   - Validate command format

3. **Homing problems**:
   - Check limit switch wiring (should be active HIGH)
   - Verify HOMING_STEPS value
   - Check motor direction

4. **Wrong speed**:
   - Check A1-A2 connection for speed selection  
   - Verify axis-specific speed parameters

---

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                      MOTOR DRIVER OVERVIEW                       ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                                                                   ║
    ║  📡 INPUT: "X1620,Y2205,T240*5E"                                 ║
    ║                     │                                             ║
    ║                     ▼                                             ║
    ║  🔐 CHECKSUM VALIDATION                                           ║
    ║                     │                                             ║
    ║                     ▼                                             ║
    ║  🎛️ COMMAND PARSING & FILTERING                                   ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ Multi-command: "X1620,Y2205,T240"                          │  ║
    ║  │               ↓      ↓       ↓                             │  ║
    ║  │ X Driver: ✓Execute X1620                                   │  ║
    ║  │ Y Driver: ✓Execute Y2205                                   │  ║  
    ║  │ Z Driver: ✗Skip (not for me)                               │  ║
    ║  │ T Driver: ✓Execute T240                                    │  ║
    ║  │ G Driver: ✗Skip (not for me)                               │  ║
    ║  └─────────────────────────────────────────────────────────────┘  ║
    ║                     │                                             ║
    ║                     ▼                                             ║
    ║  🎯 INDIVIDUAL MOTOR CONTROL (Example: Z-Axis Driver)             ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ Driver ID: Z (0b101)    │ 💡 LED: OFF (Moving)              │  ║
    ║  │ Current Pos: 2340       │ 🏠 Limit Switch: LOW              │  ║
    ║  │ Target Pos: 3975        │ ⚡ Speed: 2,000 steps/sec         │  ║
    ║  │ Distance: 1635          │ 🎛️ Accel: 1,000 steps/sec²       │  ║
    ║  │ Status: MOVING          │ ⏱️ ETA: 1.2 seconds               │  ║
    ║  └─────────────────────────────────────────────────────────────┘  ║
    ║                     │                                             ║
    ║                     ▼                                             ║
    ║  ⚙️ ACCELSTEPPER MOTOR CONTROL                                    ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ D12 ──► ENABLE (LOW = Motor ON)                            │  ║
    ║  │ D11 ──► DIRECTION (HIGH/LOW = CW/CCW)                      │  ║
    ║  │ D10 ──► STEP PULSE (PWM frequency control)                 │  ║
    ║  │                                                             │  ║
    ║  │ Smooth Acceleration Profile:                                │  ║
    ║  │ Speed  ▲                                                    │  ║
    ║  │   Max  ├─────────────                                      │  ║
    ║  │        │            ╲                                      │  ║
    ║  │        │             ╲                                     │  ║
    ║  │      0 └──────────────┴─────► Time                         │  ║
    ║  └─────────────────────────────────────────────────────────────┘  ║
    ║                                                                   ║
    ║  📊 REAL-TIME STATUS: Position reached → LED ON → Send ACK       ║
    ║                                                                   ║
    ╚═══════════════════════════════════════════════════════════════════╝
```

**💡 Tips**: Program ini adalah driver level terendah yang memberikan kontrol presisi untuk setiap motor axis. Setiap Arduino menjalankan instance program ini dengan driver ID yang berbeda melalui strap pin configuration. System menggunakan AccelStepper library untuk smooth acceleration dan positioning yang akurat.