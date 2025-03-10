# Palletizer Control System

## Project Structure

The Palletizer Control System has been restructured into a proper Python package for better maintainability and organization. Here's the directory structure:

```
PalletizerV3/
├── main.py                        # Main entry point for the application
└── palletizer/                    # Main package
    ├── __init__.py                # Package initialization
    ├── serial_communicator.py     # Serial communication handler
    ├── ui/                        # UI components subpackage
    │   ├── __init__.py            # UI subpackage initialization
    │   ├── main_window.py         # Main application window
    │   ├── slave_control_panel.py # Control panel for individual slaves
    │   ├── sequence_panel.py      # Sequence programming and execution panel
    │   └── monitor_panel.py       # Communication monitoring and logging panel
    └── utils/                     # Utilities subpackage
        ├── __init__.py            # Utilities subpackage initialization
        └── config.py              # Configuration constants and settings
```

## Module Descriptions

### Main Entry Point

- **main.py**: Application entry point that creates the QApplication instance and launches the main window. Located outside the package for easier execution.

### Core Components

- **serial_communicator.py**: Handles serial communication with the palletizer hardware. Implements a threaded approach to send commands and receive feedback.

### UI Components

- **main_window.py**: The main application window that contains the tab widget and manages the overall UI layout.
- **slave_control_panel.py**: Provides control interfaces for individual axes/slaves (X, Y, Z, T, G).
- **sequence_panel.py**: Allows creating, editing, saving, and executing sequences of movements.
- **monitor_panel.py**: Displays communication logs and allows sending manual commands.

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

## Running the Application

To run the application, execute the `main.py` file from the project root directory:

```bash
python main.py
```

## Benefits of This Structure

1. **Modularity**: Each component is in its own file, making the code more maintainable.

2. **Separation of Concerns**: UI components, serial communication, and configuration are kept separate.

3. **Easier Maintenance**: Individual files are smaller and focused on a single responsibility.

4. **Centralized Configuration**: Constants and styles are defined in a single location.

5. **Scalability**: New features can be added as new modules without affecting existing code.

6. **Code Reuse**: Components can be imported and reused in other projects.

7. **Improved Collaboration**: Multiple developers can work on different modules simultaneously.

## Common Development Tasks

### Adding a New Feature

To add a new feature, you can:

1. Create a new module in the appropriate subpackage
2. Update the imports in the relevant files
3. Integrate the new functionality with the existing components

### Modifying Configuration

All configuration values are in `utils/config.py`. Update this file to change:

- Serial communication parameters
- UI appearance settings
- Default values
- Command formats

### Adding a New Control Panel

To add a new control panel:

1. Create a new panel class in the `ui` subpackage
2. Add it to the tab widget in `main_window.py`
3. Connect its signals to the appropriate handlers

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
