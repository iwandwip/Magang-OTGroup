import sys
import time
import serial
import serial.tools.list_ports
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout,
                             QHBoxLayout, QGridLayout, QLabel, QPushButton,
                             QComboBox, QTabWidget, QGroupBox, QLineEdit,
                             QTextEdit, QSpinBox, QDoubleSpinBox, QCheckBox,
                             QFrame, QProgressBar, QMessageBox, QSlider)
from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QThread
from PyQt5.QtGui import QFont, QIcon, QColor


class SerialCommunicator(QThread):
    """Thread class untuk menangani komunikasi serial"""
    data_received = pyqtSignal(str)
    connection_status = pyqtSignal(bool, str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.serial_port = None
        self.is_connected = False
        self.running = True
        self.tx_queue = []

    def connect(self, port, baudrate):
        try:
            self.serial_port = serial.Serial(port, baudrate, timeout=0.5)
            self.is_connected = True
            self.connection_status.emit(True, f"Terhubung ke {port}")
            return True
        except Exception as e:
            self.connection_status.emit(False, f"Error: {str(e)}")
            return False

    def disconnect(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.is_connected = False
        self.connection_status.emit(False, "Terputus")

    def send_command(self, command):
        if self.is_connected and self.serial_port and self.serial_port.is_open:
            self.tx_queue.append(command)
            return True
        return False

    def run(self):
        while self.running:
            # Membaca data dari serial port
            if self.is_connected and self.serial_port and self.serial_port.is_open:
                try:
                    # Kirim perintah dari queue
                    if self.tx_queue:
                        command = self.tx_queue.pop(0)
                        self.serial_port.write((command + '\n').encode())

                    # Baca response
                    if self.serial_port.in_waiting > 0:
                        data = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                        if data:
                            self.data_received.emit(data)
                except Exception as e:
                    self.connection_status.emit(False, f"Error komunikasi: {str(e)}")
                    self.disconnect()

            # Tunggu sedikit untuk mengurangi beban CPU
            time.sleep(0.01)

    def stop(self):
        self.running = False
        self.disconnect()
        self.wait()


class SlaveControlPanel(QWidget):
    """Panel kontrol untuk satu slave stepper"""
    command_request = pyqtSignal(str)

    def __init__(self, slave_id, parent=None):
        super().__init__(parent)
        self.slave_id = slave_id
        self.setup_ui()

    def setup_ui(self):
        # Main layout
        layout = QVBoxLayout(self)

        # Group box untuk mengelompokkan kontrol
        group_box = QGroupBox(f"Slave {self.slave_id.upper()}")
        group_layout = QVBoxLayout()

        # Status indicator
        status_layout = QHBoxLayout()
        self.status_label = QLabel("Status:")
        self.status_value = QLabel("Idle")
        self.status_value.setStyleSheet("color: blue; font-weight: bold;")

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

        # Movement controls
        movement_layout = QGridLayout()

        self.home_btn = QPushButton("Home")
        self.home_btn.clicked.connect(self.on_home_clicked)

        self.move_positive_btn = QPushButton("Move +")
        self.move_positive_btn.clicked.connect(self.on_move_positive)

        self.move_negative_btn = QPushButton("Move -")
        self.move_negative_btn.clicked.connect(self.on_move_negative)

        self.stop_btn = QPushButton("Stop")
        self.stop_btn.setStyleSheet("background-color: #ffcccc;")
        self.stop_btn.clicked.connect(self.on_stop_clicked)

        self.steps_spinbox = QSpinBox()
        self.steps_spinbox.setRange(1, 10000)
        self.steps_spinbox.setValue(100)

        self.speed_spinbox = QSpinBox()
        self.speed_spinbox.setRange(10, 5000)
        self.speed_spinbox.setValue(500)
        self.speed_spinbox.setSingleStep(50)

        # Tambahkan widget ke layout
        movement_layout.addWidget(QLabel("Steps:"), 0, 0)
        movement_layout.addWidget(self.steps_spinbox, 0, 1)
        movement_layout.addWidget(QLabel("Speed:"), 1, 0)
        movement_layout.addWidget(self.speed_spinbox, 1, 1)
        movement_layout.addWidget(self.home_btn, 2, 0)
        movement_layout.addWidget(self.stop_btn, 2, 1)
        movement_layout.addWidget(self.move_positive_btn, 3, 0)
        movement_layout.addWidget(self.move_negative_btn, 3, 1)

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
        group_layout.addLayout(movement_layout)
        group_layout.addLayout(custom_layout)

        # Set group layout ke group box
        group_box.setLayout(group_layout)

        # Tambahkan group box ke layout utama
        layout.addWidget(group_box)

    def on_home_clicked(self):
        self.command_request.emit(f"{self.slave_id};{1}")  # 1 = ZERO command
        self.status_value.setText("Homing...")
        self.status_value.setStyleSheet("color: orange; font-weight: bold;")

    def on_move_positive(self):
        steps = self.steps_spinbox.value()
        speed = self.speed_spinbox.value()
        # Format: slave_id;START;steps with speed parameter
        self.command_request.emit(f"{self.slave_id};{0};{steps}")
        self.status_value.setText("Moving +")
        self.status_value.setStyleSheet("color: green; font-weight: bold;")

    def on_move_negative(self):
        steps = -self.steps_spinbox.value()  # Negative steps for opposite direction
        speed = self.speed_spinbox.value()
        # Format: slave_id;START;steps with speed parameter
        self.command_request.emit(f"{self.slave_id};{0};{steps}")
        self.status_value.setText("Moving -")
        self.status_value.setStyleSheet("color: green; font-weight: bold;")

    def on_stop_clicked(self):
        self.command_request.emit(f"{self.slave_id};{4}")  # 4 = RESET command
        self.status_value.setText("Stopped")
        self.status_value.setStyleSheet("color: red; font-weight: bold;")

    def on_send_custom(self):
        command = self.custom_command.text().strip()
        if command:
            self.command_request.emit(f"{self.slave_id};{command}")
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
            self.status_value.setStyleSheet("color: blue; font-weight: bold;")
        elif "SEQUENCE COMPLETED" in message:
            self.status_value.setText("Idle")
            self.status_value.setStyleSheet("color: blue; font-weight: bold;")
        elif "MOVING" in message:
            self.status_value.setText("Moving")
            self.status_value.setStyleSheet("color: green; font-weight: bold;")
        elif "DELAYING" in message:
            self.status_value.setText("Delaying")
            self.status_value.setStyleSheet("color: purple; font-weight: bold;")


class SequencePanel(QWidget):
    """Panel untuk mengatur dan menjalankan sekuens gerakan"""
    sequence_command = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setup_ui()

    def setup_ui(self):
        layout = QVBoxLayout(self)

        # Group box untuk sekuens
        group_box = QGroupBox("Sequence Control")
        group_layout = QVBoxLayout()

        # Text editor untuk sekuens
        self.sequence_editor = QTextEdit()
        self.sequence_editor.setPlaceholderText(
            "Masukkan sekuens gerakan di sini. Format: slave_id(parameter1,parameter2,...)\n"
            "Contoh: x(1000,500), y(2000,700), z(d3000), t(-500,300), g(0)\n"
            "* Angka pertama adalah posisi target, angka kedua adalah kecepatan\n"
            "* Prefix 'd' berarti delay (dalam ms), contoh: z(d3000) = delay 3 detik"
        )

        # Tombol kontrol
        button_layout = QHBoxLayout()

        self.run_btn = QPushButton("Run Sequence")
        self.run_btn.clicked.connect(self.on_run_sequence)
        self.run_btn.setStyleSheet("background-color: #ccffcc;")

        self.stop_all_btn = QPushButton("Stop All")
        self.stop_all_btn.clicked.connect(self.on_stop_all)
        self.stop_all_btn.setStyleSheet("background-color: #ffcccc;")

        self.clear_btn = QPushButton("Clear")
        self.clear_btn.clicked.connect(self.sequence_editor.clear)

        button_layout.addWidget(self.run_btn)
        button_layout.addWidget(self.stop_all_btn)
        button_layout.addWidget(self.clear_btn)

        # Sample sequences dropdown
        sample_layout = QHBoxLayout()
        sample_layout.addWidget(QLabel("Sample Sequences:"))

        self.sample_combo = QComboBox()
        self.sample_combo.addItem("Select a sample...")
        self.sample_combo.addItem("Basic Movement")
        self.sample_combo.addItem("Rectangle Pattern")
        self.sample_combo.addItem("Pick and Place")
        self.sample_combo.currentIndexChanged.connect(self.on_sample_selected)

        sample_layout.addWidget(self.sample_combo)
        sample_layout.addStretch()

        # Add all to group layout
        group_layout.addLayout(sample_layout)
        group_layout.addWidget(self.sequence_editor)
        group_layout.addLayout(button_layout)

        # Set group layout
        group_box.setLayout(group_layout)

        # Add to main layout
        layout.addWidget(group_box)

    def on_run_sequence(self):
        sequence = self.sequence_editor.toPlainText().strip()
        if sequence:
            # Send START command first
            self.sequence_command.emit("START")
            # Then send the sequence
            self.sequence_command.emit(sequence)

    def on_stop_all(self):
        # Send RESET command to all slaves
        self.sequence_command.emit("RESET")

    def on_sample_selected(self, index):
        if index == 0:  # "Select a sample..."
            return

        if index == 1:  # Basic Movement
            sample = (
                "x(1000,500), y(1000,500),\n"
                "z(d2000),\n"  # Wait 2 seconds
                "x(0,500), y(0,500)"
            )
        elif index == 2:  # Rectangle Pattern
            sample = (
                "x(0,800), y(0,800), z(0,800), t(0,800),\n"
                "x(1000,800), y(0,800),\n"  # Move X
                "x(1000,800), y(1000,800),\n"  # Move Y
                "x(0,800), y(1000,800),\n"  # Move X back
                "x(0,800), y(0,800)"  # Move Y back
            )
        elif index == 3:  # Pick and Place
            sample = (
                "# Home all axes\n"
                "x(0,500), y(0,500), z(0,500), g(0,500),\n"
                "# Move to pick position\n"
                "x(1000,800), y(500,800),\n"
                "z(500,400),\n"  # Lower Z
                "g(100,300),\n"  # Close gripper
                "z(0,400),\n"  # Raise Z
                "# Move to place position\n"
                "x(500,800), y(1500,800),\n"
                "z(500,400),\n"  # Lower Z
                "g(0,300),\n"  # Open gripper
                "z(0,400),\n"  # Raise Z
                "# Return home\n"
                "x(0,800), y(0,800)"
            )

        self.sequence_editor.setText(sample)
        # Reset combobox to index 0
        self.sample_combo.setCurrentIndex(0)


class MonitorPanel(QWidget):
    """Panel untuk memonitor komunikasi dan log"""
    clear_logs = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setup_ui()

    def setup_ui(self):
        layout = QVBoxLayout(self)

        # Communication log
        log_group = QGroupBox("Communication Log")
        log_layout = QVBoxLayout()

        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)

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
        # This will be connected to appropriate handler
        self.send_btn.emit(Qt.Key_Enter)
        self.command_input.clear()


class PalletizerControlApp(QMainWindow):
    """Main application window"""

    def __init__(self):
        super().__init__()
        self.serial_thread = SerialCommunicator()
        self.available_ports = []
        self.slave_panels = {}
        self.setup_ui()
        self.init_connections()

    def setup_ui(self):
        self.setWindowTitle("Palletizer Control System")
        self.setGeometry(100, 100, 1280, 720)

        # Main widget and layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # Connection controls at top
        connection_layout = QHBoxLayout()

        connection_layout.addWidget(QLabel("Port:"))
        self.port_combo = QComboBox()
        self.refresh_ports()
        connection_layout.addWidget(self.port_combo)

        connection_layout.addWidget(QLabel("Baudrate:"))
        self.baudrate_combo = QComboBox()
        for baud in ["9600", "19200", "38400", "57600", "115200"]:
            self.baudrate_combo.addItem(baud)
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

        # Control panel with all slave controls
        control_widget = QWidget()
        control_layout = QGridLayout(control_widget)

        # Create control panels for each slave
        slave_ids = ['x', 'y', 'z', 't', 'g']
        for i, slave_id in enumerate(slave_ids):
            panel = SlaveControlPanel(slave_id)
            self.slave_panels[slave_id] = panel
            row, col = divmod(i, 3)  # 3 panels per row
            control_layout.addWidget(panel, row, col)

            # Connect panel's command_request signal to handle_slave_command
            panel.command_request.connect(self.handle_slave_command)

        # Tabs
        self.tab_widget.addTab(control_widget, "Individual Control")

        # Sequence control panel
        self.sequence_panel = SequencePanel()
        self.sequence_panel.sequence_command.connect(self.handle_sequence_command)
        self.tab_widget.addTab(self.sequence_panel, "Sequence Control")

        # Monitor panel
        self.monitor_panel = MonitorPanel()
        self.monitor_panel.send_btn.clicked.connect(self.handle_manual_command)
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
        else:
            # Connect
            if not self.available_ports:
                QMessageBox.warning(self, "No Ports", "No serial ports available.")
                return

            port = self.port_combo.currentText()
            baudrate = int(self.baudrate_combo.currentText())

            if self.serial_thread.connect(port, baudrate):
                self.connect_btn.setText("Disconnect")

    def update_connection_status(self, connected, message):
        if connected:
            self.status_label.setText(f"Status: {message}")
            self.status_label.setStyleSheet("color: green;")
        else:
            self.status_label.setText(f"Status: {message}")
            self.status_label.setStyleSheet("color: red;")
            self.connect_btn.setText("Connect")

    def handle_slave_command(self, command):
        """Handle commands from individual slave panels"""
        if self.serial_thread.is_connected:
            self.serial_thread.send_command(command)
            self.monitor_panel.add_log(command, "TX")

    def handle_sequence_command(self, command):
        """Handle commands from sequence panel"""
        if self.serial_thread.is_connected:
            self.serial_thread.send_command(command)
            self.monitor_panel.add_log(command, "TX")

    def handle_manual_command(self):
        """Handle commands from manual command input"""
        command = self.monitor_panel.command_input.text().strip()
        if command and self.serial_thread.is_connected:
            self.serial_thread.send_command(command)
            self.monitor_panel.add_log(command, "TX")
            self.monitor_panel.command_input.clear()

    def handle_received_data(self, data):
        """Handle data received from serial port"""
        self.monitor_panel.add_log(data, "RX")

        # Determine message type and route to appropriate handler
        if data.startswith("[FEEDBACK]"):
            # Feedback from master
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


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = PalletizerControlApp()
    window.show()
    sys.exit(app.exec_())
