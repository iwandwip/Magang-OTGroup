from PyQt5.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                             QLabel, QPushButton, QComboBox, QTabWidget,
                             QMessageBox, QScrollArea, QGridLayout, QSizePolicy)
from PyQt5.QtCore import Qt
import serial.tools.list_ports

from palletizer.serial_communicator import SerialCommunicator
from palletizer.ui.slave_control_panel import SlaveControlPanel
from palletizer.ui.sequence import SequencePanel  # Updated import
from palletizer.ui.monitor_panel import MonitorPanel
from palletizer.ui.position_tracker import PositionTracker  # New import
from palletizer.utils.config import *


class PalletizerControlApp(QMainWindow):
    """Main application window"""

    def __init__(self):
        super().__init__()
        self.serial_thread = SerialCommunicator()
        self.available_ports = []
        self.slave_panels = {}

        # Initialize position tracker
        self.position_tracker = PositionTracker(self)

        self.setup_ui()
        self.init_connections()

    def setup_ui(self):
        self.setWindowTitle(WINDOW_TITLE)
        self.setGeometry(*WINDOW_GEOMETRY)

        # Set window to start maximized
        self.showMaximized()

        # Main widget and layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(5, 5, 5, 5)  # Reduce margins
        main_layout.setSpacing(5)  # Tighter spacing

        # Connection controls at top
        connection_layout = QHBoxLayout()
        connection_layout.setSpacing(5)  # Tighter spacing

        connection_layout.addWidget(QLabel("Port:"))
        self.port_combo = QComboBox()
        self.refresh_ports()
        connection_layout.addWidget(self.port_combo)

        connection_layout.addWidget(QLabel("Baudrate:"))
        self.baudrate_combo = QComboBox()
        for baud in BAUDRATES:
            self.baudrate_combo.addItem(str(baud))
        self.baudrate_combo.setCurrentText(str(DEFAULT_BAUDRATE))  # Default to 115200
        connection_layout.addWidget(self.baudrate_combo)

        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.on_connect)
        connection_layout.addWidget(self.connect_btn)

        self.refresh_btn = QPushButton("Refresh")
        self.refresh_btn.clicked.connect(self.refresh_ports)
        connection_layout.addWidget(self.refresh_btn)

        self.status_label = QLabel("Status: Disconnected")
        self.status_label.setStyleSheet("color: red;")
        connection_layout.addWidget(self.status_label)

        connection_layout.addStretch()

        # Tab widget for different panels
        self.tab_widget = QTabWidget()
        self.tab_widget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        # Control panel with all slave controls - use QScrollArea
        slave_scroll = QScrollArea()
        slave_scroll.setWidgetResizable(True)

        control_widget = QWidget()
        control_layout = QGridLayout(control_widget)
        control_layout.setContentsMargins(5, 5, 5, 5)  # Reduce margins
        control_layout.setSpacing(5)  # Tighter spacing

        # Create control panels for each slave
        for i, slave_id in enumerate(SLAVE_IDS):
            panel = SlaveControlPanel(slave_id)
            self.slave_panels[slave_id] = panel
            row, col = divmod(i, 3)  # 3 panels per row
            control_layout.addWidget(panel, row, col)

            # Connect panel's command_request signal to handle_slave_command
            panel.command_request.connect(self.handle_slave_command)

            # Connect panel's position_changed signal to position tracker
            panel.position_changed.connect(self.on_position_changed)

        # Add stretches to make the layout flexible
        for i in range(3):  # 3 columns
            control_layout.setColumnStretch(i, 1)

        slave_scroll.setWidget(control_widget)

        # Tabs
        self.tab_widget.addTab(slave_scroll, "Individual Control")

        # Sequence control panel
        self.sequence_panel = SequencePanel()
        self.sequence_panel.sequence_command.connect(self.handle_sequence_command)
        self.sequence_panel.global_command.connect(self.handle_global_command)
        self.tab_widget.addTab(self.sequence_panel, "Sequence Control")

        # Monitor panel
        self.monitor_panel = MonitorPanel()
        self.monitor_panel.send_command.connect(self.handle_manual_command)
        self.tab_widget.addTab(self.monitor_panel, "Monitor")

        # Add main components to layout
        main_layout.addLayout(connection_layout)
        main_layout.addWidget(self.tab_widget)

        # Setup status bar
        self.statusBar().showMessage("Ready")

    def init_connections(self):
        # Connect serial thread signals
        self.serial_thread.data_received.connect(self.handle_received_data)
        self.serial_thread.connection_status.connect(self.update_connection_status)

        # Connect position tracker signals
        self.position_tracker.position_updated.connect(self.on_tracker_position_updated)

        # Connect tab change signal to update positions
        self.tab_widget.currentChanged.connect(self.on_tab_changed)

        # Start the thread
        self.serial_thread.start()

    def refresh_ports(self):
        self.port_combo.clear()
        self.available_ports = [port.device for port in serial.tools.list_ports.comports()]
        if self.available_ports:
            for port in self.available_ports:
                self.port_combo.addItem(port)
        else:
            self.port_combo.addItem("No ports available")

    def on_connect(self):
        if self.serial_thread.is_connected:
            # Disconnect
            self.serial_thread.disconnect()
            self.connect_btn.setText("Connect")
            self.statusBar().showMessage("Disconnected")
        else:
            # Connect
            if not self.available_ports:
                QMessageBox.warning(self, "No Ports", "No serial ports available.")
                return

            port = self.port_combo.currentText()
            baudrate = int(self.baudrate_combo.currentText())

            if self.serial_thread.connect(port, baudrate):
                self.connect_btn.setText("Disconnect")
                self.statusBar().showMessage(f"Connected to {port} at {baudrate} baud")

    def update_connection_status(self, connected, message):
        if connected:
            self.status_label.setText(f"Status: {message}")
            self.status_label.setStyleSheet("color: green; font-weight: bold;")
        else:
            self.status_label.setText(f"Status: {message}")
            self.status_label.setStyleSheet("color: red;")
            self.connect_btn.setText("Connect")

    def handle_slave_command(self, command):
        """Handle commands from individual slave panels"""
        if self.serial_thread.is_connected:
            self.serial_thread.send_command(command)
            self.monitor_panel.add_log(command, "TX")

            # Update position tracker with the command
            self.position_tracker.parse_command(command)

    def handle_sequence_command(self, command):
        """Handle commands from sequence panel"""
        if self.serial_thread.is_connected:
            self.serial_thread.send_command(command)
            self.monitor_panel.add_log(command, "TX")

            # Update position tracker with the command
            self.position_tracker.parse_command(command)

    def handle_global_command(self, command):
        """Handle global commands like START, ZERO, PAUSE, etc."""
        if self.serial_thread.is_connected:
            self.serial_thread.send_command(command)
            self.monitor_panel.add_log(f"Global command: {command}", "TX")
            self.statusBar().showMessage(f"Sent global command: {command}")

            # Handle ZERO command specially - reset positions
            if command == CMD_ZERO:
                self.position_tracker.reset_all_positions()
                self.monitor_panel.add_log("Position tracker: All positions reset to zero", "INFO")

    def handle_manual_command(self, command):
        """Handle commands from manual command input"""
        if command and self.serial_thread.is_connected:
            self.serial_thread.send_command(command)
            self.monitor_panel.add_log(command, "TX")

            # Update position tracker with the command
            self.position_tracker.parse_command(command)

    def on_position_changed(self, axis_id, position):
        """Handle position changes from UI operations"""
        # Update the position in the position tracker
        self.position_tracker.set_position(axis_id, position)

        # Also update the position in the sequence panel's display
        self.sequence_panel.update_position(axis_id, position)

        # Log the update
        self.monitor_panel.add_log(f"Position update: {axis_id.upper()} to {position}", "INFO")

    def on_tracker_position_updated(self, axis_id, position):
        """Handle position updates from the position tracker"""
        # Update slave panel position
        if axis_id in self.slave_panels:
            self.slave_panels[axis_id].set_position(position)

        # Update sequence panel position display
        self.sequence_panel.update_position(axis_id, position)

    def on_tab_changed(self, index):
        """Handle tab changed event"""
        # When switching to the sequence panel, update all position displays
        if index == 1:  # Sequence Control tab
            for axis_id, position in self.position_tracker.get_all_positions().items():
                self.sequence_panel.update_position(axis_id, position)

    def handle_received_data(self, data):
        """Handle data received from serial port"""
        self.monitor_panel.add_log(data, "RX")

        # Update status bar with latest feedback
        if data.startswith("[FEEDBACK]"):
            feedback_msg = data[11:].strip()  # Remove [FEEDBACK] prefix
            self.statusBar().showMessage(f"Feedback: {feedback_msg}")

            # Check for ALL_SLAVES_COMPLETED feedback and handle it
            if feedback_msg == "ALL_SLAVES_COMPLETED":
                # Notify the sequence panel that all slaves have completed
                self.sequence_panel.handle_slave_completion()

        # Determine message type and route to appropriate handler
        if data.startswith("[FEEDBACK]"):
            # Feedback from master - already handled above
            pass
        elif data.startswith("[SLAVE]"):
            # Message from slave via master
            # Format should be: [SLAVE] slave_id;message
            parts = data[7:].split(';', 1)
            if len(parts) >= 2:
                slave_id = parts[0].lower()
                message = parts[1]

                if slave_id in self.slave_panels:
                    self.slave_panels[slave_id].update_status(message)

    def closeEvent(self, event):
        """Handle window close event"""
        # Stop the serial thread properly
        self.serial_thread.stop()
        event.accept()

    def resizeEvent(self, event):
        """Handle window resize event"""
        # Make any adjustments needed when window is resized
        super().resizeEvent(event)
