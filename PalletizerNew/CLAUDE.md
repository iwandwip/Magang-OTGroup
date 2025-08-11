# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **Palletizer Control System** built on Arduino platform, consisting of three main components that control an automated palletizing robot system:

1. **PalletizerArmControl** - Main controller handling RS485 communication, button inputs, and ARM coordination
2. **PalletizerArmDriver** - Individual stepper motor drivers for X,Y,Z,G,T axes with checksummed serial communication
3. **PalletizerCentralStateMachine** - Central state machine coordinating two robotic arms for palletizing operations

## System Architecture

### Communication Protocol
- **RS485 Communication**: Commands use XOR checksum validation with format `COMMAND*CHECKSUM`
- **Driver Identification**: Each driver has a unique ID (X=111, Y=110, Z=101, G=100, T=011) set via strap pins
- **Command Format**: `ARMx#COMMAND` where x is L(eft)/R(ight), commands include HOME, GLAD, PARK, CALI

### Hardware Configuration
- **ARM Detection**: PIN A4-A5 configuration determines if device is ARM1 or ARM2
- **Speed Selection**: A1-A2 pin connection determines motor speed (connected=high speed, disconnected=normal speed)
- **Button Control**: 4-second hold requirement for START/STOP buttons with capacitive touch detection
- **Sensor System**: 3 sensors for product/arm detection with conveyor control

### State Machine Design
The system uses a sophisticated state machine with these states:
- `ARM_IDLE` - Ready to receive commands
- `ARM_MOVING_TO_CENTER` - Moving to pickup position
- `ARM_IN_CENTER` - Positioned and waiting for product
- `ARM_PICKING` - Executing pickup/place operations
- `ARM_EXECUTING_SPECIAL` - Running calibration or park sequences
- `ARM_ERROR` - Error state with timeout recovery

## Development Commands

### Build and Upload
```bash
# Use Arduino IDE or Platform.IO
# Each .ino file should be compiled separately for its target Arduino board
```

### Serial Communication Testing
- **Baud Rate**: 9600 for all serial communications
- **Control Commands**: Available in SLEEPING state via USB serial
- **Debug Commands**: `debug_mode = true` enables verbose state logging

## Key Technical Details

### Command Generation
- **Layer Calculation**: Z positions calculated as `Z1 - layer * H` where H is product height
- **Position Mapping**: Odd/even layers use different coordinate sets (XO/YO vs XE/YE)
- **ARM Offsets**: Left/Right arms have individual offset parameters for precise positioning

### Safety Features
- **Retry Mechanism**: Up to 7 retries for failed motor commands with 200ms delays
- **Timeout Protection**: Movement (15s) and picking (15s) timeouts prevent system lockup
- **Checksum Validation**: All RS485 commands validated with XOR checksums
- **Debounced Inputs**: Button presses require 4-second hold to prevent accidental activation

### Parameter Management
- **EEPROM Storage**: System parameters saved to EEPROM with magic number and checksum validation
- **Default Values**: Comprehensive default parameter set for immediate operation
- **DIP Switch Configuration**: ARM starting layers configurable via hardware DIP switches

### Motor Control
- **Dynamic Speed**: Z-axis uses distance-based speed calculation `100*sqrt(distance)`
- **Homing Sequence**: Multi-step homing with limit switch detection and position verification
- **Acceleration Control**: Speed-dependent acceleration for smooth motion profiles

## Important Implementation Notes

- **State Transitions**: Always use `changeArmState()` function to ensure proper state logging
- **Command Timing**: Minimum 100ms interval between motor commands to prevent overwhelming drivers
- **Error Recovery**: System includes auto-recovery mechanisms but may require manual intervention for hardware failures
- **Memory Optimization**: Uses PROGMEM for string constants and efficient buffer management

## Debugging and Monitoring

Enable debug mode by setting `debug_mode = true` for:
- State transition logging
- Command retry information  
- Sensor state monitoring
- Timing information for operations

The system provides comprehensive serial output for troubleshooting hardware and timing issues.