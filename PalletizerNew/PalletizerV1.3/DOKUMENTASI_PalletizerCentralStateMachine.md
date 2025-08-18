# Dokumentasi PalletizerCentralStateMachine.ino

```
     ╔═══════════════════════════════════════════════════════════════╗
     ║                 PALLETIZER CENTRAL STATE MACHINE             ║
     ║                        (Master Controller)                   ║
     ╠═══════════════════════════════════════════════════════════════╣
     ║  🎮 ARM Coordination  │  📊 Sequence Management              ║
     ║  📡 RS485 Communication │  💾 Parameter Storage              ║
     ║  🔄 State Machine      │  🚦 Sensor Monitoring              ║
     ╚═══════════════════════════════════════════════════════════════╝
```

## 📋 Deskripsi Program
**PalletizerCentralStateMachine** adalah program utama yang mengatur koordinasi dua robot ARM (ARM1 dan ARM2) untuk sistem palletizing otomatis. Program ini bertindak sebagai "otak" yang menentukan kapan dan ARM mana yang harus bergerak.

## 🎯 Fungsi Utama
- **Koordinasi ARM**: Mengatur giliran ARM1 dan ARM2 untuk mengambil produk
- **Sensor Monitoring**: Memantau 3 sensor untuk deteksi produk dan posisi ARM
- **Command Generation**: Membuat perintah HOME dan GLAD untuk setiap ARM
- **State Management**: Mengelola state machine untuk setiap ARM
- **Parameter Management**: Menyimpan dan mengelola parameter koordinat di EEPROM

## 🔧 Hardware Yang Digunakan

```
    ┌─────────────────────────────────────────────────────────────────┐
    │                    CENTRAL STATE MACHINE                       │
    │                      Arduino Uno/Nano                         │
    │                                                                 │
    │  A4 ◄─── Sensor1 (Product Detection)                          │
    │  A5 ◄─── Sensor2 (Product Detection)                          │
    │  D2 ◄─── Sensor3 (ARM Position)                               │
    │                                                                 │
    │  D7 ◄─── ARM1 Status (HIGH=Busy)                              │
    │  D8 ◄─── ARM2 Status (HIGH=Busy)                              │
    │                                                                 │
    │  D13 ───► Conveyor Control (LOW=ON)                           │
    │                                                                 │
    │  D10,D11 ◄─► RS485 Module                                     │
    │                │                                               │
    │                ▼                                               │
    │        ┌─────────────────┐                                     │
    │        │  ARM Controllers │                                     │
    │        │  (ARML & ARMR)  │                                     │
    │        └─────────────────┘                                     │
    │                                                                 │
    │  DIP Switches: D3,D4,D5,D6 (ARM1) & A0,A1,A2,A3 (ARM2)       │
    └─────────────────────────────────────────────────────────────────┘
```

- **Arduino Uno/Nano** sebagai controller utama
- **RS485 Module** untuk komunikasi dengan ARM Controller
- **3 Sensor**:
  - Sensor1 (A4): Deteksi produk masuk area 1
  - Sensor2 (A5): Deteksi produk masuk area 2  
  - Sensor3 (D2): Deteksi ARM di posisi center (LOW = ARM ada)
- **2 ARM Status Pin**:
  - ARM1_PIN (D7): Status busy ARM1 (HIGH = busy)
  - ARM2_PIN (D8): Status busy ARM2 (HIGH = busy)
- **DIP Switch**: Untuk setting layer awal ARM1 dan ARM2
- **Conveyor Control** (D13): Mengontrol conveyor belt

## 📊 Konsep Sequence dan Layer

### 🔢 Sistem Penomoran Sequence

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                        PALLETIZING LAYERS                        ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║  Layer 2 (Ganjil)    │ 17│ 18│ 19│ 20│ 21│ 22│ 23│ 24│          ║
    ║  [XO coordinates]     │   │   │   │   │   │   │   │   │          ║
    ║                       └───┴───┴───┴───┴───┴───┴───┴───┘          ║
    ║                                                                   ║
    ║  Layer 1 (Genap)      │ 9 │ 10│ 11│ 12│ 13│ 14│ 15│ 16│          ║
    ║  [XE coordinates]     │   │   │   │   │   │   │   │   │          ║
    ║                       └───┴───┴───┴───┴───┴───┴───┴───┘          ║
    ║                                                                   ║
    ║  Layer 0 (Ganjil)     │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │          ║
    ║  [XO coordinates]     │   │   │   │   │   │   │   │   │          ║
    ║                       └───┴───┴───┴───┴───┴───┴───┴───┘          ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                     Task: 0   1   2   3   4   5   6   7          ║
    ║                  Position: 1   2   3   4   5   6   7   8          ║
    ╚═══════════════════════════════════════════════════════════════════╝
    
    Sequence 5 = Layer 0, Task 2, Position 3 (menggunakan XO3/YO3)
