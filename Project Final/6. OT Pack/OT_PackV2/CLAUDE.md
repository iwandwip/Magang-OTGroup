# OT Pack V2 Project Documentation

## Project Overview
OT_PackV2 is a stepper motor control system for packaging automation. The system controls a single stepper motor with acceleration/deceleration profiles using the AccelStepper library. The latest version features a clean OOP design with the OTPack class, providing simple yet powerful functionality while maintaining ease of use.

## Hardware Configuration
- **Microcontroller**: Arduino compatible board
- **Motor**: Stepper motor with microstepping capability
- **Microstepping Resolution**: 4x (configurable via `MICROSTEPPING_RESOLUTION` constant)
- **Motor Steps**: 58 full steps per revolution (configured for specific motor model)

### Pin Mapping (Improved Naming)
```
SENSOR_INPUT_PIN (Pin 3)      - Digital input with pull-up (trigger sensor)
STEPPER_STEP_PIN (Pin 10)     - Step pulse output to motor driver
STEPPER_ENABLE_PIN (Pin 9)    - Motor enable/disable control
STEPPER_DIRECTION_PIN (Pin 8) - Direction control pin
```

## Software Architecture (OTPack Class Design)

### Core Components
1. **OTPack Class**: Encapsulated stepper motor control with clean public interface
2. **AccelStepper Integration**: Uses DRIVER mode for step/direction control
3. **Enum-Based State Machine**: Structured state management using `OTPack::State` enum
4. **Automatic Debouncing**: Built-in sensor debouncing with configurable timing
5. **Simple Public API**: Easy to use methods for configuration and control

### OTPack Class Structure
```cpp
class OTPack {
public:
    enum State {
        WAITING_FORWARD,
        WAITING_REVERSE
    };
    
    // Constructor & Setup
    OTPack(byte sensorPin, byte stepPin, byte dirPin, byte enablePin);
    void begin();
    
    // Runtime Control
    void update();
    void reset();
    
    // Configuration Methods
    void setMotorConfig(int stepsPerRev, int microstepping = 4);
    void setForwardProfile(float speed, float accel, int debounceMs = 150);
    void setReverseProfile(float speed, float accel, int debounceMs = 250, int settleMs = 100);
    void setPositionOffset(int offset = 2);
    
    // Status Methods
    State getState() const;
    bool isBusy() const;
};
```

### Simple Usage Pattern
```cpp
// Create instance
OTPack otpack(sensorPin, stepPin, dirPin, enablePin);

// Setup
void setup() {
    otpack.begin();
    otpack.setMotorConfig(58, 4);  // Optional customization
    otpack.setMotionMode(OTPack::NON_BLOCKING);  // Choose mode
}

// Main loop
void loop() {
    otpack.update();  // That's it!
}
```

### Blocking vs Non-Blocking Modes

#### BLOCKING Mode (Default)
- **Behavior**: `runToPosition()` - Program waits until motion completes
- **Pros**: Simple, guaranteed completion before next operation
- **Cons**: Cannot do other tasks during motion
- **Use Case**: Simple applications, single-task operations

#### NON_BLOCKING Mode  
- **Behavior**: `run()` - Returns immediately, motion continues in background
- **Pros**: Can perform other tasks while motor moves
- **Cons**: Need to check `isMoving()` status
- **Use Case**: Multi-tasking, responsive systems, UI updates

### Motion Control Logic
The system operates using a state machine with two distinct states:

#### WAITING_FOR_FORWARD State
- **Trigger**: Sensor input goes HIGH
- **Speed**: `FORWARD_MAX_SPEED` (1200) × microstepping = 4800 steps/sec
- **Acceleration**: `FORWARD_ACCELERATION` (600) × microstepping = 2400 steps/sec²
- **Distance**: `TOTAL_STEPS` (232) steps
- **Flow**: Configure → Debounce → Enable → Execute → State Transition

#### WAITING_FOR_REVERSE State  
- **Trigger**: Sensor input goes LOW
- **Speed**: `REVERSE_MAX_SPEED` (3000) × microstepping = 12000 steps/sec
- **Acceleration**: `REVERSE_ACCELERATION` (1900) × microstepping = 7600 steps/sec²
- **Distance**: -(232 - 2) = -230 steps (`POSITION_OFFSET` for accuracy)
- **Flow**: Configure → Debounce → Execute → Settle → Disable → State Transition

### AccelStepper Library Analysis

#### Library Version & Capabilities
- **Version**: 1.64
- **Author**: Mike McCauley
- **Features**: Acceleration/deceleration, multiple motor support, non-blocking operation
- **Interface Types**: DRIVER, FULL2WIRE, FULL3WIRE, FULL4WIRE, HALF3WIRE, HALF4WIRE

#### Key Classes & Methods Used
```cpp
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);
stepper.setMaxSpeed(speed);
stepper.setAcceleration(acceleration);
stepper.move(steps);
stepper.runToPosition();
```

