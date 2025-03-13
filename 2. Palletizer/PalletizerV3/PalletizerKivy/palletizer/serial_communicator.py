from kivy.clock import Clock
import threading
import time


class SerialCommunicator(threading.Thread):
    """Thread class untuk menangani komunikasi serial - Implementasi Kivy"""

    def __init__(self, parent=None):
        super(SerialCommunicator, self).__init__()
        self.daemon = True
        self.serial_port = None
        self.is_connected = False
        self.running = True
        self.tx_queue = []
        self.callbacks = {
            'on_data_received': [],
            'on_connection_status': []
        }

    def register_callback(self, event_name, callback):
        """Register callback untuk event"""
        if event_name in self.callbacks:
            self.callbacks[event_name].append(callback)

    def connect(self, port, baudrate):
        try:
            # Import serial di sini untuk mendukung sistem tanpa modul serial
            import serial
            self.serial_port = serial.Serial(port, baudrate, timeout=0.5)
            self.is_connected = True
            self._notify_connection_status(True, f"Terhubung ke {port}")
            return True
        except Exception as e:
            self._notify_connection_status(False, f"Error: {str(e)}")
            return False

    def _notify_connection_status(self, connected, message):
        """Notify semua callback tentang status koneksi"""
        callbacks = self.callbacks.get('on_connection_status', [])
        for callback in callbacks:
            # Gunakan Clock untuk memastikan callback dijalankan di thread utama
            Clock.schedule_once(lambda dt: callback(connected, message), 0)

    def disconnect(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.is_connected = False
        self._notify_connection_status(False, "Terputus")

    def send_command(self, command):
        if self.is_connected and self.serial_port and self.serial_port.is_open:
            self.tx_queue.append(command)
            return True
        return False

    def run(self):
        """Thread utama untuk komunikasi"""
        while self.running:
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
                            self._notify_data_received(data)
                except Exception as e:
                    self._notify_connection_status(False, f"Error komunikasi: {str(e)}")
                    self.disconnect()

            # Tunggu untuk mengurangi beban CPU
            time.sleep(0.01)

    def _notify_data_received(self, data):
        """Notify semua callback tentang data yang diterima"""
        callbacks = self.callbacks.get('on_data_received', [])
        for callback in callbacks:
            Clock.schedule_once(lambda dt: callback(data), 0)

    def stop(self):
        """Hentikan thread"""
        self.running = False
        self.disconnect()
