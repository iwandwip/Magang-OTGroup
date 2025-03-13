from kivy.uix.boxlayout import BoxLayout
from kivy.uix.tabbedpanel import TabbedPanel, TabbedPanelItem
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.uix.spinner import Spinner
from kivy.properties import ObjectProperty, BooleanProperty, StringProperty

from palletizer.serial_communicator import SerialCommunicator
from palletizer.ui.slave_control_panel import SlaveControlPanel
from palletizer.ui.sequence.sequence_panel import SequencePanel
from palletizer.ui.monitor_panel import MonitorPanel
from palletizer.ui.position_tracker import PositionTracker
from palletizer.ui.visualization_panel import VisualizationPanel
from palletizer.ui.communication_settings_panel import CommunicationSettingsPanel
from palletizer.utils.config import *


class PalletizerControlApp(BoxLayout):
    """Main application window"""

    is_connected = BooleanProperty(False)
    status_text = StringProperty("Status: Disconnected")

    def __init__(self, **kwargs):
        super(PalletizerControlApp, self).__init__(**kwargs)
        self.orientation = 'vertical'
        self.spacing = 5
        self.padding = 5

        # Initialize components
        self.serial_thread = SerialCommunicator()
        self.available_ports = []
        self.slave_panels = {}

        # Initialize position tracker
        self.position_tracker = PositionTracker(self)

        # Command settings dictionary
        self.command_settings = {
            'START': CMD_START,
            'HOME': CMD_ZERO,
            'PAUSE': CMD_PAUSE,
            'RESUME': CMD_RESUME,
            'RESET': CMD_RESET,
            'SPEED_FORMAT': CMD_SPEED_FORMAT,
            'COMPLETE_FEEDBACK': 'ALL_SLAVES_COMPLETED'
        }

        # Build UI
        self.setup_ui()
        self.init_connections()

    def setup_ui(self):
        # Connection controls at top
        connection_layout = BoxLayout(size_hint=(1, 0.05), spacing=5)

        connection_layout.add_widget(Label(text="Port:", size_hint=(0.1, 1)))
        self.port_spinner = Spinner(text="Select Port", values=self.get_available_ports(), size_hint=(0.2, 1))
        connection_layout.add_widget(self.port_spinner)

        connection_layout.add_widget(Label(text="Baudrate:", size_hint=(0.1, 1)))
        baud_values = [str(b) for b in BAUDRATES]
        self.baudrate_spinner = Spinner(text=str(DEFAULT_BAUDRATE), values=baud_values, size_hint=(0.2, 1))
        connection_layout.add_widget(self.baudrate_spinner)

        self.connect_btn = Button(text="Connect", size_hint=(0.15, 1))
        self.connect_btn.bind(on_press=self.on_connect)
        connection_layout.add_widget(self.connect_btn)

        self.refresh_btn = Button(text="Refresh", size_hint=(0.15, 1))
        self.refresh_btn.bind(on_press=self.refresh_ports)
        connection_layout.add_widget(self.refresh_btn)

        self.status_label = Label(text=self.status_text, size_hint=(0.2, 1))
        connection_layout.add_widget(self.status_label)

        self.add_widget(connection_layout)

        # Tab panel for different sections
        self.tab_panel = TabbedPanel(do_default_tab=False, size_hint=(1, 0.95))

        # Individual Control Tab
        control_tab = TabbedPanelItem(text="Individual Control")
        control_content = BoxLayout(orientation='vertical')

        # Create grid layout for slave controls
        control_grid = self.create_slave_control_grid()
        control_content.add_widget(control_grid)
        control_tab.content = control_content

        # Sequence Control Tab
        sequence_tab = TabbedPanelItem(text="Sequence Control")
        self.sequence_panel = SequencePanel()
        sequence_tab.content = self.sequence_panel

        # Visualization Tab
        visualization_tab = TabbedPanelItem(text="3D Visualization")
        self.visualization_panel = VisualizationPanel()
        visualization_tab.content = self.visualization_panel

        # Monitor Tab
        monitor_tab = TabbedPanelItem(text="Monitor")
        self.monitor_panel = MonitorPanel()
        monitor_tab.content = self.monitor_panel

        # Settings Tab
        settings_tab = TabbedPanelItem(text="Communication Settings")
        self.comm_settings_panel = CommunicationSettingsPanel(self)
        settings_tab.content = self.comm_settings_panel

        # Add tabs to panel
        self.tab_panel.add_widget(control_tab)
        self.tab_panel.add_widget(sequence_tab)
        self.tab_panel.add_widget(visualization_tab)
        self.tab_panel.add_widget(monitor_tab)
        self.tab_panel.add_widget(settings_tab)

        self.add_widget(self.tab_panel)

    def create_slave_control_grid(self):
        from kivy.uix.gridlayout import GridLayout
        from kivy.uix.scrollview import ScrollView

        scroll_view = ScrollView(bar_width=10)
        grid = GridLayout(cols=3, spacing=10, size_hint_y=None)
        grid.bind(minimum_height=grid.setter('height'))

        # Create control panels for each slave
        for slave_id in SLAVE_IDS:
            panel = SlaveControlPanel(slave_id)
            self.slave_panels[slave_id] = panel
            grid.add_widget(panel)

            # Bind panel signals (Implementasi Kivy event binding)
            panel.bind(on_command=self.handle_slave_command)
            panel.bind(on_position_change=self.on_position_changed)

        scroll_view.add_widget(grid)
        return scroll_view

    def init_connections(self):
        # Register Kivy Events
        self.serial_thread.register_callback('on_data_received', self.handle_received_data)
        self.serial_thread.register_callback('on_connection_status', self.update_connection_status)

        # Connect position tracker signals
        self.position_tracker.bind(on_position_updated=self.on_tracker_position_updated)

        # Start serial thread
        self.serial_thread.start()

    def get_available_ports(self):
        try:
            from serial.tools.list_ports import comports
            self.available_ports = [port.device for port in comports()]
            if not self.available_ports:
                return ["No ports available"]
            return self.available_ports
        except:
            return ["Serial module not available"]

    def refresh_ports(self, instance=None):
        ports = self.get_available_ports()
        self.port_spinner.values = ports
        if ports:
            self.port_spinner.text = ports[0]

    def on_connect(self, instance):
        if self.is_connected:
            # Disconnect
            self.serial_thread.disconnect()
            self.connect_btn.text = "Connect"
        else:
            # Connect
            if not self.available_ports or self.available_ports[0] == "No ports available":
                from kivy.uix.popup import Popup
                from kivy.uix.label import Label
                popup = Popup(title='Error', content=Label(text='No serial ports available.'),
                              size_hint=(0.5, 0.3))
                popup.open()
                return

            port = self.port_spinner.text
            baudrate = int(self.baudrate_spinner.text)

            if self.serial_thread.connect(port, baudrate):
                self.connect_btn.text = "Disconnect"

    def update_connection_status(self, connected, message):
        self.is_connected = connected
        if connected:
            self.status_text = f"Status: {message}"
            self.status_label.color = [0, 1, 0, 1]  # Green
        else:
            self.status_text = f"Status: {message}"
            self.status_label.color = [1, 0, 0, 1]  # Red
            self.connect_btn.text = "Connect"

    # Implementasi handler lainnya...
    def handle_slave_command(self, instance, command):
        """Handle commands from individual slave panels"""
        if self.is_connected:
            self.serial_thread.send_command(command)
            self.monitor_panel.add_log(command, "TX")
            self.position_tracker.parse_command(command)

    def on_position_changed(self, instance, axis_id, position):
        """Handle position changes from UI operations"""
        self.position_tracker.set_position(axis_id, position)
        self.sequence_panel.update_position(axis_id, position)
        self.visualization_panel.update_position(axis_id, position)
        self.monitor_panel.add_log(f"Position update: {axis_id.upper()} to {position}", "INFO")

    def on_tracker_position_updated(self, instance, axis_id, position):
        """Handle position updates from the position tracker"""
        if axis_id in self.slave_panels:
            self.slave_panels[axis_id].set_position(position)
        self.sequence_panel.update_position(axis_id, position)
        self.visualization_panel.update_position(axis_id, position)

    def handle_received_data(self, data):
        # Implementasi penanganan data yang diterima
        pass
