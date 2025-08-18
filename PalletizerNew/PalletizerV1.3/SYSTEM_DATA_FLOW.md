# PalletizerV1.3 System Data Flow Documentation

## System Overview with ASCII Art

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          PALLETIZER V1.3 SYSTEM ARCHITECTURE                   │
│                                                                                 │
│  ┌─────────────────┐    RS485     ┌─────────────────┐    AltSoft  ┌──────────┐  │
│  │                 │   9600bps    │                 │    Serial   │          │  │
│  │   CENTRAL       │◄────────────►│   ARM CONTROL   │◄───────────►│  DRIVER  │  │
│  │  STATE MACHINE  │              │   (ARM1/ARM2)   │   9600bps   │ (X,Y,Z,T,G) │
│  │                 │              │                 │             │          │  │
│  └─────────────────┘              └─────────────────┘             └──────────┘  │
│          │                                │                                      │
│          │ Sensors/DIP                    │ Buttons/LEDs                        │
│          ▼                                ▼                                      │
│   ┌─────────────┐                  ┌─────────────┐                              │
│   │ S1,S2,S3    │                  │ START/STOP  │                              │
│   │ ARM1,ARM2   │                  │ LEDs/Buzzer │                              │
│   │ DIP1,DIP2   │                  │             │                              │
│   └─────────────┘                  └─────────────┘                              │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Layer 1: Central State Machine Data Flow

### Initial System Data
```
STARTUP DATA INITIALIZATION:
┌─────────────────────────────────────────┐
│ 1. EEPROM Parameters (56 total)         │
│    ┌─ Global (14): x,y1,y2,z,t,g...    │
│    ├─ Positions (32): XO1-XO8,YO1-YO8..│
│    └─ Offsets (10): xL,yL,zL,xR,yR,zR..│
│                                         │
│ 2. DIP Switch Reading                   │
│    ┌─ ARM1: D5,D6,D3,D4 → start_layer  │
│    └─ ARM2: A0,A1,A3,A2 → start_layer  │
│                                         │
│ 3. ARM State Machine Init               │
│    ┌─ ARM1_SM: state=IDLE, pos=0       │
│    └─ ARM2_SM: state=IDLE, pos=0       │
└─────────────────────────────────────────┘
```

### Main Loop Data Flow
```
MAIN LOOP CYCLE (every ~10ms):
┌────────────────────────────────────────────────────────────────┐
│                                                                │
│  ┌─────────────────┐    30ms      ┌─────────────────────────┐   │
│  │   Read Sensors  │─ interval ──►│ sensor1_state = S1_PIN  │   │
│  │                 │              │ sensor2_state = S2_PIN  │   │
│  │                 │              │ sensor3_state = S3_PIN  │   │
│  │                 │              │ arm1_response = ARM1_PIN │   │
│  │                 │              │ arm2_response = ARM2_PIN │   │
│  └─────────────────┘              └─────────────────────────┘   │
│           │                                    │                │
│           ▼                                    ▼                │
│  ┌─────────────────┐              ┌─────────────────────────┐   │
│  │ Update ARM1_SM  │              │ Update ARM2_SM          │   │
│  │ State Machine   │              │ State Machine           │   │
│  └─────────────────┘              └─────────────────────────┘   │
│           │                                    │                │
│           └──────────────┬─────────────────────┘                │
│                          ▼                                      │
│                ┌─────────────────┐                              │
│                │ System Logic    │                              │
│                │ handleSystemLogicStateMachine()                │
│                └─────────────────┘                              │
│                          │                                      │
│                          ▼                                      │
│                ┌─────────────────┐                              │
│                │ Command         │                              │
│                │ Generation &    │                              │
│                │ RS485 Send      │                              │
│                └─────────────────┘                              │
└────────────────────────────────────────────────────────────────┘
```

