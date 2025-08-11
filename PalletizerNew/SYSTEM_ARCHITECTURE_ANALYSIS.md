# Palletizer System Architecture Analysis

## Pemahaman Arsitektur yang Benar

Berdasarkan analisis mendalam kode, arsitektur sistem Palletizer adalah sebagai berikut:

### **❌ PEMAHAMAN SALAH (Yang Ditanyakan)**
```
PalletizerCentralStateMachine 
    └── PalletizerArmDriver(1) → 5 PalletizerArmControl
    └── PalletizerArmDriver(2) → 5 PalletizerArmControl
```

### **✅ ARSITEKTUR SEBENARNYA**
```
PalletizerCentralStateMachine (1 unit - MASTER)
    ├── RS485 Communication (Multi-drop)
    │   ├── PalletizerArmControl(ARM1) → AltSoftSerial → 5x PalletizerArmDriver (X,Y,Z,G,T)
    │   └── PalletizerArmControl(ARM2) → AltSoftSerial → 5x PalletizerArmDriver (X,Y,Z,G,T)
    └── Hardware I/O
        ├── Sensors (3x product detection + 2x ARM busy status)
        ├── Conveyor Control
        └── DIP Switches (layer configuration)
```

## Detail Komponen dan Komunikasi

### **1. PalletizerCentralStateMachine (Master Controller)**
- **Fungsi:** Koordinator utama sistem dual-arm palletizer
- **Hardware:** Arduino dengan RS485 interface
- **Tanggung Jawab:**
  - State machine management untuk 2 ARM
  - Product detection dan koordinasi pickup
  - Command generation (HOME, GLAD, PARK, CALI)
  - Sensor monitoring dan conveyor control
  - Parameter management (EEPROM storage)

**Komunikasi Output:**
```cpp
// Command format: ARML#HOME(x,y,z,t,g)*CHECKSUM atau ARMR#GLAD(...)*CHECKSUM
String armPrefix = (selectedArm == 1) ? "ARML" : "ARMR";
String fullCommand = armPrefix + "#" + command;
sendRS485CommandWithRetry(currentArm, fullCommand);
```

### **2. PalletizerArmControl (ARM Controllers - 2 units)**
- **Fungsi:** High-level command processor untuk setiap ARM
- **Hardware:** Arduino dengan RS485 + AltSoftSerial interfaces
- **Tanggung Jawab:**
  - Parse high-level commands (HOME, GLAD, PARK, CALI)
  - Break down ke multi-step sequences
  - Coordinate 5 motor drivers per ARM
  - Button control (START/STOP dengan 4-second hold)
  - LED status indication dan buzzer control

**Device Identification:**
```cpp
// ARM1: A4 pin FLOATING (HIGH) → ARML commands
// ARM2: A4 connected to A5 (LOW) → ARMR commands
bool isARM2_device = digitalRead(A4) == LOW;
const char* devicePrefix = isARM2_device ? "ARMR" : "ARML";
```

**Command Processing:**
```cpp
// Parse: ARML#HOME(1290,130,1280,80,-10)*CHECKSUM
if (strncmp(command, devicePrefix, 4) == 0) {
    const char* separator = strchr(command, '#');
    executeCommand(separator + 1); // Execute "HOME(1290,130,1280,80,-10)"
}
```

**Sequence Breakdown:**
- **HOME:** 2 steps (XY+T+G → Z)
- **GLAD:** 8 steps (Zb → G → Z → XYT → Zn → G → Z → XT)
- **PARK:** 2 steps (Z0 → X0,Y0,T0,G0)

### **3. PalletizerArmDriver (Motor Drivers - 10 units total)**
- **Fungsi:** Individual stepper motor controller
- **Hardware:** Arduino dengan AccelStepper + AltSoftSerial
- **Tanggung Jawab:**
  - Control single stepper motor (X, Y, Z, G, atau T)
  - Homing dengan limit switches
  - Dynamic speed calculation
  - Position tracking

