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
const byte sensorPin = 3;      // Sensor input (pullup enabled)
const byte stepPin = 10;       // Step pin to motor driver
const byte enablePin = 9;      // Motor enable pin
const byte directionPin = 8;   // Direction pin to motor driver
```

## Motor Configuration
- **Steps per revolution**: 58 steps (base motor)
- **Microstepping**: 4x resolution = 232 total steps per revolution
- **Interface**: AccelStepper::DRIVER (step/direction interface)

### Variable Names
- `microsteppingResolution`: Microstepping multiplier (4x)
- `stepsPerRevolution`: Total steps including microstepping
- `isExtended`: State tracking for extend/retract position

## Operation Logic
The system operates in two main states based on sensor input:

### Extend Motion (Sensor HIGH, not extended)
- **Speed**: 1200 steps/sec × 4 = 4800 steps/sec
- **Acceleration**: 600 steps/sec² × 4 = 2400 steps/sec²
- **Movement**: Full `stepsPerRevolution` forward
- **Delay**: 150ms before movement starts
- **Enable**: Motor enabled during operation
- **State**: Sets `isExtended = true`

### Retract Motion (Sensor LOW, extended)
- **Speed**: 3000 steps/sec × 4 = 12000 steps/sec
- **Acceleration**: 1900 steps/sec² × 4 = 7600 steps/sec²
- **Movement**: `stepsPerRevolution - 2` reverse (with 2-step adjustment)
- **Delay**: 250ms before movement, 100ms after completion
- **Enable**: Motor disabled after completion
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
3. **Sensor Integration**: Digital input `sensorPin` triggers motion sequences
4. **Power Management**: `enablePin` control for motor power saving
5. **Precise Positioning**: Microstepping for improved accuracy

## Code Structure
- Single-file Arduino sketch (`OT_PackV3.ino`)
- Setup function initializes pins and serial communication
- Main loop continuously monitors sensor state and controls motor
- State machine prevents multiple triggers and ensures proper sequencing

## Serial Monitoring
The system outputs sensor state via Serial for debugging:
```
| sensorPin: [0|1]
```

## Safety Considerations
- Motor enable pin prevents accidental movement when disabled
- Pull-up resistor on input prevents floating states
- State tracking prevents rapid toggling
- Blocking motion calls ensure completion before state changes