### Command Generation Process
```
DYNAMIC COMMAND GENERATION:
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│ Input: ARM_ID + command_index                                       │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Calculate       │              │ commandPair = index / 2     │    │
│ │ Layer & Task    │─────────────►│ layer = commandPair / 8     │    │
│ │                 │              │ task = commandPair % 8      │    │
│ │                 │              │ isHome = (index % 2 == 0)   │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Select          │              │ if (armId == 1):            │    │
│ │ ARM Offset      │─────────────►│   xOffset = params.xL       │    │
│ │                 │              │ else:                       │    │
│ │                 │              │   xOffset = params.xR       │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐                                                 │
│ │ Generate        │  HOME: "H(x,y,z,t,g)"                          │
│ │ Command String  │─ GLAD: "G(xn,yn,zn,tn,dp,gp,za,zb,xa,ta)"     │
│ │                 │  PARK: "P"                                      │
│ │                 │  CALI: "C"                                      │
│ └─────────────────┘                                                 │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Add ARM Prefix  │              │ "L#" + command (ARM1)       │    │
│ │ & Send RS485    │─────────────►│ "R#" + command (ARM2)       │    │
│ │                 │              │ + XOR checksum              │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### State Machine Transitions
```
ARM STATE MACHINE DATA FLOW:
┌────────────────────────────────────────────────────────────────────┐
│                                                                    │
│    ARM_IDLE                                                        │
│       │                                                            │
│       │ Condition: sensor3==HIGH && arm_in_center==0               │
│       ▼                                                            │
│ ┌─────────────────┐           ┌─────────────────────────────┐       │
│ │ Generate HOME   │           │ Command: "L#H(x,y,z,t,g)"  │       │
│ │ Command         │──────────►│ Send via RS485              │       │
│ │                 │           │ arm_in_center = ARM_ID      │       │
│ └─────────────────┘           └─────────────────────────────┘       │
│       │                                                            │
│       ▼                                                            │
│ ARM_MOVING_TO_CENTER                                               │
│       │                                                            │
│       │ Condition: !sensor3 && !arm.is_busy                       │
│       ▼                                                            │
│ ARM_IN_CENTER                                                      │
│       │                                                            │
│       │ Condition: !sensor1 && !sensor2 && !sensor3               │
│       ▼                                                            │
│ ┌─────────────────┐           ┌─────────────────────────────┐       │
│ │ Generate GLAD   │           │ Command: "L#G(...)"         │       │
│ │ Command         │──────────►│ Send via RS485              │       │
│ │                 │           │ turnOffConveyor()           │       │
│ └─────────────────┘           └─────────────────────────────┘       │
│       │                                                            │
│       ▼                                                            │
│ ARM_PICKING                                                        │
│       │                                                            │
│       │ Condition: was_busy && now_not_busy                        │
│       ▼                                                            │
│ ┌─────────────────┐                                                │
│ │ Check Complete  │ ─ Even Layer Complete ──► ARM_EXECUTING_SPECIAL │
│ │ Position        │ ─ All Complete ──────────► ARM_EXECUTING_SPECIAL │
│ │                 │ ─ Continue ──────────────► ARM_IDLE             │
│ └─────────────────┘                                                │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

## Layer 2: ARM Control Data Flow

### Device Identification Process
```
DEVICE DETECTION ON STARTUP:
┌─────────────────────────────────────────┐
│                                         │
│ A5 = OUTPUT LOW (ground reference)      │
│ A4 = INPUT_PULLUP                       │
│   │                                     │
│   ▼                                     │
│ ┌─────────────────┐                     │
│ │ if (A4 == LOW): │ ──► ARM2 detected   │
│ │   isARM2 = true │                     │
│ │ else:           │ ──► ARM1 detected   │
│ │   isARM2 = false│                     │
│ └─────────────────┘                     │
│   │                                     │
│   ▼                                     │
│ Set devicePrefix:                       │
│   ARM1: prefix = "L"                    │
│   ARM2: prefix = "R"                    │
│                                         │
└─────────────────────────────────────────┘
```

