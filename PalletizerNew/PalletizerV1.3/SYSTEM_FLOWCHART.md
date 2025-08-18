# PalletizerV1.3 System Communication Flowchart
*Detailed Mermaid Diagrams showing exact data flow and timing*

## 1. System Startup Flow

```mermaid
graph TD
    A[System Power ON] --> B[Central State Machine<br/>PalletizerCentralStateMachine.ino<br/>Line 1087: setup()]
    B --> C[Load EEPROM Parameters<br/>Line 1117: loadParametersFromEEPROM()]
    C --> D{EEPROM Valid?}
    D -->|No| E[Reset to Defaults<br/>Line 1118: resetParametersToDefault()]
    D -->|Yes| F[Parameters Loaded<br/>56 parameters total]
    E --> F
    F --> G[Read DIP Switches<br/>Line 1125: readDipSwitchLayers()]
    G --> H[ARM1 Start Layer: DIP pins D5,D6,D3,D4<br/>ARM2 Start Layer: DIP pins A0,A1,A3,A2]
    H --> I[Initialize RS485<br/>Line 1089: rs485.begin(9600)]
    I --> J[Initialize ARM State Machines<br/>Line 1128: initializeArmDataStateMachine()]
    J --> K[Central READY<br/>Print: System ready with generated commands]
    
    L[ARM Control Startup<br/>PalletizerArmControl.ino<br/>Line 186: setup()] --> M[Device Detection<br/>Line 193: isARM2()]
    M --> N{A4 == LOW?}
    N -->|Yes| O[Device: ARM2<br/>Prefix: R]
    N -->|No| P[Device: ARM1<br/>Prefix: L]
    O --> Q[Initialize Serial<br/>Line 198: initializeSerial()]
    P --> Q
    Q --> R[RS485: Line 259<br/>AltSoftSerial: Line 256]
    R --> S[Enter ZEROING State<br/>Line 206: enterZeroingState()]
    
    T[Driver Startup<br/>PalletizerArmDriver.ino<br/>Line 107: setup()] --> U[Read Strap Pins<br/>Line 110: setDriverID()]
    U --> V[Detect Driver ID<br/>D3,D4,D5 pins → X,Y,Z,T,G]
    V --> W[Set Speed Parameters<br/>Line 111: setSpeedParameters()]
    W --> X[Check A1-A2 Connection<br/>High/Normal Speed Selection]
    X --> Y[Initialize AccelStepper<br/>Line 112: initializeStepperMotor()]
    Y --> Z[Driver READY<br/>Print: Driver ID + Speed config]
```

## 2. Main Operational Flow - Who Sends What First

