# PalletizerV1.3 System Data Flow Documentation
*Detailed Code-Level Communication Analysis*

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

### Command Generation Process - WITH CODE REFERENCES
```
DYNAMIC COMMAND GENERATION:
┌─────────────────────────────────────────────────────────────────────┐
│ 🔗 CODE: PalletizerCentralStateMachine.ino                         │
│                                                                     │
│ Input: ARM_ID + command_index                                       │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Calculate       │  📍Line 1212 │ commandPair = index / 2     │    │
│ │ Layer & Task    │─────────────►│ layer = commandPair / 8     │    │
│ │ generateCommand │  📍Line 1213 │ task = commandPair % 8      │    │
│ │ ()              │  📍Line 1214 │ isHome = (index % 2 == 0)   │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Select          │  📍Line 1220 │ if (armId == 1):            │    │
│ │ ARM Offset      │─────────────►│   xOffset = params.xL       │    │
│ │                 │  📍Line 1221 │ else:                       │    │
│ │                 │              │   xOffset = params.xR       │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐                                                 │
│ │ Generate        │  📍Line 1239: sprintf(commandBuffer, "H(...)" │
│ │ Command String  │  📍Line 1261: sprintf(commandBuffer, "G(...)" │
│ │                 │  📍Line  775: return "P"                       │
│ │                 │  📍Line  777: return "C"                       │
│ └─────────────────┘                                                 │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Add ARM Prefix  │  📍Line  828 │ armPrefix = (arm_in_center  │    │
│ │ & Send RS485    │─────────────►│   == 1) ? "L" : "R";       │    │
│ │                 │  📍Line  829 │ fullCommand = armPrefix +   │    │
│ │ sendRS485Cmd... │              │   "#" + gladCommand;        │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ 📍Line 1320: sendRS485Command(command)                             │
│ 📍Line 1326: uint8_t checksum = calculateXORChecksum(...)          │
│ 📍Line 1327: String fullCommand = command + "*" + checksum         │
│ 📍Line 1328: rs485.println(fullCommand);                           │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### RS485 Communication: Central → ARM Control
```
📡 TRANSMISSION PROCESS:
┌─────────────────────────────────────────────────────────────────────┐
│ 🔗 FROM: PalletizerCentralStateMachine.ino                         │
│ 🔗 TO:   PalletizerArmControl.ino                                   │
│                                                                     │
│ SENDER SIDE (Central):                                              │
│ 📍Line   42: SoftwareSerial rs485(RS485_RO, RS485_DI);             │
│ 📍Line 1089: rs485.begin(9600);                                    │
│                                                                     │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Format Command  │  📍Line 1327 │ fullCommand = command +     │    │
│ │                 │─────────────►│   "*" + String(checksum,HEX)│    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Transmit via    │  📍Line 1328 │ rs485.println(fullCommand); │    │
│ │ RS485           │─────────────►│ rs485.flush();              │    │
│ │                 │  📍Line 1329 │ delay(50);                  │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│                                                                     │
│ RECEIVER SIDE (ARM Control):                                        │
│ 📍Line   76: SoftwareSerial rs485Serial(RS485_RX_PIN,RS485_TX_PIN);│
│ 📍Line  259: rs485Serial.begin(SERIAL_BAUD_RATE);                  │
│                                                                     │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Receive Buffer  │  📍Line  474 │ if (rs485Serial.available())│    │
│ │                 │─────────────►│   while (rs485Serial.       │    │
│ │                 │  📍Line  475 │     available())             │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Parse Command   │  📍Line  476 │ char receivedChar =         │    │
│ │                 │─────────────►│   rs485Serial.read();       │    │
│ │                 │  📍Line  478 │ if (receivedChar == '\n')   │    │
│ │                 │  📍Line  480 │   processCommand(buffer);   │    │
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

