from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QGroupBox,
                             QLabel, QPushButton, QLineEdit, QCheckBox, QTabWidget,
                             QFrame, QScrollArea, QComboBox, QMessageBox, QSizePolicy)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont

from ..utils.config import *


class CommunicationSettingsPanel(QWidget):
    config_updated = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)

        self.command_settings = {
            'START': 'START',
            'HOME': 'ZERO',
            'PAUSE': 'PAUSE',
            'RESUME': 'RESUME',
            'RESET': 'RESET',
            'SPEED_FORMAT': 'SPEED;{};{}',
            'COMPLETE_FEEDBACK': 'ALL_SLAVES_COMPLETED'
        }

        self.setup_ui()
        self.load_settings()

    def setup_ui(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.setSpacing(5)

        # Title and buttons at top - similar to what's shown in the screenshot
        header_layout = QHBoxLayout()

        # Title on left
        title_label = QLabel("Communication Settings")
        title_label.setStyleSheet("font-weight: bold; font-size: 14px;")
        header_layout.addWidget(title_label)

        header_layout.addStretch()

        # Buttons on right
        self.save_btn = QPushButton("Save Settings")
        self.save_btn.clicked.connect(self.save_settings)
        # Match the green style seen in the Start buttons
        self.save_btn.setStyleSheet("background-color: #ccffcc; color: #006400; font-weight: bold;")
        header_layout.addWidget(self.save_btn)

        self.reset_btn = QPushButton("Reset to Defaults")
        self.reset_btn.clicked.connect(self.reset_to_defaults)
        # Match the red style seen in the Stop/Reset buttons
        self.reset_btn.setStyleSheet("background-color: #ffcccc; color: #800000; font-weight: bold;")
        header_layout.addWidget(self.reset_btn)

        main_layout.addLayout(header_layout)

        # Main content in a scroll area
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setFrameShape(QFrame.NoFrame)

        settings_container = QWidget()
        settings_layout = QVBoxLayout(settings_container)
        settings_layout.setContentsMargins(0, 0, 0, 0)
        settings_layout.setSpacing(15)

        # Global command settings group
        command_group = QGroupBox("Global Command Settings")
        command_layout = QGridLayout()
        command_layout.setColumnStretch(1, 1)

        commands = [
            ("START Button:", "START", "Command sent when START button is pressed"),
            ("HOME Button:", "HOME", "Command sent when HOME button is pressed"),
            ("PAUSE Button:", "PAUSE", "Command sent when PAUSE button is pressed"),
            ("RESUME Button:", "RESUME", "Command sent when RESUME button is pressed"),
            ("STOP/RESET Button:", "RESET", "Command sent when STOP/RESET button is pressed"),
        ]

        row = 0
        self.command_inputs = {}

        for label_text, key, tooltip in commands:
            label = QLabel(label_text)
            label.setToolTip(tooltip)

            command_input = QLineEdit()
            command_input.setToolTip(tooltip)
            self.command_inputs[key] = command_input

            command_layout.addWidget(label, row, 0)
            command_layout.addWidget(command_input, row, 1)
            row += 1

        speed_label = QLabel("Speed Command Format:")
        speed_label.setToolTip("Format for speed commands. Use {} as placeholders for slave_id and speed_value")

        self.speed_format_input = QLineEdit()
        self.speed_format_input.setToolTip("Format: Use {} as placeholders for slave_id and speed_value")

        command_layout.addWidget(speed_label, row, 0)
        command_layout.addWidget(self.speed_format_input, row, 1)

        command_group.setLayout(command_layout)
        settings_layout.addWidget(command_group)

        # Feedback settings group
        feedback_group = QGroupBox("Feedback Settings")
        feedback_layout = QGridLayout()
        feedback_layout.setColumnStretch(1, 1)

        complete_label = QLabel("Completion Feedback:")
        complete_label.setToolTip("Feedback string that indicates all slaves have completed their tasks")

        self.complete_feedback_input = QLineEdit()
        self.complete_feedback_input.setToolTip("String that the master sends when all operations are complete")

        feedback_layout.addWidget(complete_label, 0, 0)
        feedback_layout.addWidget(self.complete_feedback_input, 0, 1)

        protocol_frame = QFrame()
        protocol_frame.setFrameShape(QFrame.StyledPanel)
        protocol_frame.setStyleSheet("background-color: #f8f8f8; border: 1px solid #dddddd;")

        protocol_layout = QVBoxLayout(protocol_frame)
        protocol_layout.setContentsMargins(10, 10, 10, 10)

        protocol_title = QLabel("Communication Protocol")
        protocol_title.setStyleSheet("font-weight: bold;")
        protocol_layout.addWidget(protocol_title)

        protocol_text = QLabel(
            "• Master → GUI: Commands are sent via serial connection\n"
            "• GUI → Master: [FEEDBACK] prefix is used for feedback messages\n"
            "• Master → Slave: Commands are forwarded with slave_id prefix\n"
            "• Slave → Master → GUI: [SLAVE] prefix used for slave feedback\n\n"
            "For automatic sequence execution, the completion feedback string\n"
            "is used to trigger the next row."
        )
        protocol_text.setWordWrap(True)
        protocol_layout.addWidget(protocol_text)

        feedback_layout.addWidget(protocol_frame, 1, 0, 1, 2)

        feedback_group.setLayout(feedback_layout)
        settings_layout.addWidget(feedback_group)

        # Test communication group
        test_group = QGroupBox("Test Communication")
        test_layout = QVBoxLayout()

        test_command_layout = QHBoxLayout()
        test_command_layout.addWidget(QLabel("Test Command:"))

        self.test_command_input = QLineEdit()
        test_command_layout.addWidget(self.test_command_input)

        self.send_test_btn = QPushButton("Send Test Command")
        self.send_test_btn.clicked.connect(self.send_test_command)
        self.send_test_btn.setStyleSheet(BUTTON_SPEED)  # Use the predefined style
        test_command_layout.addWidget(self.send_test_btn)

        test_layout.addLayout(test_command_layout)

        test_info = QLabel(
            "Use this section to test custom commands. Enter a command and press 'Send Test Command'\n"
            "to send it directly to the master controller. This is useful for testing new commands\n"
            "or debugging communication issues."
        )
        test_info.setWordWrap(True)
        test_info.setStyleSheet("color: #666666; font-style: italic;")
        test_layout.addWidget(test_info)

        test_group.setLayout(test_layout)
        settings_layout.addWidget(test_group)

        # Add stretch to push everything to the top
        settings_layout.addStretch()

        scroll_area.setWidget(settings_container)
        main_layout.addWidget(scroll_area)

    def load_settings(self):
        self.command_inputs["START"].setText(CMD_START)
        self.command_inputs["HOME"].setText(CMD_ZERO)
        self.command_inputs["PAUSE"].setText(CMD_PAUSE)
        self.command_inputs["RESUME"].setText(CMD_RESUME)
        self.command_inputs["RESET"].setText(CMD_RESET)
        self.speed_format_input.setText(CMD_SPEED_FORMAT)
        self.complete_feedback_input.setText("ALL_SLAVES_COMPLETED")

        self.update_command_settings()

    def update_command_settings(self):
        self.command_settings["START"] = self.command_inputs["START"].text()
        self.command_settings["HOME"] = self.command_inputs["HOME"].text()
        self.command_settings["PAUSE"] = self.command_inputs["PAUSE"].text()
        self.command_settings["RESUME"] = self.command_inputs["RESUME"].text()
        self.command_settings["RESET"] = self.command_inputs["RESET"].text()
        self.command_settings["SPEED_FORMAT"] = self.speed_format_input.text()
        self.command_settings["COMPLETE_FEEDBACK"] = self.complete_feedback_input.text()

    def save_settings(self):
        self.update_command_settings()
        self.config_updated.emit()
        QMessageBox.information(self, "Settings Saved",
                                "Communication settings have been saved and applied.")

    def reset_to_defaults(self):
        reply = QMessageBox.question(
            self,
            "Reset Settings",
            "Are you sure you want to reset all communication settings to defaults?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )

        if reply == QMessageBox.Yes:
            self.command_inputs["START"].setText(CMD_START)
            self.command_inputs["HOME"].setText(CMD_ZERO)
            self.command_inputs["PAUSE"].setText(CMD_PAUSE)
            self.command_inputs["RESUME"].setText(CMD_RESUME)
            self.command_inputs["RESET"].setText(CMD_RESET)
            self.speed_format_input.setText(CMD_SPEED_FORMAT)
            self.complete_feedback_input.setText("ALL_SLAVES_COMPLETED")

            self.update_command_settings()
            self.config_updated.emit()

            QMessageBox.information(self, "Settings Reset",
                                    "Communication settings have been reset to defaults.")

    def send_test_command(self):
        command = self.test_command_input.text().strip()
        if not command:
            QMessageBox.warning(self, "Empty Command",
                                "Please enter a command to send.")
            return

        self.parent().handle_manual_command(command)

        QMessageBox.information(self, "Command Sent",
                                f"Test command '{command}' has been sent.")