```mermaid
sequenceDiagram
    participant C as Central State Machine<br/>(PalletizerCentralStateMachine.ino)
    participant A1 as ARM1 Control<br/>(PalletizerArmControl.ino)  
    participant A2 as ARM2 Control<br/>(PalletizerArmControl.ino)
    participant D1 as Drivers ARM1<br/>(X,Y,Z,T,G instances)
    participant D2 as Drivers ARM2<br/>(X,Y,Z,T,G instances)
    
    Note over C: STARTUP COMPLETE - Main Loop Begins
    
    loop Every ~30ms - Main Loop
        C->>C: Read Sensors<br/>Line 1145-1147: readSensors()<br/>sensor1,2,3 + ARM1,2 status
        Note over C: Sensor States:<br/>S1,S2: Product detection<br/>S3: ARM in center<br/>ARM1,2: Busy status
        
        C->>C: Update State Machines<br/>Line 1151-1152:<br/>updateArmStateMachine(&arm1_sm)<br/>updateArmStateMachine(&arm2_sm)
        
        alt Sensor3 HIGH + ARM available
            C->>C: Generate HOME Command<br/>Line 1210: generateCommand(arm_id, cmd_index)<br/>Result: "H(3870,390,3840,240,-30)"
            C->>C: Add ARM Prefix<br/>Line 828-829:<br/>"L#" or "R#" + command
            C->>C: Calculate Checksum<br/>Line 1326: calculateXORChecksum()
            C->>C: Send RS485<br/>Line 1328: rs485.println("L#H(...)*7F")
            
            alt Command for ARM1
                C->>A1: RS485: "L#H(3870,390,3840,240,-30)*7F"<br/>9600 baud, Pin 10,11
                A1->>A1: Receive Buffer<br/>Line 474-488: processSerialCommands()
                A1->>A1: Validate CRC<br/>Line 500-508: parseAndValidateMessage()
                A1->>A1: Check Device Prefix<br/>Line 510-520: if (strncmp(command, "L", 1))
                A1->>A1: Parse HOME Command<br/>Line 842-843: sscanf("%d,%d,%d,%d,%d")
                A1->>A1: Start HOME Sequence<br/>Line 912-925: startHomeSequence()
                
                A1->>A1: Set COMMAND_ACTIVE_PIN LOW<br/>Line 446: digitalWrite(13, LOW)
                A1->>A1: Build Motor Command Step 1<br/>Line 992-993: snprintf_P("X%d,Y%d,T%d,G%d")
                A1->>A1: Add Checksum<br/>Line 585: calculateXORChecksum()
                A1->>D1: AltSoftSerial: "X3870,Y390,T240,G-30*A3"<br/>Pin 8,9
                
                loop For each Driver (X,Y,Z,T,G)
                    D1->>D1: Receive Command<br/>Line 251-265: processSerialInput()
                    D1->>D1: Validate CRC<br/>Line 73-104: validateAndExecuteCommand()
                    D1->>D1: Parse Multi-Command<br/>Line 276-336: executeCommand()
                    D1->>D1: Filter by Driver ID<br/>Line 300-308: if (targetDriverID == myDriverID)
                    
                    alt Command matches my ID
                        D1->>D1: Set LED OFF (Busy)<br/>Line 320: setStatusLED(false)
                        D1->>D1: Move to Position<br/>Line 340-358: moveToPosition()
                        D1->>D1: Set LED ON (Ready)<br/>Line 322: setStatusLED(true)
                    else Command not for me
                        D1->>D1: Ignore Command<br/>Line 328-330: skip
                    end
                end
                
                A1->>A1: Wait for Motor Ready<br/>Line 767-792: isMotorReady()
                A1->>A1: Build Motor Command Step 2<br/>Line 1000-1001: snprintf_P("Z%d")
                A1->>D1: AltSoftSerial: "Z3840*B1"
                
                A1->>A1: HOME Sequence Complete<br/>Line 1009: currentSequence = SEQ_NONE
                A1->>A1: Set COMMAND_ACTIVE_PIN LOW<br/>Line 446: Running State
                
            else Command for ARM2
                C->>A2: RS485: "R#H(3870,390,3840,240,-30)*7F"
                Note over A2: Same process as ARM1<br/>but filtered by "R#" prefix
                A2->>D2: Similar motor commands to ARM2 drivers
            end
            
            C->>C: Read ARM Status<br/>Line 1281-1282: digitalRead(ARM1_PIN/ARM2_PIN)
            C->>C: Update arm_in_center<br/>ARM reached center position
        end
        
        alt Products Detected + ARM in center
            C->>C: Product Logic<br/>Line 960: if (!sensor1 && !sensor2 && !sensor3)
            C->>C: Generate GLAD Command<br/>Line 820: getNextCommandStateMachine()
            C->>C: Send GLAD<br/>"L#G(1620,2205,3975,240,60,270,750,3960,2340,240)*B7"
            
            alt GLAD for ARM1
                C->>A1: RS485: GLAD Command
                A1->>A1: Parse GLAD<br/>Line 865-907: parseGladCommand()
                A1->>A1: Start GLAD Sequence<br/>Line 928-940: startGladSequence()
                
                loop 8 GLAD Steps
                    A1->>A1: Build Step Command<br/>Line 1053-1161: executeGladStep()
                    A1->>D1: Motor Command for current step
                    D1->>D1: Execute Movement
                    A1->>A1: Wait for completion<br/>isMotorReady()
                end
                
                A1->>A1: GLAD Complete<br/>Transition to READY state
            end
            
            C->>C: Turn OFF Conveyor<br/>Line 831: turnOffConveyor()
            C->>C: Set ARM to PICKING state<br/>Line 836: changeArmState(PICKING)
        end
        
        alt Special Commands Needed
            C->>C: Check Position<br/>Line 566: if (current_pos % 64 == 0)
            
            alt Even Layer Complete
                C->>C: Set CALI Command<br/>Line 567-568: SPECIAL_CALI
                C->>A1: RS485: "L#C*2D"
                A1->>A1: Execute CAL<br/>Line 534-540: ZEROING with CAL flag
                A1->>D1: PARK Sequence<br/>Z0, X0T0G0, Y0 commands
            end
            
            alt All Layers Complete  
                C->>C: Set PARK Command<br/>Line 570-571: SPECIAL_PARK
                C->>A1: RS485: "L#P*1F"
                A1->>A1: Execute PARK<br/>Line 527-533: ZEROING state
                A1->>D1: PARK Sequence commands
            end
        end
        
        C->>C: Control Conveyor<br/>Line 1164: controlConveyor()
        Note over C: Auto ON after 3 seconds
    end
```

