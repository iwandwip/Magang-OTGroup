# OT Pack V3 Project Documentation

## Project Overview
OT_PackV3 is an Arduino-based stepper motor control system designed for packaging automation. The project uses the AccelStepper library to control a stepper motor with precise positioning and smooth acceleration/deceleration profiles.

## Hardware Configuration
- **Microcontroller**: Arduino (compatible board)
- **Stepper Motor**: Uses microstepping with 4x resolution
- **Motor Driver**: External stepper driver (DRIVER interface type)
- **Input Sensor**: Digital input on pin 3 (with pullup)
- **Enable Control**: Motor enable/disable functionality

### Pin Assignments
```cpp
const byte SENSOR_PIN = 3;      // Sensor input (pullup enabled)
const byte STEP_PIN = 10;       // Step pin to motor driver
const byte ENABLE_PIN = 9;      // Motor enable pin
const byte DIRECTION_PIN = 8;   // Direction pin to motor driver
```

## Motor Configuration
- **Steps per revolution**: 58 steps (base motor)
- **Microstepping**: 4x resolution = 232 total steps per revolution
- **Interface**: AccelStepper::DRIVER (step/direction interface)

### Constants and Variables
**Constants (UPPER_CASE):**
- `MICROSTEPPING_RESOLUTION`: Microstepping multiplier (4x)
- `BASE_STEPS_PER_REV`: Base motor steps per revolution (58)
- `SENSOR_PIN`, `STEP_PIN`, `ENABLE_PIN`, `DIRECTION_PIN`: Hardware pin assignments

**Variables (camelCase):**
- `stepsPerRevolution`: Total steps including microstepping (calculated)
- `isExtended`: State tracking for extend/retract position

**Configurable Motion Parameters:**
- `extendMaxSpeed`: Base speed for extend motion (default: 1200.0 steps/sec)
- `extendAcceleration`: Base acceleration for extend motion (default: 600.0 steps/sec²)
- `extendDelayBefore`: Delay before extend motion (default: 150ms)
- `retractMaxSpeed`: Base speed for retract motion (default: 3000.0 steps/sec)
- `retractAcceleration`: Base acceleration for retract motion (default: 1900.0 steps/sec²)
- `retractDelayBefore`: Delay before retract motion (default: 250ms)
- `retractDelayAfter`: Delay after retract motion (default: 100ms)
- `retractStepAdjustment`: Step adjustment for retract motion (default: 2)

## Operation Logic
The system operates in two main states based on sensor input:

### Extend Motion (SENSOR_PIN HIGH, not extended)
- **Speed**: 1200 × `MICROSTEPPING_RESOLUTION` = 4800 steps/sec
- **Acceleration**: 600 × `MICROSTEPPING_RESOLUTION` = 2400 steps/sec²
- **Movement**: Full `stepsPerRevolution` forward
- **Delay**: 150ms before movement starts
- **Enable**: `ENABLE_PIN` set HIGH during operation
- **State**: Sets `isExtended = true`

### Retract Motion (SENSOR_PIN LOW, extended)
- **Speed**: 3000 × `MICROSTEPPING_RESOLUTION` = 12000 steps/sec
- **Acceleration**: 1900 × `MICROSTEPPING_RESOLUTION` = 7600 steps/sec²
- **Movement**: `stepsPerRevolution - 2` reverse (with 2-step adjustment)
- **Delay**: 250ms before movement, 100ms after completion
- **Enable**: `ENABLE_PIN` set LOW after completion
- **State**: Sets `isExtended = false`

## Dependencies
- **AccelStepper Library** (v1.64)
  - Location: `/mnt/d/User/source/caktin_ws/KinematrixLibraries/Libraries/AccelStepper`
  - Author: Mike McCauley
  - Provides smooth acceleration/deceleration control
  - Non-blocking stepper motor control
  - Supports multiple motor interface types

## Key Features
1. **State-based Control**: Tracks extend/retract states via `isExtended` to prevent unwanted movement
2. **Smooth Motion**: Uses acceleration/deceleration for smooth operation
3. **Sensor Integration**: Digital input `SENSOR_PIN` triggers motion sequences
4. **Power Management**: `ENABLE_PIN` control for motor power saving
5. **Precise Positioning**: `MICROSTEPPING_RESOLUTION` for improved accuracy
6. **Runtime Configuration**: SerialCommander for real-time parameter adjustment
7. **Enhanced Debugging**: Improved serial output with motion status and state information

## Code Structure
- Single-file Arduino sketch (`OT_PackV3.ino`)
- `setup()`: Initializes pins, serial communication, and displays welcome message
- `loop()`: Main execution loop with serial command processing and motion control
- `serialCommander()`: Handles real-time configuration via serial commands
- State machine prevents multiple triggers and ensures proper sequencing

## Serial Commander Interface
The system provides a comprehensive serial interface for configuration and monitoring:

### Status Output (every 500ms):
```
| SENSOR_PIN: [0|1] | State: [Extended|Retracted]
```

### Motion Feedback:
```
Starting extend motion...
Extend motion completed.
Starting retract motion...
Retract motion completed.
```

### Available Commands:
- `HELP` - Show all available commands
- `SHOW` - Display current configuration
- `SET EXTEND_SPEED=value` - Set extend motion speed
- `SET EXTEND_ACCEL=value` - Set extend acceleration
- `SET EXTEND_DELAY=value` - Set extend delay (ms)
- `SET RETRACT_SPEED=value` - Set retract motion speed
- `SET RETRACT_ACCEL=value` - Set retract acceleration
- `SET RETRACT_DELAY_BEFORE=value` - Set retract delay before (ms)
- `SET RETRACT_DELAY_AFTER=value` - Set retract delay after (ms)
- `SET RETRACT_ADJUSTMENT=value` - Set retract step adjustment

### Command Examples:
```
SET EXTEND_SPEED=1500
SET RETRACT_ACCEL=2500
SHOW
HELP
```

## Safety Considerations
- `ENABLE_PIN` prevents accidental movement when disabled
- Pull-up resistor on `SENSOR_PIN` prevents floating states
- `isExtended` state tracking prevents rapid toggling
- Blocking motion calls ensure completion before state changes

## Coding Standards Applied
- **Constants**: UPPER_CASE with underscores (e.g., `SENSOR_PIN`, `MICROSTEPPING_RESOLUTION`)
- **Variables**: camelCase (e.g., `isExtended`, `stepsPerRevolution`, `extendMaxSpeed`)
- **Functions**: camelCase (Arduino standard)
- **Clear naming**: Descriptive names that explain purpose
- **Configurable parameters**: Clear naming with motion type prefix (extend/retract)

## Usage Examples

### Basic Operation:
1. Upload code to Arduino
2. Open Serial Monitor (9600 baud)
3. Type `HELP` to see available commands
4. Use sensor to trigger extend/retract motions

### Runtime Configuration:
```
// Check current settings
SHOW

// Increase extend speed
SET EXTEND_SPEED=1800

// Reduce retract delays
SET RETRACT_DELAY_BEFORE=150
SET RETRACT_DELAY_AFTER=50

// Verify changes
SHOW
```

### Troubleshooting:
- Monitor motion feedback messages
- Check sensor state in status output
- Verify motor enable/disable states
- Adjust parameters via SerialCommander without re-uploading code