```

```
Layer 0 (Ganjil):  Sequence 1,  2,  3,  4,  5,  6,  7,  8  (Task 0-7)
Layer 1 (Genap):   Sequence 9,  10, 11, 12, 13, 14, 15, 16 (Task 0-7)
Layer 2 (Ganjil):  Sequence 17, 18, 19, 20, 21, 22, 23, 24 (Task 0-7)
...dan seterusnya
```

### 📦 Struktur Command Index
```cpp
commandIndex = (layer * 16) + (task * 2) + (isGLAD ? 1 : 0)

Contoh Sequence 5:
- commandPair = 5 ÷ 2 = 2 (pair ke-2)
- layer = 2 ÷ 8 = 0 (layer pertama)
- task = 2 % 8 = 2 (task ke-2 dalam layer)
- commandIndex 4 = HOME command
- commandIndex 5 = GLAD command (inilah sequence 5)
```

## 🎮 State Machine ARM

### 🔄 State ARM

```
    ┌─────────────────────────────────────────────────────────────────┐
    │                     ARM STATE MACHINE                          │
    │                                                                 │
    │  ┌─────────────┐   HOME    ┌─────────────────┐   Product       │
    │  │             │ Command   │                 │   Detected      │
    │  │  ARM_IDLE   ├──────────►│ ARM_MOVING_TO   ├─────────────┐   │
    │  │             │           │    CENTER       │             │   │
    │  └─────┬───────┘           └─────────────────┘             ▼   │
    │        ▲                                                   │   │
    │        │                   ┌─────────────────┐             │   │
    │        │    Complete       │                 │ Sensor3=LOW │   │
    │        │   ◄───────────────┤  ARM_IN_CENTER  │◄────────────┘   │
    │        │                   │                 │                 │
    │        │                   └─────────┬───────┘                 │
    │        │                             │ GLAD                    │
    │        │                             │ Command                 │
    │        │                             ▼                         │
    │   ┌────┴─────┐                 ┌─────────────────┐             │
    │   │          │    Complete     │                 │             │
    │   │ARM_ERROR │◄────────────────┤  ARM_PICKING    │             │
    │   │          │    Timeout      │                 │             │
    │   └──────────┘                 └─────────────────┘             │
    │                                                                 │
    │                 ┌─────────────────┐                            │
    │                 │ARM_EXECUTING    │                            │
    │                 │   _SPECIAL      │                            │
    │                 │ (PARK/CALI)     │                            │
    │                 └─────────────────┘                            │
    └─────────────────────────────────────────────────────────────────┘
