# OT Pack V2 Project Documentation

## Project Overview
OT_PackV2 is a stepper motor control system for packaging automation. The system controls a single stepper motor with acceleration/deceleration profiles using the AccelStepper library.

## Hardware Configuration
- **Microcontroller**: Arduino compatible board
- **Motor**: Stepper motor with microstepping capability
- **Microstepping Resolution**: 4x (configurable via `MicrosteppingResolution` constant)
- **Motor Steps**: 58 full steps per revolution (configured for specific motor model)

### Pin Mapping
```
P1_Input (Pin 3)    - Digital input with pull-up (trigger sensor)
P2_Output (Pin 10)  - Step pulse output to motor driver
Enable (Pin 9)      - Motor enable/disable control
Clockwise (Pin 8)   - Direction control pin
```

## Software Architecture

### Core Components
1. **AccelStepper Library Integration**: Uses DRIVER mode for step/direction control
2. **State Machine**: Boolean state tracking for forward/reverse operation
3. **Motion Profiles**: Different speed/acceleration settings for each direction

### Motion Control Logic
The system operates in two distinct motion phases:

#### Forward Motion (state = false → true)
- Triggered when P1_Input goes HIGH
- **Speed**: 1200 steps/sec × microstepping resolution = 4800 steps/sec
- **Acceleration**: 600 steps/sec² × microstepping resolution = 2400 steps/sec²
- **Distance**: 58 × 4 = 232 steps
- **Behavior**: Blocking motion using `runToPosition()`
- **Enable Control**: Motor enabled during motion

#### Reverse Motion (state = true → false)
- Triggered when P1_Input goes LOW
- **Speed**: 3000 steps/sec × microstepping resolution = 12000 steps/sec
- **Acceleration**: 1900 steps/sec² × microstepping resolution = 7600 steps/sec²
- **Distance**: -(232 - 2) = -230 steps (slight offset for positioning accuracy)
- **Behavior**: Blocking motion using `runToPosition()`
- **Enable Control**: Motor disabled after motion completes

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

## Configuration Parameters

### Motor Specifications
```cpp
const int MicrosteppingResolution = 4;  // 1/4 step resolution
int Steps = 58 * MicrosteppingResolution;  // 232 total steps
```

### Motion Profiles
| Parameter | Forward | Reverse | Units |
|-----------|---------|---------|--------|
| Max Speed | 4800 | 12000 | steps/sec |
| Acceleration | 2400 | 7600 | steps/sec² |
| Distance | +232 | -230 | steps |
| Delay | 150ms | 250ms | milliseconds |

## Operational Flow
1. **Initialization**: Setup pins, serial communication, and AccelStepper object
2. **Monitoring Loop**: Continuously check P1_Input state
3. **Forward Trigger**: Input HIGH → Configure forward motion → Execute → Enable motor
4. **Reverse Trigger**: Input LOW → Configure reverse motion → Execute → Disable motor
5. **State Update**: Toggle boolean state after each complete motion

## Dependencies
- **AccelStepper.h**: Primary motion control library
- **Arduino Core**: digitalRead, digitalWrite, pinMode, Serial functions

## Hardware Compatibility
- **Arduino Uno/Nano**: Compatible (tested configuration)
- **ESP32/ESP8266**: Should work with pin mapping adjustments
- **Faster Processors**: Recommended for higher speed operation

## Potential Improvements
1. **Non-blocking Operation**: Replace `runToPosition()` with `run()` for better responsiveness
2. **Error Handling**: Add position feedback and error detection
3. **Configuration**: Make motion parameters configurable via serial commands
4. **Safety Features**: Add limit switches and emergency stop functionality
5. **Speed Optimization**: Verify maximum achievable speeds for target hardware

## Usage Notes
- Motor must be properly connected and powered before operation
- Input sensor should provide clean digital signals
- Enable pin controls motor driver power - ensure proper wiring
- Serial monitor can be used for debugging and monitoring system state