## 3. Critical Data Flow Paths

```mermaid
graph LR
    subgraph "Layer 1: Central State Machine"
        A[Sensor Reading<br/>Line 1274-1282] --> B[State Machine Update<br/>Line 1151-1152]
        B --> C[System Logic<br/>Line 1160]
        C --> D[Command Generation<br/>Line 1210]
        D --> E[RS485 Send<br/>Line 1328]
    end
    
    subgraph "Layer 2: ARM Control"  
        F[RS485 Receive<br/>Line 474-488] --> G[CRC Validation<br/>Line 500-508]
        G --> H[Command Parse<br/>Line 842/876]
        H --> I[Sequence Execute<br/>Line 986/1053]
        I --> J[AltSoft Send<br/>Line 595-598]
    end
    
    subgraph "Layer 3: Drivers"
        K[AltSoft Receive<br/>Line 251-265] --> L[CRC Validate<br/>Line 97]
        L --> M[ID Filter<br/>Line 308] 
        M --> N[Motor Execute<br/>Line 340/361]
        N --> O[Status LED<br/>Line 425]
    end
    
    E --> F
    J --> K
    O --> P[Hardware Status<br/>Pin 3 → Pin 13 → Pin 7/8]
    P --> A
```

## 4. Hardware Pin Communication Flow

```mermaid
graph TB
    subgraph "CENTRAL (PalletizerCentralStateMachine.ino)"
        C1[ARM1_PIN = 7<br/>Input from ARM1]
        C2[ARM2_PIN = 8<br/>Input from ARM2] 
        C3[RS485_RO = 10<br/>RS485_DI = 11<br/>Output to ARM Controls]
        C4[Sensor Pins:<br/>S1=A4, S2=A5, S3=D2<br/>DIP1=D5,D6,D3,D4<br/>DIP2=A0,A1,A3,A2]
    end
    
    subgraph "ARM CONTROL 1 (PalletizerArmControl.ino)"
        A1[COMMAND_ACTIVE_PIN = 13<br/>Output to Central Pin 7]
        A2[MOTOR_DONE_PIN = 3<br/>Input from Drivers]
        A3[AltSoftSerial Pin 8,9<br/>Output to Drivers]
        A4[RS485_RX=10, RS485_TX=11<br/>Input from Central]
        A5[Device ID: A4,A5<br/>A4=INPUT_PULLUP<br/>A5=OUTPUT_LOW]
    end
    
    subgraph "ARM CONTROL 2 (PalletizerArmControl.ino)"  
        B1[COMMAND_ACTIVE_PIN = 13<br/>Output to Central Pin 8]
        B2[MOTOR_DONE_PIN = 3<br/>Input from Drivers]
        B3[AltSoftSerial Pin 8,9<br/>Output to Drivers] 
        B4[RS485_RX=10, RS485_TX=11<br/>Input from Central]
        B5[Device ID: A4,A5<br/>A4=LOW for ARM2]
    end
    
    subgraph "DRIVERS (PalletizerArmDriver.ino)"
        D1[LED_STATUS_PIN = 13<br/>Output to ARM Control Pin 3]
        D2[AltSoftSerial Pin 8,9<br/>Input from ARM Control]
        D3[Strap Pins D3,D4,D5<br/>Hardware ID detection]
        D4[Speed Select A1,A2<br/>Connected = High Speed]
        D5[LIMIT_SWITCH_PIN = A0<br/>Homing reference]
    end
    
    C3 --> A4
    C3 --> B4
    A1 --> C1
    B1 --> C2
    A3 --> D2
    B3 --> D2
    D1 --> A2
    D1 --> B2
```