```

```cpp
enum ArmState {
  ARM_IDLE,               // ARM siap menerima command baru
  ARM_MOVING_TO_CENTER,   // ARM sedang bergerak ke center (HOME)
  ARM_IN_CENTER,          // ARM sudah di center, menunggu produk
  ARM_PICKING,            // ARM sedang mengambil produk (GLAD)
  ARM_EXECUTING_SPECIAL,  // ARM menjalankan PARK/CALIBRATION
  ARM_ERROR               // ARM error, perlu intervensi manual
}
```

### ⏱️ Timeout Protection
- **MOVE_TIMEOUT**: 15 detik untuk movement
- **PICK_TIMEOUT**: 15 detik untuk picking operation
- **Auto-recovery**: 30 detik dari ERROR state

## 📐 Sistem Koordinat dan Parameter

### 🏠 HOME Command
Membawa ARM ke posisi home untuk siap pickup:
```cpp
HOME(homeX, homeY, homeZ, homeT, homeG)
```

### 🤏 GLAD Command  
Melakukan pickup produk dari conveyor:
```cpp
GLAD(gladXn, gladYn, gladZn, gladTn, gladDp, gladGp, gladZa, gladZb, gladXa, gladTa)
```

### 📍 Parameter Koordinat
**Layer Ganjil (0, 2, 4, ...)** menggunakan koordinat **XO/YO**:
- XO1, YO1 = Posisi 1 (task 0)
- XO2, YO2 = Posisi 2 (task 1)
- ...sampai XO8, YO8 = Posisi 8 (task 7)

**Layer Genap (1, 3, 5, ...)** menggunakan koordinat **XE/YE**:
- XE1, YE1 = Posisi 1 (task 0)
- XE2, YE2 = Posisi 2 (task 1)
- ...sampai XE8, YE8 = Posisi 8 (task 7)

### 🎯 Y Pattern System
```cpp
params.y_pattern[0] = 2;  // Task 0 uses y2 (pickup height 2)
params.y_pattern[1] = 1;  // Task 1 uses y1 (pickup height 1)
params.y_pattern[2] = 2;  // Task 2 uses y2
params.y_pattern[3] = 1;  // Task 3 uses y1
params.y_pattern[4] = 1;  // Task 4 uses y1
params.y_pattern[5] = 1;  // Task 5 uses y1 ← Sequence 5 menggunakan y1
params.y_pattern[6] = 1;  // Task 6 uses y1
params.y_pattern[7] = 1;  // Task 7 uses y1
```

## 🔄 Flow Operasi Sistem

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                       SISTEM FLOW OPERASI                        ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                                                                   ║
    ║  📦 Product ────► [Sensor1,2,3] ────► 🎯 ARM Selection            ║
    ║   Detected             │                      │                   ║
    ║                        ▼                      ▼                   ║
    ║                 🚦 Sensor Logic       🤖 Generate Command         ║
    ║                        │                      │                   ║
    ║                        ▼                      ▼                   ║
    ║              ⚡ State Update ────────► 📡 RS485 Transmission      ║
    ║                        │                      │                   ║
    ║                        ▼                      ▼                   ║
    ║               🔄 Monitor Progress ◄─── 📨 Command Response         ║
    ║                                                                   ║
    ╚═══════════════════════════════════════════════════════════════════╝
    
    Detailed Flow:
    
    🏭 Conveyor ──┐
                  ├─► 📍 Sensor1 & Sensor2 (Product Detection)
    🤖 ARM1 ──────┤
    🤖 ARM2 ──────┤
                  └─► 📍 Sensor3 (ARM Position)
    
    📊 Decision Logic:
    ┌─────────────────────────────────────────────────────────────┐
    │ if (sensors_detect_product && arm_available) {             │
    │    selected_arm = choose_arm();                            │
    │    command = generate_command(selected_arm);               │
    │    send_rs485_command(command);                            │
    │    update_arm_state(MOVING_TO_CENTER);                     │
    │ }                                                          │
    └─────────────────────────────────────────────────────────────┘
```

### 1️⃣ **Sensor Detection**
```
Sensor1 & Sensor2 & Sensor3 HIGH → Ada produk di conveyor
```

### 2️⃣ **ARM Selection** 
```cpp
if (arm1_ready && arm2_ready) {
    // Alternating selection
    selectedArm = last_arm_sent ? 2 : 1;
} else if (arm1_ready) {
    selectedArm = 1;
} else if (arm2_ready) {
    selectedArm = 2;
}
```

### 3️⃣ **Command Generation**
```cpp
String command = generateCommand(selectedArm, currentPosition);
// Contoh: "HOME(3870,390,3840,240,-30)" atau "GLAD(1620,2205,3975,240,60,270,750,3960,2340,240)"
```

### 4️⃣ **Command Transmission**
```cpp
String fullCommand = armPrefix + "#" + command;
// Contoh: "ARML#HOME(3870,390,3840,240,-30)" atau "ARMR#GLAD(...)"
sendRS485CommandWithRetry(arm, fullCommand);
```

### 5️⃣ **State Monitoring**
Program memantau status ARM melalui pin hardware dan mengubah state sesuai dengan progress operasi.

## 📡 Protokol Komunikasi

### 📤 Format Command
```
ARMx#COMMAND*CHECKSUM
```
Contoh:
```
ARML#HOME(3870,390,3840,240,-30)*5A
ARMR#GLAD(1620,2205,3975,240,60,270,750,3960,2340,240)*B7
```

### 🔐 Checksum Validation
```cpp
uint8_t checksum = calculateXORChecksum(command.c_str(), command.length());
String fullCommand = command + "*" + String(checksum, HEX);
```

## ⚙️ Koordinasi Non-Blocking

### 🚫 Leave Center Delay
```cpp
// Delay 500ms setelah ARM keluar dari center sebelum ARM berikutnya bisa masuk
const int LEAVE_CENTER_DELAY = 500;

// Non-blocking implementation:
if (leave_center_delay_active) {
    if (millis() - leave_center_timer >= LEAVE_CENTER_DELAY) {
        leave_center_delay_active = false;
        // Sekarang ARM lain boleh masuk ke center
    }
}
```

### 🔄 ARM Alternating Logic
```cpp
// Prioritas 1: Special commands (PARK/CALI)
// Prioritas 2: ARM yang butuh HOME command
// Prioritas 3: Alternating selection jika kedua ARM ready
```

## 💾 Parameter Storage (EEPROM)