#### Motion Algorithm
The library implements David Austin's stepper motor speed profile algorithm:
- **Initial Step Calculation**: Based on desired acceleration
- **Subsequent Steps**: Calculated using equation-based step intervals
- **Speed Profile**: Trapezoidal acceleration/constant speed/deceleration

## Code Analysis

### Performance Characteristics
- **Maximum Speed**: Library supports ~4000 steps/sec at 16MHz
- **Current Implementation**: Uses speeds up to 12000 steps/sec (may require faster processor)
- **Timing**: Uses microsecond-based step intervals for precision

### State Management
```cpp
bool state = false;  // Global state tracker
// state = false: Ready for forward motion
// state = true:  Ready for reverse motion
```

### Input Processing
- **Debouncing**: 150ms delay for forward motion, 250ms for reverse
- **Pull-up Resistor**: Internal pull-up enabled on input pin
- **Trigger Logic**: HIGH-to-LOW and LOW-to-HIGH transitions

### Serial Debugging
- **Baud Rate**: 9600
- **Output**: Continuous monitoring of P1_Input state
- **Format**: `"| digitalRead(3): [value]"`

## Improved Configuration Management

### Hardware Constants
```cpp
// Pin assignments with descriptive names
const byte SENSOR_INPUT_PIN = 3;
const byte STEPPER_STEP_PIN = 10;
const byte STEPPER_ENABLE_PIN = 9;
const byte STEPPER_DIRECTION_PIN = 8;

// Motor configuration
const int MICROSTEPPING_RESOLUTION = 4;
const int STEPS_PER_REVOLUTION = 58;
const int TOTAL_STEPS = STEPS_PER_REVOLUTION * MICROSTEPPING_RESOLUTION;
```

### Motion Profile Constants
```cpp
// Forward motion parameters
const float FORWARD_MAX_SPEED = 1200.0;
const float FORWARD_ACCELERATION = 600.0;

// Reverse motion parameters  
const float REVERSE_MAX_SPEED = 3000.0;
const float REVERSE_ACCELERATION = 1900.0;

// Timing parameters
const int FORWARD_DEBOUNCE_DELAY = 150;
const int REVERSE_DEBOUNCE_DELAY = 250;
const int REVERSE_SETTLE_DELAY = 100;
const int POSITION_OFFSET = 2;
```

### Improved Motion Profiles Table
| Parameter | Forward | Reverse | Units | Constant Name |
|-----------|---------|---------|-------|---------------|
| Max Speed | 4800 | 12000 | steps/sec | FORWARD_MAX_SPEED / REVERSE_MAX_SPEED |
| Acceleration | 2400 | 7600 | steps/sec² | FORWARD_ACCELERATION / REVERSE_ACCELERATION |
| Distance | +232 | -230 | steps | TOTAL_STEPS / -(TOTAL_STEPS - POSITION_OFFSET) |
| Debounce Delay | 150ms | 250ms | milliseconds | FORWARD_DEBOUNCE_DELAY / REVERSE_DEBOUNCE_DELAY |

## Enhanced Operational Flow
1. **System Initialization**: 
   - `initializePins()` - Configure all hardware pins
   - `initializeMotor()` - Set default motor parameters
   - Serial communication setup with status messages

2. **State Machine Loop**:
   - Read and display sensor status with descriptive output
   - Execute state-specific logic using switch-case structure
   - Clear state transitions with enum-based states

3. **Forward Motion Sequence**:
   - `configureMotorForForward()` - Set motion parameters
   - Debounce delay for sensor stability  
   - `enableMotor()` - Power up motor driver
   - Execute motion with detailed logging
   - State transition to WAITING_FOR_REVERSE

4. **Reverse Motion Sequence**:
   - `configureMotorForReverse()` - Set motion parameters
   - Debounce delay for sensor stability
   - Execute motion with position offset
   - Settle delay for mechanical stability
   - `disableMotor()` - Power down motor driver  
   - State transition to WAITING_FOR_FORWARD

## Dependencies
- **AccelStepper.h**: Primary motion control library
- **Arduino Core**: digitalRead, digitalWrite, pinMode, Serial functions

## Hardware Compatibility
- **Arduino Uno/Nano**: Compatible (tested configuration)
- **ESP32/ESP8266**: Should work with pin mapping adjustments
- **Faster Processors**: Recommended for higher speed operation

## OTPack Class Benefits

### 1. **Encapsulation & Clean Interface**
- All stepper motor functionality encapsulated in OTPack class
- Private implementation details hidden from user code
- Simple public API with intuitive method names
- No global variables or complex initialization sequences

### 2. **Plug & Play Design**
- Single constructor with pin assignments
- Default settings work out of the box
- Optional configuration methods for customization
- Single `update()` call handles everything

### 3. **Built-in Safety Features**
- Automatic sensor debouncing prevents false triggers
- State machine ensures predictable operation
- Motor enable/disable handled automatically
- Reset function for emergency stops