## 5. Error and Retry Flow

```mermaid
graph TD
    A[Command Sent] --> B{Response Received?}
    B -->|No| C[Wait Timeout<br/>Central: 500ms<br/>ARM: 200ms<br/>Driver: 200ms]
    C --> D{Max Retries?<br/>Central: 7<br/>ARM: 10<br/>Driver: N/A}
    D -->|No| E[Retry Count++<br/>Resend Command]
    E --> A
    D -->|Yes| F[Enter ERROR State<br/>Central: Line 1368<br/>ARM: Line 1180]
    F --> G[Auto Recovery<br/>30 seconds timeout<br/>Line 626-631]
    G --> H[Return to IDLE]
    B -->|Yes| I[Command Success<br/>Continue Sequence]
    
    I --> J{Sequence Complete?}
    J -->|No| K[Next Step]
    K --> A
    J -->|Yes| L[State Transition<br/>READY/RUNNING/IDLE]
```

## 6. State Machine Interaction

```mermaid
stateDiagram-v2
    [*] --> Central_Init
    
    state Central_Init {
        [*] --> EEPROM_Load
        EEPROM_Load --> DIP_Read
        DIP_Read --> RS485_Init
        RS485_Init --> ARM_SM_Init
        ARM_SM_Init --> [*]
    }
    
    Central_Init --> Central_Running
    
    state Central_Running {
        [*] --> Sensor_Read
        Sensor_Read --> State_Update
        State_Update --> System_Logic
        System_Logic --> Command_Gen
        Command_Gen --> RS485_Send
        RS485_Send --> Sensor_Read
        
        System_Logic --> Special_Command: Even layer/All complete
        Special_Command --> RS485_Send
    }
    
    state ARM_Control {
        [*] --> ZEROING
        ZEROING --> SLEEPING: Park complete
        SLEEPING --> READY: START button (4s hold)
        READY --> RUNNING: HOME command received  
        RUNNING --> READY: GLAD complete
        RUNNING --> ZEROING: STOP button (4s hold)
        READY --> ZEROING: PARK command
        READY --> ZEROING: CAL command
    }
    
    state Driver_State {
        [*] --> Idle
        Idle --> Moving: Command received
        Moving --> Homing: Position = 0
        Moving --> Position: Position > 0
        Homing --> Idle: Complete
        Position --> Idle: Complete
    }
    
    Central_Running --> ARM_Control: RS485 Command
    ARM_Control --> Driver_State: AltSoft Command
    Driver_State --> ARM_Control: Status Pin
    ARM_Control --> Central_Running: Status Pin
```

## Summary: First Message & Data Flow Priority

### 🚀 **SYSTEM STARTUP ORDER:**

1. **FIRST**: All three components power up simultaneously
2. **SECOND**: Central loads EEPROM → reads DIP switches → initializes state machines  
3. **THIRD**: ARM Controls detect device ID → initialize serial → enter ZEROING state
4. **FOURTH**: Drivers detect hardware ID → set speeds → initialize steppers

### 📡 **FIRST COMMUNICATION:**

1. **INITIATOR**: Central State Machine (Master)
2. **FIRST MESSAGE**: HOME command when sensor3 goes HIGH
3. **DATA**: `"L#H(3870,390,3840,240,-30)*7F"` via RS485
4. **RECEIVER**: ARM Control (filters by device prefix L/R)
5. **RESPONSE**: ARM Control breaks down to motor commands
6. **FINAL**: Drivers execute individual motor movements

### ⚡ **CRITICAL SUCCESS PATH:**
Central sensor reading → State machine logic → Command generation → RS485 send → ARM receive → CRC validation → Command parsing → Motor command generation → AltSoft send → Driver receive → ID filtering → Motor execution → Status feedback → State update → Next cycle

Flowchart ini menunjukkan exactly siapa yang memulai, kapan, dan data apa yang dikirim pada setiap tahap sistem!