**Driver Identification (Strap Pins):**
```cpp
// Hardware strap configuration menentukan driver ID:
const uint8_t DRIVER_ID_X = 0b111; // Pin 3,4,5 = HIGH,HIGH,HIGH
const uint8_t DRIVER_ID_Y = 0b110; // Pin 3,4,5 = HIGH,HIGH,LOW  
const uint8_t DRIVER_ID_Z = 0b101; // Pin 3,4,5 = HIGH,LOW,HIGH
const uint8_t DRIVER_ID_G = 0b100; // Pin 3,4,5 = HIGH,LOW,LOW
const uint8_t DRIVER_ID_T = 0b011; // Pin 3,4,5 = LOW,HIGH,HIGH
```

**Command Parsing:**
```cpp
// Parse multi-axis commands: "X3870,Y390,T240,G-30*CHECKSUM"
// Hanya execute command untuk driver yang sesuai:
if (targetDriverID == driverID) {
    long targetPosition = singleCommand.substring(1).toInt();
    if (targetPosition == 0) {
        performHoming();
    } else {
        moveToPosition(targetPosition);
    }
}
```

## Flow Komunikasi Lengkap

### **Scenario: ARM1 Pickup Product**

1. **Central State Machine:**
```cpp
// Generate command
String command = "HOME(3870,390,3840,240,-30)"; // Generated on-demand
String fullCommand = "ARML#" + command;
sendRS485Command(fullCommand + "*" + checksum);
```

2. **ARM1 Control receives:**
```
"ARML#HOME(3870,390,3840,240,-30)*7F"
```

3. **ARM1 Control breaks down ke 2 steps:**
```cpp
// Step 1:
sendMotorCommand("X3870,Y390,T240,G-30*CHECKSUM");

// Step 2 (after step 1 complete):
sendMotorCommand("Z3840*CHECKSUM");
```

4. **5 Drivers receive parallel commands:**
```
X Driver: "X3870,Y390,T240,G-30*CHECKSUM" → Execute X3870
Y Driver: "X3870,Y390,T240,G-30*CHECKSUM" → Execute Y390  
Z Driver: "Z3840*CHECKSUM" → Execute Z3840
G Driver: "X3870,Y390,T240,G-30*CHECKSUM" → Execute G-30
T Driver: "X3870,Y390,T240,G-30*CHECKSUM" → Execute T240
```

### **Communication Protocols**

1. **RS485 (Central ↔ ArmControl):**
   - Baud: 9600
   - Format: `ARML#COMMAND*CHECKSUM\n`
   - XOR checksum validation
   - Retry mechanism (max 7x)

2. **AltSoftSerial (ArmControl ↔ ArmDrivers):**
   - Baud: 9600  
   - Format: `COMMAND*CHECKSUM\n`
   - Multi-driver broadcasting
   - Driver ID filtering

### **Hardware Configuration Summary**

**Total Hardware:**
- **1x Central Controller** (Arduino + RS485)
- **2x ARM Controllers** (Arduino + RS485 + AltSoftSerial)
- **10x Motor Drivers** (5 per ARM: X,Y,Z,G,T)
- **Sensors:** 3x product detection, 2x ARM busy status
- **Peripherals:** Conveyor, LEDs, buzzer, buttons

**Physical Layout:**
```
Central Controller
    ├── RS485 Bus (shared)
    │   ├── ARM1 Controller (ARML) ── AltSoftSerial ── [X1,Y1,Z1,G1,T1 Drivers]
    │   └── ARM2 Controller (ARMR) ── AltSoftSerial ── [X2,Y2,Z2,G2,T2 Drivers]
    ├── Sensor Inputs (5x)
    ├── Conveyor Output
    └── DIP Switch Inputs (8x)
```

## Kesimpulan

Arsitektur yang benar adalah **hierarchical dengan 3 layer komunikasi**, bukan flat structure seperti yang ditanyakan. Central controller mengirim high-level commands ke ARM controllers, yang kemudian mem-breakdown menjadi motor-specific commands untuk individual drivers.

---

**Created:** 2025-08-11  
**Author:** Claude Code Analysis  
**Status:** Architecture Documentation Complete