### AltSoftSerial Communication: ARM Control → Drivers
```
📡 MOTOR COMMAND TRANSMISSION:
┌─────────────────────────────────────────────────────────────────────┐
│ 🔗 FROM: PalletizerArmControl.ino                                   │
│ 🔗 TO:   PalletizerArmDriver.ino (each axis)                        │
│                                                                     │
│ SENDER SIDE (ARM Control):                                          │
│ 📍Line   75: AltSoftSerial motorSerial;                            │
│ 📍Line  256: motorSerial.begin(SERIAL_BAUD_RATE);                  │
│                                                                     │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Build Command   │  📍Line  992 │ snprintf_P(command, sizeof  │    │
│ │                 │─────────────►│   (command), PSTR("X%d,Y%d │    │
│ │                 │  📍Line  993 │   ,T%d,G%d"), homeCmd.x,    │    │
│ │                 │              │   homeCmd.y, homeCmd.t,...)  │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Add Checksum    │  📍Line  585 │ uint8_t checksum =          │    │
│ │                 │─────────────►│   calculateXORChecksum(...) │    │
│ │                 │  📍Line  596 │ motorSerial.print(checksum, │    │
│ │                 │              │   HEX);                     │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Transmit        │  📍Line  595 │ motorSerial.print(command); │    │
│ │                 │─────────────►│ motorSerial.print("*");     │    │
│ │                 │  📍Line  598 │ motorSerial.println();      │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│                                                                     │
│ RECEIVER SIDE (Driver):                                             │
│ 📍Line   44: AltSoftSerial serialComm;                             │
│ 📍Line  153: serialComm.begin(SERIAL_BAUD_RATE);                   │
│                                                                     │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Receive Buffer  │  📍Line  251 │ while (serialComm.          │    │
│ │                 │─────────────►│   available())               │    │
│ │                 │  📍Line  252 │   char receivedChar =        │    │
│ │                 │              │     serialComm.read();       │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│   │                                                                 │
│   ▼                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────┐    │
│ │ Parse & Filter  │  📍Line  255 │ if (receivedChar == '\n')   │    │
│ │                 │─────────────►│   validateAndExecuteCommand │    │
│ │                 │  📍Line  258 │     (commandBuffer);         │    │
│ └─────────────────┘              └─────────────────────────────┘    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Command Sequence Execution - WITH CODE REFERENCES
```
COMMAND SEQUENCE BREAKDOWN:
┌─────────────────────────────────────────────────────────────────────────────────┐
│ 🔗 CODE: PalletizerArmControl.ino                                               │
│                                                                                 │
│ HOME COMMAND: "L#H(3870,390,3840,240,-30)"                                     │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐    Parse     ┌─────────────────────────────────────────┐    │
│ │ parseHomeCommand│  📍Line  842 │ sscanf(openParen + 1, "%d,%d,%d,%d,%d", │    │
│ │                 │─────────────►│   &homeCmd.x, &homeCmd.y, &homeCmd.z,  │    │
│ │                 │  📍Line  843 │   &homeCmd.t, &homeCmd.g);             │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ HOME Sequence   │   Step 1     │ 📍Line  992: snprintf_P(command,       │    │
│ │ executeHomeStep │  📍Line  989 │   PSTR("X%d,Y%d,T%d,G%d"), homeCmd.x,  │    │
│ │                 │─────────────►│   homeCmd.y, homeCmd.t, homeCmd.g);    │    │
│ │                 │   Step 2     │ 📍Line 1001: snprintf_P(command,       │    │
│ │                 │  📍Line 1000 │   PSTR("Z%d"), homeCmd.z);             │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
│ GLAD COMMAND: "L#G(1620,2205,3975,240,60,270,750,3960,2340,240)"              │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ GLAD Sequence   │   Step 1     │ 📍Line 1062: sendSafeMotorCommand      │    │
│ │ executeGladStep │  📍Line 1059 │   (PSTR("Z%d"), gladCmd.zb);           │    │
│ │ (8 steps total) │─────────────►│ Step 2: PSTR("G%d"), gladCmd.gp);      │    │
│ │                 │   Step 3     │ Step 3: PSTR("Z%d"), zn-za);           │    │
│ │                 │──────────────►│ Step 4: PSTR("X%d,Y%d,T%d"), xn,yn,tn);│    │
│ │                 │   ...        │ Step 5: PSTR("Z%d"), gladCmd.zn);      │    │
│ │                 │              │ Step 6: PSTR("G%d"), gladCmd.dp);      │    │
│ │                 │              │ Step 7: PSTR("Z%d"), zn-za again);     │    │
│ │                 │              │ Step 8: PSTR("X%d,T%d"), xa,ta);       │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### Driver Command Processing - WITH CODE REFERENCES  
```
DRIVER COMMAND FILTERING & EXECUTION:
┌─────────────────────────────────────────────────────────────────────────────────┐
│ 🔗 CODE: PalletizerArmDriver.ino                                                │
│                                                                                 │
│ INPUT: "X3870,Y390,T240,G-30*A3"                                               │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Validate CRC    │  📍Line   75 │ separatorIndex = receivedData.          │    │
│ │                 │─────────────►│   indexOf('*');                         │    │
│ │                 │  📍Line   97 │ if (receivedChecksum ==                 │    │
│ │                 │              │     calculatedChecksum)                 │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Split by Comma  │  📍Line  276 │ int startIndex = 0;                     │    │
│ │                 │─────────────►│ commaIndex = command.indexOf(',',       │    │
│ │                 │  📍Line  281 │   startIndex);                          │    │
│ │                 │              │ Parse each: "X3870", "Y390", etc.      │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Filter by       │  📍Line  300 │ char targetDriverID =                   │    │
│ │ Driver ID       │─────────────►│   singleCommand.charAt(0);              │    │
│ │                 │  📍Line  308 │ if (targetDriverID == driverID) {       │    │
│ │                 │              │   // Process this command               │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Execute Motor   │  📍Line  312 │ long targetPosition =                   │    │
│ │ Movement        │─────────────►│   singleCommand.substring(1).toInt();  │    │
│ │                 │  📍Line  317 │ if (targetPosition == 0)                │    │
│ │                 │              │   performHoming();                      │    │
│ │                 │              │ else moveToPosition(targetPosition);    │    │
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

### Status Feedback Flow - WITH CODE REFERENCES
```
📡 STATUS FEEDBACK CHAIN:
┌─────────────────────────────────────────────────────────────────────────────────┐
│ 🔗 DRIVER → ARM CONTROL → CENTRAL                                               │
│                                                                                 │
│ DRIVER STATUS (PalletizerArmDriver.ino):                                       │
│ 📍Line  425: setStatusLED(bool state)                                          │
│ 📍Line  429: digitalWrite(LED_STATUS_PIN, LOW);  // Busy                       │
│ 📍Line  436: digitalWrite(LED_STATUS_PIN, HIGH); // Ready                      │
│                                                                                 │
│ ARM CONTROL STATUS (PalletizerArmControl.ino):                                 │
│ 📍Line   26: const int MOTOR_DONE_PIN = 3;       // Input dari drivers        │
│ 📍Line   18: const int COMMAND_ACTIVE_PIN = 13;  // Output ke central         │
│                                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Read Motor      │  📍Line  774 │ bool reading1 =                         │    │
│ │ Status          │─────────────►│   digitalRead(MOTOR_DONE_PIN);          │    │
│ │ (PalletizerArm  │  📍Line  767 │ return lastMotorState == HIGH;          │    │
│ │ Control.ino)    │              │ // HIGH = ready, LOW = busy              │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Set Command     │  📍Line  419 │ digitalWrite(COMMAND_ACTIVE_PIN, HIGH); │    │
│ │ Active Status   │─────────────►│ // HIGH = busy/zeroing                  │    │
│ │ (PalletizerArm  │  📍Line  446 │ digitalWrite(COMMAND_ACTIVE_PIN, LOW);  │    │
│ │ Control.ino)    │              │ // LOW = ready/running                  │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
│ CENTRAL STATUS READ (PalletizerCentralStateMachine.ino):                       │
│ 📍Line   10: const int ARM1_PIN = 7;      // Input dari ARM1 control          │
│ 📍Line   11: const int ARM2_PIN = 8;      // Input dari ARM2 control          │
│                                                                                 │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Read ARM        │  📍Line 1281 │ arm1_response =                         │    │
│ │ Status          │─────────────►│   digitalRead(ARM1_PIN);                │    │
│ │ (PalletizerCen  │  📍Line 1282 │ arm2_response =                         │    │
│ │ tralStateMach.. │              │   digitalRead(ARM2_PIN);                │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│   │                                                                             │
│   ▼                                                                             │
│ ┌─────────────────┐              ┌─────────────────────────────────────────┐    │
│ │ Update State    │  📍Line 1284 │ arm1_sm.is_busy = arm1_response;        │    │
│ │ Machine         │─────────────►│ arm2_sm.is_busy = arm2_response;        │    │
│ │ (PalletizerCen  │  📍Line  641 │ bool hardware_busy = digitalRead(...)   │    │
│ │ tralStateMach.. │              │ // Used in state transitions            │    │
│ └─────────────────┘              └─────────────────────────────────────────┘    │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## System Integration Data Flow Summary - WITH FILE REFERENCES

```
COMPLETE SYSTEM DATA FLOW SUMMARY:
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                                 │
│ 1. 🔗 PalletizerCentralStateMachine.ino reads sensors → generates commands     │
│    📍Line 1274-1282: readSensors() → sensor1_state, sensor2_state, sensor3     │
│    📍Line 1210: generateCommand() → "L#H(x,y,z,t,g)*checksum"                  │
│    📍Line 1328: rs485.println(fullCommand); → sends via RS485                  │
│                                                                                 │
│ 2. 🔗 PalletizerArmControl.ino receives → validates → parses → breaks down     │
│    📍Line 474-488: rs485Serial.available() → processCommand()                  │
│    📍Line 500-508: parseAndValidateMessage() → CRC validation                  │
│    📍Line 842: sscanf() parse HOME → 992: snprintf_P() build motor cmd         │
│    📍Line 595-598: motorSerial.print() → "X3870,Y390,T240,G-30*checksum"      │
│                                                                                 │
│ 3. 🔗 PalletizerArmDriver.ino receive → filter by ID → execute movements      │
│    📍Line 251-265: serialComm.available() → commandBuffer                     │
│    📍Line 97: CRC validation → executeCommand()                               │  
│    📍Line 300-308: if (targetDriverID == driverID) → process command          │
│    📍Line 317-322: if (pos==0) performHoming() else moveToPosition()          │
│                                                                                 │
│ 4. 🔗 STATUS flows back: DRIVER→ARM CONTROL→CENTRAL via hardware pins         │
│    📍Driver Line 429/436: LED_STATUS_PIN (HIGH=ready, LOW=busy)               │
│    📍ARM Control Line 774: digitalRead(MOTOR_DONE_PIN)                        │
│    📍ARM Control Line 419/446: digitalWrite(COMMAND_ACTIVE_PIN)               │ 
│    📍Central Line 1281-1282: digitalRead(ARM1_PIN/ARM2_PIN)                   │
│                                                                                 │
│ 5. 🔗 PalletizerCentralStateMachine.ino monitors → updates → next commands    │
│    📍Line 1151-1152: updateArmStateMachine() → state transitions              │
│    📍Line 1160: handleSystemLogicStateMachine() → trigger next cycle          │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## Critical Communication Failure Points - WITH EXACT CODE LOCATIONS

### 🚨 **HIGH PROBABILITY FAILURE POINTS:**

```
FAILURE ANALYSIS WITH CODE REFERENCES:
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                                 │
│ 1. 🔥 COMMAND ROUTING BUG (Central):                                           │
│    📍Line 511-512: const char* devicePrefix = isARM2_device ? "R" : "L";      │  
│    ❌ PROBLEM: isARM2_device undefined in Central → always "L"                │
│    💥 IMPACT: ARM2 commands never processed                                   │
│                                                                                 │
│ 2. 🔥 DEVICE DETECTION BUG (ARM Control):                                     │
│    📍Line 249-251: return digitalRead(A4) == LOW;                             │
│    ❌ PROBLEM: A4 with INPUT_PULLUP = always HIGH when not connected          │
│    💥 IMPACT: ARM1/ARM2 detection fails → wrong command filtering             │
│                                                                                 │
│ 3. 🔥 BLOCKING DELAYS (ARM Control):                                          │
│    📍Line 644,648,775,777: delay(10); delay(20);                              │
│    ❌ PROBLEM: Total 60ms+ blocking delays per loop cycle                     │
│    💥 IMPACT: Missed RS485 communication, timing failures                     │
│                                                                                 │
│ 4. 🔥 COMMAND PROCESSING (ARM Control):                                       │
│    📍Line 510-520: if (strncmp(command, devicePrefix, 1) == 0)                │
│    ❌ PROBLEM: Using 'command' instead of 'cleanCommand'                      │
│    💥 IMPACT: Commands with CRC suffix not matched properly                   │
│                                                                                 │
│ 5. 🔥 MEMORY CORRUPTION (Driver):                                             │
│    📍Line 49: String commandBuffer = "";                                      │
│    ❌ PROBLEM: Arduino String class dynamic allocation                        │
│    💥 IMPACT: Memory fragmentation → system crashes                           │
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