### RS485 Command Reception Flow
```
RS485 COMMAND PROCESSING:
┌────────────────────────────────────────────────────────────────────┐
│                                                                    │
│ RS485 Serial Input Buffer                                          │
│   │                                                                │
│   ▼                                                                │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Receive Chars   │              │ commandBuffer[64]           │    │
│ │ until \n or \r  │─────────────►│ Build command string        │    │
│ │                 │              │                             │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                │
│   ▼                                                                │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Parse & Validate│              │ Find '*' separator          │    │
│ │ CRC Checksum    │─────────────►│ Extract command & checksum  │    │
│ │                 │              │ Validate XOR checksum       │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                │
│   ▼                                                                │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Check Device    │              │ if (prefix == devicePrefix) │    │
│ │ Prefix Match    │─────────────►│   Process command           │    │
│ │                 │              │ else: ignore                │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

### Command Sequence Execution
```
COMMAND SEQUENCE BREAKDOWN:
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                                 │
│ HOME COMMAND: "L#H(3870,390,3840,240,-30)"                                     │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐    Parse     ┌─────────────────────────────────────────┐    │
│ │ parseHomeCommand│─────────────►│ homeCmd.x = 3870                       │    │
│ │                 │              │ homeCmd.y = 390                        │    │
│ │                 │              │ homeCmd.z = 3840                       │    │
│ │                 │              │ homeCmd.t = 240                        │    │
│ │                 │              │ homeCmd.g = -30                        │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ HOME Sequence   │   Step 1     │ Send: "X3870,Y390,T240,G-30*checksum"  │    │
│ │ executeHomeStep │─────────────►│ Wait for motor response                 │    │
│ │                 │   Step 2     │ Send: "Z3840*checksum"                  │    │
│ │                 │──────────────►│ Wait for motor response                 │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
│ GLAD COMMAND: "L#G(1620,2205,3975,240,60,270,750,3960,2340,240)"              │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ GLAD Sequence   │   Step 1     │ Send: "Z3960*checksum" (zb)             │    │
│ │ executeGladStep │─────────────►│ Step 2: "G270*checksum" (gp)            │    │
│ │ (8 steps total) │   Step 3     │ Send: "Z3225*checksum" (zn-za)          │    │
│ │                 │──────────────►│ Step 4: "X1620,Y2205,T240*checksum"    │    │
│ │                 │   ...        │ Step 5: "Z3975*checksum" (zn)           │    │
│ │                 │              │ Step 6: "G60*checksum" (dp)             │    │
│ │                 │              │ Step 7: "Z3225*checksum" (zn-za)        │    │
│ │                 │              │ Step 8: "X2340,T240*checksum" (xa,ta)   │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### State Machine Data Flow
```
ARM CONTROL STATE TRANSITIONS:
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                                 │
│ STATE_ZEROING                                                                   │
│   │ Data: isParkSequenceActive=true, parkSequenceStep=1                        │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ PARK Sequence   │   Step 1     │ Send: "Z0*checksum" + 2s delay          │    │
│ │                 │─────────────►│ Step 2: "X0,T0,G0*checksum" + 2s delay │    │
│ │                 │   Step 3     │ Send: "Y0*checksum"                     │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │ When complete: isMotorReady() == true                                       │
│   ▼                                                                             │
│ STATE_SLEEPING                                                                  │
│   │ Data: RED LED ON, USB commands enabled                                     │
│   │ Condition: isStartButtonPressed() (4-second hold)                          │
│   ▼                                                                             │
│ STATE_READY                                                                     │
│   │ Data: RED+YELLOW LEDs, COMMAND_ACTIVE_PIN = LOW                            │
│   │ Accepts: HOME commands from RS485                                          │
│   ▼                                                                             │
│ STATE_RUNNING                                                                   │
│   │ Data: GREEN LED, processes GLAD commands                                   │
│   │ Exit: STOP button (4-second hold) or sequence complete                     │
│   ▼                                                                             │
│ Back to STATE_ZEROING                                                           │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Layer 3: Driver Data Flow

### Hardware Configuration Detection
```
DRIVER INITIALIZATION DATA:
┌─────────────────────────────────────────┐
│                                         │
│ Strap Pin Reading (D3,D4,D5):           │
│   │                                     │
│   ▼                                     │
│ ┌─────────────────┐                     │
│ │ strapCode =     │  0b111 ──► X-axis   │
│ │ (D5<<2) |       │  0b110 ──► Y-axis   │
│ │ (D4<<1) |       │  0b101 ──► Z-axis   │
│ │ (D3<<0)         │  0b100 ──► G-axis   │
│ │                 │  0b011 ──► T-axis   │
│ └─────────────────┘                     │
│   │                                     │
│   ▼                                     │
│ Speed Selection (A1-A2):                │
│   if (A1 connected to A2):              │
│     MOVE_MAX_SPEED = full_speed         │
│   else:                                 │
│     MOVE_MAX_SPEED = 0.5 * full_speed   │
│                                         │
└─────────────────────────────────────────┘
```

### Motor Speed Configuration by Axis
```
AXIS-SPECIFIC SPEED CONFIGURATION:
┌────────────────────────────────────────────────────────────────────────────────┐
│                                                                                │
│ X-AXIS: MOVE_MAX_SPEED=2600, MOVE_HOME_SPEED=300, TOLERANCE=3                 │
│ Y-AXIS: MOVE_MAX_SPEED=4000, MOVE_HOME_SPEED=300, TOLERANCE=10                │
│ Z-AXIS: MOVE_MAX_SPEED=4000, MOVE_HOME_SPEED=300, TOLERANCE=0                 │
│ T-AXIS: MOVE_MAX_SPEED=5000, MOVE_HOME_SPEED=4000, TOLERANCE=100              │
│ G-AXIS: MOVE_MAX_SPEED=4000, MOVE_HOME_SPEED=150, TOLERANCE=10                │
│                                                                                │
│ AccelStepper Configuration:                                                    │
│   stepperMotor.setMaxSpeed(speed)                                             │
│   stepperMotor.setAcceleration(speed * 0.5)                                  │
│                                                                                │
└────────────────────────────────────────────────────────────────────────────────┘
```

### Command Processing Flow
```
DRIVER COMMAND PROCESSING:
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                                 │
│ AltSoftSerial Input: "X3870,Y390,T240,G-30*A3"                                 │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Parse Command   │              │ Find '*' separator                      │    │
│ │ & Validate CRC  │─────────────►│ command = "X3870,Y390,T240,G-30"       │    │
│ │                 │              │ checksum = "A3"                         │    │
│ │                 │              │ Validate XOR checksum                   │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Split by Comma  │              │ "X3870" → targetDriverID='X', pos=3870 │    │
│ │ & Filter by ID  │─────────────►│ "Y390"  → targetDriverID='Y', pos=390  │    │
│ │                 │              │ "T240"  → targetDriverID='T', pos=240  │    │
│ │                 │              │ "G-30"  → targetDriverID='G', pos=-30  │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Execute if      │              │ if (targetDriverID == myDriverID):     │    │
│ │ ID Matches      │─────────────►│   if (pos == 0): performHoming()       │    │
│ │                 │              │   else: moveToPosition(pos)             │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### Motor Movement Execution
```
MOTOR MOVEMENT DATA FLOW:
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                                 │
│ NORMAL MOVEMENT: moveToPosition(3870)                                          │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Set Parameters  │              │ stepperMotor.setMaxSpeed(MOVE_MAX_SPEED)│    │
│ │                 │─────────────►│ stepperMotor.setAcceleration(MOVE_ACCEL)│    │
│ │                 │              │ stepperMotor.moveTo(3870)               │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Execute Move    │              │ while(distanceToGo() != 0):             │    │
│ │ with Monitoring │─────────────►│   stepperMotor.run()                    │    │
│ │                 │              │   if(distanceToGo() <= TOLERANCE):      │    │
│ │                 │              │     setStatusLED(true)                  │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
│ HOMING MOVEMENT: performHoming()                                               │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Homing Sequence │   Step 1     │ if(isAtHomePosition()): moveAwayFromHome│    │
│ │                 │─────────────►│ Step 2: returnToHome()                  │    │
│ │                 │   Step 3     │ Step 3: setCurrentPosition(0)           │    │
│ │                 │──────────────►│ Step 4: moveTo(0)                      │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## System Integration Data Flow Summary

```
COMPLETE SYSTEM DATA FLOW SUMMARY:
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                                 │
│ 1. CENTRAL reads sensors → generates commands → sends via RS485                 │
│    Data: "L#H(x,y,z,t,g)*checksum" or "R#G(xn,yn,zn...)*checksum"             │
│                                                                                 │
│ 2. ARM CONTROL receives → validates → parses → breaks into motor commands      │
│    Data: "X3870,Y390,T240,G-30*checksum" → sent via AltSoftSerial             │
│                                                                                 │
│ 3. DRIVERS receive → filter by ID → execute motor movements                    │
│    Data: Only processes commands matching own driver ID (X,Y,Z,T,G)            │
│                                                                                 │
│ 4. STATUS flows back: DRIVER→ARM CONTROL→CENTRAL via hardware pins             │
│    Data: MOTOR_DONE_PIN → COMMAND_ACTIVE_PIN → ARM1_PIN/ARM2_PIN               │
│                                                                                 │
│ 5. CENTRAL monitors status → updates state machines → generates next commands  │
│    Data: State transitions trigger next command generation cycle               │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Critical Data Dependencies

1. **EEPROM Parameters**: Must be valid for command generation
2. **DIP Switch Settings**: Determines ARM start positions  
3. **Hardware Pin States**: Critical for state machine transitions
4. **Checksum Validation**: All inter-layer communication depends on this
5. **Device ID Detection**: ARM1/ARM2 and X/Y/Z/T/G identification must work
6. **Timing Synchronization**: State machines depend on precise timing
