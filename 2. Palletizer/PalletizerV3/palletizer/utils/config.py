# Configuration values and constants for the Palletizer application

# Default serial settings
DEFAULT_BAUDRATE = 115200

# Available baudrates
BAUDRATES = [9600, 19200, 38400, 57600, 115200]

# Default slave IDs
SLAVE_IDS = ['x', 'y', 'z', 't', 'g']

# UI Settings
WINDOW_TITLE = "Palletizer Control System"
WINDOW_GEOMETRY = (100, 100, 1280, 720)

# Speed settings
MIN_SPEED = 100
MAX_SPEED = 2000
DEFAULT_SPEED = 1000
SPEED_STEP = 100

# Step settings
MIN_STEPS = 1
MAX_STEPS = 10000
DEFAULT_STEPS = 100

# Command types and formats
CMD_START = "START"
CMD_ZERO = "ZERO"
CMD_PAUSE = "PAUSE"
CMD_RESUME = "RESUME"
CMD_RESET = "RESET"
CMD_SPEED_FORMAT = "SPEED;{};{}"  # SPEED;slave_id;speed_value

# Style sheets
STATUS_IDLE = "color: blue; font-weight: bold;"
STATUS_MOVING = "color: green; font-weight: bold;"
STATUS_PAUSED = "color: orange; font-weight: bold;"
STATUS_STOPPED = "color: red; font-weight: bold;"
STATUS_HOMING = "color: orange; font-weight: bold;"
STATUS_SETTING = "color: purple; font-weight: bold;"

BUTTON_START = "background-color: #ccffcc; font-weight: bold;"
BUTTON_HOME = "background-color: #ffffcc;"
BUTTON_PAUSE = "background-color: #ffffcc;"
BUTTON_RESUME = "background-color: #ccffcc;"
BUTTON_STOP = "background-color: #ffcccc;"
BUTTON_SPEED = "background-color: #e0e0ff;"
