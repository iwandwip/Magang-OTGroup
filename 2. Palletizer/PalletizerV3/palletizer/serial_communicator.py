import time
import serial
import serial.tools.list_ports
from PyQt5.QtCore import QThread, pyqtSignal


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