### 📂 Default Parameters
Program memiliki default parameter lengkap yang disimpan dalam fungsi `resetParametersToDefault()`:
- **Global params**: x, y1, y2, z, t, g, gp, dp, za, zb, H, Ly, T90, Z1
- **Koordinat ODD**: XO1-XO8, YO1-YO8  
- **Koordinat EVEN**: XE1-XE8, YE1-YE8
- **ARM Offsets**: xL, yL, zL, tL, gL (LEFT), xR, yR, zR, tR, gR (RIGHT)
- **Y Pattern**: Array untuk menentukan pickup height per task

### 💿 EEPROM Management
```cpp
// Save to EEPROM
saveParametersToEEPROM();

// Load from EEPROM (dengan checksum validation)
if (!loadParametersFromEEPROM()) {
    resetParametersToDefault();
}
```

## 🛠️ Special Commands

### 🏠 PARK Command
Mengembalikan ARM ke posisi parkir (semua axis ke posisi 0):
```cpp
if (arm->current_pos >= arm->total_commands) {
    arm->pending_special_command = SPECIAL_PARK;
    arm->need_special_command = true;
}
```

### ⚙️ CALIBRATION Command
Dilakukan setiap selesai layer genap untuk re-homing:
```cpp
if (is_even_layer_number && layer_complete) {
    arm->pending_special_command = SPECIAL_CALI;
    arm->need_special_command = true;
}
```

## 🔄 Retry Mechanism

### 📡 Command Retry
```cpp
const int MAX_RETRY_COUNT = 7;
const unsigned long RETRY_DELAY = 200;  // 200ms between retries
const unsigned long BUSY_RESPONSE_TIMEOUT = 500;  // 0.5 second timeout
```

### ⚠️ Error Handling
Jika ARM tidak response setelah 7x retry:
```cpp
changeArmState(arm, ARM_ERROR);
// Auto recovery setelah 30 detik
```

## 🎛️ DIP Switch Configuration

### 📟 ARM1 Layer Setting (Digital pins 3,4,5,6)
### 📟 ARM2 Layer Setting (Analog pins A0,A1,A2,A3)

DIP switch menentukan layer awal masing-masing ARM (0-9).

## 🎮 Conveyor Control

### ⏹️ Conveyor Stop
Conveyor dimatikan selama 3 detik setelah GLAD command dikirim:
```cpp
const unsigned long CONVEYOR_OFF_DURATION = 3000;  // 3 seconds
```

## 📊 Monitoring & Debug

### 🖥️ Serial Monitor Output
Program menyediakan debug output untuk:
- State transitions
- Command generation
- Sensor readings
- Error conditions
- Parameter calculations

### 📈 System Status
Monitor real-time status ARM1 dan ARM2 beserta current position dan state mereka.

---

```
    ╔═══════════════════════════════════════════════════════════════════╗
    ║                     SISTEM OVERVIEW DIAGRAM                      ║
    ╠═══════════════════════════════════════════════════════════════════╣
    ║                                                                   ║
    ║  🏭 CONVEYOR BELT                                                 ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ 📦 ────► 📦 ────► 📦 ────► [📍S1📍S2📍S3] ────► 📦 ────► │  ║
    ║  └─────────────────────────────────────────────────────────────┘  ║
    ║                              │                                   ║
    ║                              ▼                                   ║
    ║  🧠 CENTRAL STATE MACHINE (Arduino Uno)                          ║
    ║  ┌─────────────────────────────────────────────────────────────┐  ║
    ║  │ • Sensor monitoring & product detection                    │  ║
    ║  │ • ARM coordination & sequence management                   │  ║
    ║  │ • Parameter calculation & command generation               │  ║
    ║  │ • DIP switch layer configuration                           │  ║
    ║  │ • EEPROM parameter storage                                 │  ║
    ║  └─────────────────┬───────────────────┬───────────────────────┘  ║
    ║                    │                   │                         ║
    ║                    ▼                   ▼                         ║
    ║  🤖 ARM1 (LEFT)                    🤖 ARM2 (RIGHT)               ║
    ║  State: IDLE                       State: PICKING                ║
    ║  Sequence: 23                      Sequence: 47                  ║
    ║  Layer: 1                          Layer: 2                      ║
    ║                                                                   ║
    ║  📊 REAL-TIME STATUS:                                             ║
    ║  ├─ Active ARM: ARM2                                              ║
    ║  ├─ Products processed: 47                                        ║
    ║  ├─ Current layer: Layer 2 (Even - XE coordinates)               ║
    ║  ├─ Conveyor: OFF (3 sec delay)                                   ║
    ║  └─ System: RUNNING                                               ║
    ║                                                                   ║
    ╚═══════════════════════════════════════════════════════════════════╝
```

**💡 Tips**: Program ini menggunakan state machine yang robust dengan timeout protection, retry mechanism, dan error recovery untuk memastikan operasi yang reliable dalam environment industri.