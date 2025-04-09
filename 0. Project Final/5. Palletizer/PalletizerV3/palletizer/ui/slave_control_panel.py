from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
                             QLabel, QPushButton, QGroupBox, QLineEdit, QSpinBox,
                             QCheckBox, QSlider, QSizePolicy)
from PyQt5.QtCore import Qt, pyqtSignal
from ..utils.config import *


class SlaveControlPanel(QWidget):
    """Panel kontrol untuk satu slave stepper dengan Absolute Positioning"""
    command_request = pyqtSignal(str)
    position_changed = pyqtSignal(str, int)  # Emits (axis_id, new_position) when position changes

    def __init__(self, slave_id, parent=None):
        super().__init__(parent)
        self.slave_id = slave_id
        self.current_speed = DEFAULT_SPEED
        self.current_position = 0  # Current calculated position
        self.target_position = 0   # Target position for movements
        self.setup_ui()

    def setup_ui(self):
        # Main layout
        layout = QVBoxLayout(self)
        layout.setContentsMargins(5, 5, 5, 5)  # Reduce margins

        # Group box untuk mengelompokkan kontrol
        group_box = QGroupBox(f"Slave {self.slave_id.upper()}")
        group_layout = QVBoxLayout()
        group_layout.setSpacing(5)  # Tighter spacing

        # Status indicator
        status_layout = QHBoxLayout()
        self.status_label = QLabel("Status:")
        self.status_value = QLabel("Idle")
        self.status_value.setStyleSheet(STATUS_IDLE)

        status_layout.addWidget(self.status_label)
        status_layout.addWidget(self.status_value)
        status_layout.addStretch()

        # Position display
        position_layout = QHBoxLayout()
        self.position_label = QLabel("Position:")
        self.position_value = QLabel("0")  # Current position
        self.position_value.setStyleSheet("font-family: monospace; font-weight: bold;")

        position_layout.addWidget(self.position_label)
        position_layout.addWidget(self.position_value)
        position_layout.addStretch()

        # Target position input
        target_layout = QHBoxLayout()
        target_layout.addWidget(QLabel("Target Pos:"))

        # Add SpinBox for setting the target position
        self.target_spinbox = QSpinBox()
        self.target_spinbox.setRange(-MAX_STEPS*100, MAX_STEPS*100)  # Wide range for absolute positioning
        self.target_spinbox.setValue(0)
        self.target_spinbox.setSingleStep(100)
        self.target_spinbox.valueChanged.connect(self.on_target_changed)

        self.goto_button = QPushButton("Go To")
        self.goto_button.clicked.connect(self.on_goto_clicked)
        self.goto_button.setStyleSheet(BUTTON_START)

        target_layout.addWidget(self.target_spinbox)
        target_layout.addWidget(self.goto_button)

        # Speed control
        speed_layout = QHBoxLayout()
        speed_layout.addWidget(QLabel("Speed:"))

        self.speed_slider = QSlider(Qt.Horizontal)
        self.speed_slider.setMinimum(MIN_SPEED)
        self.speed_slider.setMaximum(MAX_SPEED)
        self.speed_slider.setValue(DEFAULT_SPEED)
        self.speed_slider.setTickPosition(QSlider.TicksBelow)
        self.speed_slider.setTickInterval(SPEED_STEP)
        self.speed_slider.valueChanged.connect(self.update_speed_display)

        self.speed_spinbox = QSpinBox()
        self.speed_spinbox.setRange(MIN_SPEED, MAX_SPEED)
        self.speed_spinbox.setValue(DEFAULT_SPEED)
        self.speed_spinbox.valueChanged.connect(self.update_speed_slider)

        self.set_speed_btn = QPushButton("Set")
        self.set_speed_btn.clicked.connect(self.on_set_speed)
        self.set_speed_btn.setStyleSheet(BUTTON_SPEED)

        speed_layout.addWidget(self.speed_slider)
        speed_layout.addWidget(self.speed_spinbox)
        speed_layout.addWidget(self.set_speed_btn)

        # Command controls
        command_layout = QGridLayout()
        command_layout.setSpacing(5)  # Tighter spacing

        # Button 1: Start (CMD_START)
        self.start_btn = QPushButton("Start")
        self.start_btn.clicked.connect(self.on_start_clicked)
        self.start_btn.setToolTip("Start movement (CMD_START)")
        self.start_btn.setStyleSheet(BUTTON_START)

        # Button 2: Home (CMD_ZERO)
        self.home_btn = QPushButton("Home")
        self.home_btn.clicked.connect(self.on_home_clicked)
        self.home_btn.setToolTip("Home/Zero the axis (CMD_ZERO)")
        self.home_btn.setStyleSheet(BUTTON_HOME)

        # Button 3: Pause (CMD_PAUSE)
        self.pause_btn = QPushButton("Pause")
        self.pause_btn.clicked.connect(self.on_pause_clicked)
        self.pause_btn.setToolTip("Pause movement (CMD_PAUSE)")
        self.pause_btn.setStyleSheet(BUTTON_PAUSE)

        # Button 4: Resume (CMD_RESUME)
        self.resume_btn = QPushButton("Resume")
        self.resume_btn.clicked.connect(self.on_resume_clicked)
        self.resume_btn.setToolTip("Resume movement (CMD_RESUME)")
        self.resume_btn.setStyleSheet(BUTTON_RESUME)

        # Button 5: Reset/Stop (CMD_RESET)
        self.stop_btn = QPushButton("Stop/Reset")
        self.stop_btn.clicked.connect(self.on_stop_clicked)
        self.stop_btn.setToolTip("Stop all movement and reset (CMD_RESET)")
        self.stop_btn.setStyleSheet(BUTTON_STOP)

        # Add command buttons to layout
        command_layout.addWidget(self.start_btn, 0, 0, 1, 2)  # span 2 columns
        command_layout.addWidget(self.home_btn, 1, 0)
        command_layout.addWidget(self.pause_btn, 1, 1)
        command_layout.addWidget(self.resume_btn, 2, 0)
        command_layout.addWidget(self.stop_btn, 2, 1)

        # Custom command
        custom_layout = QHBoxLayout()
        self.custom_command = QLineEdit()
        self.custom_command.setPlaceholderText(f"Custom command untuk slave {self.slave_id}")
        self.send_custom_btn = QPushButton("Send")
        self.send_custom_btn.clicked.connect(self.on_send_custom)

        custom_layout.addWidget(self.custom_command)
        custom_layout.addWidget(self.send_custom_btn)

        # Tambahkan semua sub-layout ke group layout
        group_layout.addLayout(status_layout)
        group_layout.addLayout(position_layout)
        group_layout.addLayout(target_layout)
        group_layout.addLayout(speed_layout)
        group_layout.addLayout(command_layout)
        # Removed Steps section
        group_layout.addLayout(custom_layout)

        # Set group layout ke group box
        group_box.setLayout(group_layout)

        # Set size policy to allow expansion
        group_box.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Expanding)

        # Tambahkan group box ke layout utama
        layout.addWidget(group_box)

    def update_speed_display(self, value):
        self.speed_spinbox.setValue(value)
        self.current_speed = float(value)

    def update_speed_slider(self, value):
        self.speed_slider.setValue(value)
        self.current_speed = float(value)

    def on_target_changed(self, value):
        """Handle target position spinbox change"""
        self.target_position = value

    def on_goto_clicked(self):
        """Send command to go to the target position"""
        # Format: x(position) - absolute positioning
        move_cmd = f"{self.slave_id}({self.target_position})"
        self.command_request.emit(move_cmd)
        self.status_value.setText(f"Moving to {self.target_position}")
        self.status_value.setStyleSheet(STATUS_MOVING)

        # Update the position (will be confirmed by feedback)
        self.current_position = self.target_position
        self.position_value.setText(str(self.current_position))
        self.position_changed.emit(self.slave_id, self.current_position)

    def on_set_speed(self):
        # Format: SPEED;<slave_id>;<speed_value>
        command = CMD_SPEED_FORMAT.format(self.slave_id, self.current_speed)
        self.command_request.emit(command)
        self.status_value.setText(f"Setting Speed: {self.current_speed}")
        self.status_value.setStyleSheet(STATUS_SETTING)

    def on_start_clicked(self):
        # Sesuai dengan format yang diharapkan master (START)
        self.command_request.emit(CMD_START)
        self.status_value.setText("Starting...")
        self.status_value.setStyleSheet(STATUS_MOVING)

    def on_home_clicked(self):
        # Sesuai dengan format yang diharapkan master (ZERO)
        self.command_request.emit(CMD_ZERO)
        self.status_value.setText("Homing...")
        self.status_value.setStyleSheet(STATUS_HOMING)

        # Reset the position to zero
        self.current_position = 0
        self.target_position = 0
        self.target_spinbox.setValue(0)
        self.position_value.setText("0")
        self.position_changed.emit(self.slave_id, 0)

    def on_pause_clicked(self):
        # Sesuai dengan format yang diharapkan master (PAUSE)
        self.command_request.emit(CMD_PAUSE)
        self.status_value.setText("Paused")
        self.status_value.setStyleSheet(STATUS_PAUSED)

    def on_resume_clicked(self):
        # Sesuai dengan format yang diharapkan master (RESUME)
        self.command_request.emit(CMD_RESUME)
        self.status_value.setText("Resuming")
        self.status_value.setStyleSheet(STATUS_MOVING)

    def on_stop_clicked(self):
        # Sesuai dengan format yang diharapkan master (RESET)
        self.command_request.emit(CMD_RESET)
        self.status_value.setText("Stopped")
        self.status_value.setStyleSheet(STATUS_STOPPED)

    def on_send_custom(self):
        command = self.custom_command.text().strip()
        if command:
            self.command_request.emit(command)
            self.custom_command.clear()

            # If this is a position command, try to update the position
            if self.slave_id in command and '(' in command and ')' in command:
                try:
                    # Simple parsing for position commands like "x(100)"
                    if command.startswith(self.slave_id):
                        value_str = command.split('(')[1].split(')')[0]
                        # Handle simple direct movement
                        if value_str.isdigit() or (value_str.startswith('-') and value_str[1:].isdigit()):
                            position = int(value_str)
                            self.current_position = position
                            self.target_position = position
                            self.target_spinbox.setValue(position)
                            self.position_value.setText(str(position))
                            self.position_changed.emit(self.slave_id, position)
                except Exception as e:
                    # Silently fail on parsing errors
                    pass

    def update_status(self, message):
        if "POS:" in message:
            parts = message.split()
            for part in parts:
                if part.startswith("POS:"):
                    position = part.split(':')[1]
                    try:
                        # Update position from feedback
                        pos_value = int(position)
                        self.current_position = pos_value
                        self.position_value.setText(position)
                        self.target_spinbox.setValue(pos_value)
                        self.position_changed.emit(self.slave_id, pos_value)
                    except ValueError:
                        # Just update the display if conversion fails
                        self.position_value.setText(position)

        if "ZERO DONE" in message:
            self.status_value.setText("Homed")
            self.status_value.setStyleSheet(STATUS_IDLE)
            # Reset position on successful homing
            self.current_position = 0
            self.target_position = 0
            self.target_spinbox.setValue(0)
            self.position_value.setText("0")
            self.position_changed.emit(self.slave_id, 0)
        elif "PAUSE DONE" in message:
            self.status_value.setText("Paused")
            self.status_value.setStyleSheet(STATUS_PAUSED)
        elif "RESUME DONE" in message:
            self.status_value.setText("Resumed")
            self.status_value.setStyleSheet(STATUS_MOVING)
        elif "RESET DONE" in message:
            self.status_value.setText("Reset")
            self.status_value.setStyleSheet(STATUS_IDLE)
        elif "SEQUENCE COMPLETED" in message:
            self.status_value.setText("Idle")
            self.status_value.setStyleSheet(STATUS_IDLE)
        elif "MOVING" in message:
            self.status_value.setText("Moving")
            self.status_value.setStyleSheet(STATUS_MOVING)
        elif "DELAYING" in message:
            self.status_value.setText("Delaying")
            self.status_value.setStyleSheet(STATUS_SETTING)
        elif "SPEED SET TO" in message:
            speed_value = message.split("TO ")[1].strip()
            self.status_value.setText(f"Speed: {speed_value}")
            self.status_value.setStyleSheet(STATUS_IDLE)
            # Update sliders and spinbox to match the confirmation
            try:
                speed_float = float(speed_value)
                self.speed_slider.setValue(int(speed_float))
                self.speed_spinbox.setValue(int(speed_float))
                self.current_speed = speed_float
            except ValueError:
                pass

    def set_position(self, position):
        """External method to set the position"""
        self.current_position = position
        self.target_position = position
        self.target_spinbox.setValue(position)
        self.position_value.setText(str(position))
