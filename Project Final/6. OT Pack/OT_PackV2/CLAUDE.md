# OT Pack V2 Project Documentation

## Project Overview
OT_PackV2 is a stepper motor control system for packaging automation. The system controls a single stepper motor with acceleration/deceleration profiles using the AccelStepper library. The improved version features better code organization, clear naming conventions, and structured program flow.

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

## Software Architecture (Improved Version)

### Core Components
1. **AccelStepper Library Integration**: Uses DRIVER mode for step/direction control
2. **Enum-Based State Machine**: Structured state management using `PackingState` enum
3. **Function-Based Organization**: Clear separation of concerns with dedicated functions
4. **Constant Definitions**: All magic numbers replaced with named constants

### Improved Code Structure
```cpp
enum PackingState {
  WAITING_FOR_FORWARD,
  WAITING_FOR_REVERSE
};
```

### Function Organization
- `initializePins()` - Hardware pin configuration
- `initializeMotor()` - Motor setup and default parameters  
- `executeForwardMotion()` - Forward motion sequence
- `executeReverseMotion()` - Reverse motion sequence
- `configureMotorForForward()` - Forward motion parameters
- `configureMotorForReverse()` - Reverse motion parameters
- `enableMotor()` / `disableMotor()` - Motor control functions
- `printSensorStatus()` - Enhanced debugging output

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

## Code Improvements Implemented

### 1. **Enhanced Variable Naming**
- Replaced generic names (`P1_Input`, `P2_Output`) with descriptive constants
- All pin assignments use clear, self-documenting names
- Motion parameters have meaningful constant names

### 2. **Structured Program Flow**
- Enum-based state machine replaces simple boolean state
- Switch-case structure for cleaner state handling
- Function-based organization with single responsibility principle

### 3. **Better Code Organization**
- Separated initialization into dedicated functions
- Motion sequences encapsulated in specific functions
- Motor configuration separated from execution logic

### 4. **Improved Debugging**
- Enhanced serial output with descriptive status messages
- State information displayed in readable format
- Motion execution logging for better troubleshooting

### 5. **Maintainability Enhancements**
- All magic numbers replaced with named constants
- Consistent naming conventions throughout codebase
- Clear separation between configuration and logic

## Future Improvement Suggestions
1. **Non-blocking Operation**: Replace `runToPosition()` with `run()` for better responsiveness
2. **Error Handling**: Add position feedback and error detection
3. **Configuration**: Make motion parameters configurable via serial commands
4. **Safety Features**: Add limit switches and emergency stop functionality
5. **Speed Optimization**: Verify maximum achievable speeds for target hardware
6. **Parameter Validation**: Add bounds checking for motion parameters
7. **Interrupt-Based Input**: Use interrupts for more responsive sensor handling

## Enhanced Usage Notes
- Motor must be properly connected and powered before operation
- Input sensor should provide clean digital signals for reliable operation
- Enable pin controls motor driver power - ensure proper wiring and power supply
- Serial monitor provides comprehensive system status and debugging information
- All timing parameters can be easily adjusted via named constants
- State machine ensures predictable and safe operation sequences