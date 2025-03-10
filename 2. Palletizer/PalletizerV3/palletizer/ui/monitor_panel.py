import time
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGroupBox,
                             QLabel, QPushButton, QLineEdit, QTextEdit, QCheckBox,
                             QSizePolicy)
from PyQt5.QtCore import pyqtSignal


class MonitorPanel(QWidget):
    """Panel untuk memonitor komunikasi dan log"""
    clear_logs = pyqtSignal()
    send_command = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setup_ui()

    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(5, 5, 5, 5)  # Reduce margins
        layout.setSpacing(5)  # Tighter spacing

        # Communication log
        log_group = QGroupBox("Communication Log")
        log_layout = QVBoxLayout()
        log_layout.setSpacing(5)  # Tighter spacing

        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        log_button_layout = QHBoxLayout()
        self.clear_log_btn = QPushButton("Clear Log")
        self.clear_log_btn.clicked.connect(self.on_clear_log)

        self.autoscroll_check = QCheckBox("Auto-scroll")
        self.autoscroll_check.setChecked(True)

        log_button_layout.addWidget(self.clear_log_btn)
        log_button_layout.addWidget(self.autoscroll_check)
        log_button_layout.addStretch()

        log_layout.addWidget(self.log_text)
        log_layout.addLayout(log_button_layout)

        log_group.setLayout(log_layout)

        # Manual command
        command_group = QGroupBox("Manual Command")
        command_layout = QHBoxLayout()
        command_layout.setSpacing(5)  # Tighter spacing

        self.command_input = QLineEdit()
        self.command_input.setPlaceholderText("Enter command to send to master")
        self.command_input.returnPressed.connect(self.on_send_command)

        self.send_btn = QPushButton("Send")
        self.send_btn.clicked.connect(self.on_send_command)

        command_layout.addWidget(self.command_input)
        command_layout.addWidget(self.send_btn)

        command_group.setLayout(command_layout)

        # Add to main layout
        layout.addWidget(log_group, 3)  # Give more space to log
        layout.addWidget(command_group, 1)

    def add_log(self, message, direction=""):
        timestamp = time.strftime("%H:%M:%S")

        if direction == "TX":
            formatted = f"<span style='color:blue'>[{timestamp}] TX: {message}</span>"
        elif direction == "RX":
            formatted = f"<span style='color:green'>[{timestamp}] RX: {message}</span>"
        else:
            formatted = f"<span style='color:black'>[{timestamp}] {message}</span>"

        self.log_text.append(formatted)

        if self.autoscroll_check.isChecked():
            scrollbar = self.log_text.verticalScrollBar()
            scrollbar.setValue(scrollbar.maximum())

    def on_clear_log(self):
        self.log_text.clear()
        self.clear_logs.emit()

    def on_send_command(self):
        command = self.command_input.text().strip()
        if not command:
            return

        # Emit signal with command data
        self.send_command.emit(command)
        self.command_input.clear()
