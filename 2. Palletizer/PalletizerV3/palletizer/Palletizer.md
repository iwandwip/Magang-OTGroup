# Palletizer Control System

## Project Structure

The Palletizer Control System has been structured into a proper Python package for better maintainability and organization. Here's the current directory structure:

```
PalletizerV3/
├── main.py                                # Main entry point for the application
└── palletizer/                            # Main package
    ├── __init__.py                        # Package initialization
    ├── Palletizer.md                      # Documentation
    ├── serial_communicator.py             # Serial communication handler
    ├── ui/                                # UI components subpackage
    │   ├── __init__.py                    # UI subpackage initialization
    │   ├── main_window.py                 # Main application window
    │   ├── slave_control_panel.py         # Control panel for individual slaves
    │   ├── monitor_panel.py               # Communication monitoring and logging panel
    │   ├── position_tracker.py            # Position tracking logic
    │   ├── position_display_widget.py     # Position display UI component
    │   └── sequence/                      # Sequence control subpackage
    │       ├── __init__.py                # Sequence subpackage initialization
    │       ├── sequence_panel.py          # Main sequence control panel
    │       ├── sequence_row_manager.py    # Row management for sequences
    │       ├── sequence_executor.py       # Sequence execution logic
    │       └── sequence_file_operations.py # Sequence file handling
    └── utils/                             # Utilities subpackage
        ├── __init__.py                    # Utilities subpackage initialization
        └── config.py                      # Configuration constants and settings
```

## Module Descriptions

### Main Entry Point

- **main.py**: Application entry point that creates the QApplication instance and launches the main window. Located outside the package for easier execution.

### Core Components

- **serial_communicator.py**: Handles serial communication with the palletizer hardware. Implements a threaded approach to send commands and receive feedback.

### UI Components

- **main_window.py**: The main application window that contains the tab widget and manages the overall UI layout.
- **slave_control_panel.py**: Provides control interfaces for individual axes/slaves (X, Y, Z, T, G) with absolute positioning.
- **monitor_panel.py**: Displays communication logs and allows sending manual commands.
- **position_tracker.py**: Tracks the position of each axis based on commands and feedback.
- **position_display_widget.py**: UI component for displaying current positions of all axes.

### Sequence Control Subpackage

The sequence control functionality has been refactored into a subpackage with multiple components:

- **sequence_panel.py**: Main sequence panel that integrates all sequence control functionality.
- **sequence_row_manager.py**: Handles sequence row operations (add, edit, delete, etc.).
- **sequence_executor.py**: Manages the execution of sequences, including step-by-step execution.
- **sequence_file_operations.py**: Handles saving, loading, and managing sequence files.

### Utilities

- **config.py**: Contains centralized configuration values, constants, and style settings for the entire application.

## Import Strategy

The application uses a combination of absolute and relative imports:

- **Absolute imports** are used in `main.py` since it's outside the package:

  ```python
  from palletizer.ui.main_window import PalletizerControlApp
  ```

- **Relative imports** are used within the package for importing from sibling modules:

  ```python
  from ..utils.config import *  # In UI modules importing from utils
  ```

- **Absolute imports from palletizer package** are used in `main_window.py`:

  ```python
  from palletizer.serial_communicator import SerialCommunicator
  from palletizer.ui.slave_control_panel import SlaveControlPanel
  ```

- **Imports for the sequence subpackage**:

  ```python
  # From main_window.py
  from palletizer.ui.sequence import SequencePanel

  # From within sequence_panel.py
  from .sequence_row_manager import SequenceRowManager
  from .sequence_executor import SequenceExecutor
  from .sequence_file_operations import SequenceFileManager
  ```

## Running the Application

To run the application, execute the `main.py` file from the project root directory:

```bash
python main.py
```

## Key Features

### 1. Modular UI Architecture

The UI is organized into reusable components, each with a specific responsibility:

- Main window manages overall layout and tabs
- Individual slave control panels handle axes control
- Sequence panel manages sequence creation and execution
- Monitor panel displays communication logs

### 2. Position Tracking System

The application includes a robust position tracking system:

- `position_tracker.py` maintains the absolute position of each axis
- Positions are updated based on commands sent and feedback received
- Positions are reset to zero when homing commands are executed
- Position displays are available in both Individual Control and Sequence Control tabs

### 3. Sequence Control System

The sequence control system has been refactored into a modular subpackage:

- Sequences can be created, edited, saved, and loaded
- Sequences can be executed in full or step-by-step
- Individual axes can be tested from sequence rows
- Position tracking is integrated with sequence execution

## Adding New Features

To add a new feature, you can:

1. Identify the appropriate location for the feature:

   - Is it related to a specific UI component? Add it to that component.
   - Is it a new UI component? Create a new file in the `ui` directory.
   - Is it sequence-related? Add it to the appropriate file in the `sequence` subpackage.

2. Update the imports in the relevant files to access the new functionality.

3. Connect any signals/slots needed for the feature to communicate with other components.

4. Update configuration in `config.py` if needed.

## Modifying Configuration

All configuration values are in `utils/config.py`. Update this file to change:

- Serial communication parameters
- UI appearance settings
- Default values
- Command formats

## Troubleshooting

### Import Errors

If you encounter import errors, check that:

- You're running the application from the correct directory
- All imports use the correct relative or absolute paths
- All dependencies are installed

### Communication Issues

If the application can't communicate with the hardware:

- Check the serial port settings in `config.py`
- Verify the hardware connections
- Check the communication protocol implementation

### Position Tracking Issues

If positions are not tracking correctly:

- Verify the command parsing in `position_tracker.py`
- Check that position signals are properly connected in `main_window.py`
- Ensure position displays are updated when positions change
