from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
                             QLabel, QPushButton, QGroupBox, QLineEdit, QSpinBox,
                             QCheckBox, QSlider, QSizePolicy)
from PyQt5.QtCore import Qt, pyqtSignal
from ..utils.config import *


class SlaveControlPanel(QWidget):
    """Panel kontrol untuk satu slave stepper"""
    command_request = pyqtSignal(str)

    def __init__(self, slave_id, parent=None):
        super().__init__(parent)
        self.slave_id = slave_id
        self.current_speed = DEFAULT_SPEED
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
        self.position_label = QLabel("Posisi:")
        self.position_value = QLabel("0")
        self.position_value.setStyleSheet("font-family: monospace; font-weight: bold;")

        position_layout.addWidget(self.position_label)
        position_layout.addWidget(self.position_value)
        position_layout.addStretch()

        # Speed control - NEW SECTION
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

        # Movement controls
        movement_layout = QGridLayout()
        movement_layout.setSpacing(5)  # Tighter spacing

        self.move_positive_btn = QPushButton("Move +")
        self.move_positive_btn.clicked.connect(self.on_move_positive)

        self.move_negative_btn = QPushButton("Move -")
        self.move_negative_btn.clicked.connect(self.on_move_negative)

        self.steps_spinbox = QSpinBox()
        self.steps_spinbox.setRange(MIN_STEPS, MAX_STEPS)
        self.steps_spinbox.setValue(DEFAULT_STEPS)

        # Tambahkan widget ke layout
        movement_layout.addWidget(QLabel("Steps:"), 0, 0)
        movement_layout.addWidget(self.steps_spinbox, 0, 1)
        movement_layout.addWidget(self.move_positive_btn, 1, 0)
        movement_layout.addWidget(self.move_negative_btn, 1, 1)

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
        group_layout.addLayout(speed_layout)  # Add the new speed control
        group_layout.addLayout(command_layout)
        group_layout.addLayout(movement_layout)
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

    def on_move_positive(self):
        steps = self.steps_spinbox.value()

        # Format untuk gerakan koordinat setelah START
        # Format: x(position) - tanpa kecepatan karena konstan
        move_cmd = f"{self.slave_id}({steps})"

        self.command_request.emit(move_cmd)
        self.status_value.setText("Moving +")
        self.status_value.setStyleSheet(STATUS_MOVING)

    def on_move_negative(self):
        steps = -self.steps_spinbox.value()  # Negative steps for opposite direction

        # Format untuk gerakan koordinat setelah START
        # Format: x(-position) - tanpa kecepatan karena konstan
        move_cmd = f"{self.slave_id}({steps})"

        self.command_request.emit(move_cmd)
        self.status_value.setText("Moving -")
        self.status_value.setStyleSheet(STATUS_MOVING)

    def on_send_custom(self):
        command = self.custom_command.text().strip()
        if command:
            self.command_request.emit(command)
            self.custom_command.clear()

    def update_status(self, message):
        if "POS:" in message:
            parts = message.split()
            for part in parts:
                if part.startswith("POS:"):
                    position = part.split(':')[1]
                    self.position_value.setText(position)

        if "ZERO DONE" in message:
            self.status_value.setText("Homed")
            self.status_value.setStyleSheet(STATUS_IDLE)
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