### 4. **Flexible Configuration**
- Runtime configuration of all motion parameters
- Independent forward and reverse profiles
- Configurable timing and positioning
- Easy parameter adjustment without code changes

### 5. **Maintainable Code Structure**
- Clear separation of concerns within class
- Private methods handle implementation details
- Public interface focuses on user needs
- Self-contained with minimal dependencies

## Future Improvement Suggestions
1. **Non-blocking Operation**: Replace `runToPosition()` with `run()` for better responsiveness
2. **Error Handling**: Add position feedback and error detection
3. **Configuration**: Make motion parameters configurable via serial commands
4. **Safety Features**: Add limit switches and emergency stop functionality
5. **Speed Optimization**: Verify maximum achievable speeds for target hardware
6. **Parameter Validation**: Add bounds checking for motion parameters
7. **Interrupt-Based Input**: Use interrupts for more responsive sensor handling

## OTPack Usage Examples

### Basic Usage - Blocking Mode
```cpp
#include "OTPack.h"

OTPack otpack(3, 10, 8, 9);  // sensor, step, dir, enable pins

void setup() {
    Serial.begin(9600);
    otpack.begin();  // Uses default BLOCKING mode
}

void loop() {
    otpack.update();  // Handles everything automatically
    // Program waits here during motion
}
```

### Basic Usage - Non-Blocking Mode
```cpp
void setup() {
    Serial.begin(9600);
    otpack.begin();
    otpack.setMotionMode(OTPack::NON_BLOCKING);  // Enable non-blocking
}

void loop() {
    otpack.update();  // Motion happens in background
    
    // Can do other tasks here while motor moves
    handleSerialInput();
    updateDisplay();
    checkOtherSensors();
}
```

### Mode Comparison Examples
```cpp
// BLOCKING - Simple but blocks execution
otpack.setMotionMode(OTPack::BLOCKING);
otpack.update();  // Waits for motion to complete
Serial.println("Motion finished");  // This waits

// NON_BLOCKING - Responsive but requires status checking
otpack.setMotionMode(OTPack::NON_BLOCKING);
otpack.update();  // Returns immediately
if (otpack.isMoving()) {
    Serial.println("Still moving...");  // This executes immediately
}
```

### Advanced Configuration
```cpp
void setup() {
    Serial.begin(9600);
    otpack.begin();
    
    // Motor configuration
    otpack.setMotorConfig(200, 8);  // 200 steps/rev, 8x microstepping
    
    // Motion profiles
    otpack.setForwardProfile(800.0, 400.0, 100);  // slower, smoother
    otpack.setReverseProfile(1500.0, 800.0, 200, 50);  // custom timing
    otpack.setPositionOffset(5);  // larger offset
    
    // Choose motion mode
    otpack.setMotionMode(OTPack::NON_BLOCKING);
}
```

### Status Monitoring & Control
```cpp
void loop() {
    otpack.update();
    
    // State checking
    if (otpack.getState() == OTPack::WAITING_FORWARD) {
        // Ready for forward motion
    }
    
    // Motion status (especially useful for non-blocking)
    if (otpack.isBusy()) {
        // System is busy (moving or settling)
    }
    
    if (otpack.isMoving()) {
        // Motor is actively moving right now
    }
    
    // Mode checking
    if (otpack.getMotionMode() == OTPack::NON_BLOCKING) {
        // In non-blocking mode
    }
}

// Runtime mode switching
void switchToNonBlocking() {
    otpack.setMotionMode(OTPack::NON_BLOCKING);
    Serial.println("Now in non-blocking mode");
}

// Emergency control
void emergencyStop() {
    otpack.reset();  // Stop motor, disable, reset to initial state
}
```

## Enhanced Usage Notes

### Integration
- **Simple Setup**: Just include OTPack.h and create instance
- **One-Line Operation**: Single `update()` call handles all functionality
- **Hardware Independence**: Works with any Arduino-compatible board

### Motion Modes
- **BLOCKING (Default)**: Simple, program waits for motion completion
- **NON_BLOCKING**: Advanced, allows multitasking during motion
- **Runtime Switching**: Can change modes anytime with `setMotionMode()`

### Configuration & Control
- **Easy Customization**: Optional methods for all parameters
- **Built-in Safety**: Automatic debouncing, state machine protection
- **Status Monitoring**: Multiple status checking methods available
- **Emergency Control**: Reset function for immediate stop and restart

### Best Practices
- **BLOCKING Mode**: Use for simple, single-task applications
- **NON_BLOCKING Mode**: Use when you need to do other tasks during motion
- **Status Checking**: Use `isMoving()` in non-blocking mode for real-time status
- **Error Recovery**: Always have `reset()` available for emergency situations

### Performance Notes
- **Timing**: Non-blocking mode provides better system responsiveness  
- **CPU Usage**: Blocking mode uses less CPU but limits multitasking
- **Memory**: Minimal overhead for both modes
- **Reliability**: Both modes handle debouncing and state management automatically