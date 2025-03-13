from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.uix.textinput import TextInput
from kivy.uix.slider import Slider
from kivy.uix.checkbox import CheckBox
from kivy.properties import StringProperty, NumericProperty, BooleanProperty, ObjectProperty
from kivy.lang import Builder

# Definisikan UI dalam Kivy Language
Builder.load_string('''
<SlaveControlPanel>:
    orientation: 'vertical'
    padding: 5
    spacing: 5
    canvas.before:
        Color:
            rgba: 0.95, 0.95, 0.95, 1
        Rectangle:
            pos: self.pos
            size: self.size
    
    BoxLayout:
        size_hint_y: 0.1
        Label:
            text: "Status:"
        Label:
            id: status_value
            text: "Idle"
            color: 0, 0, 1, 1
    
    BoxLayout:
        size_hint_y: 0.1
        Label:
            text: "Position:"
        Label:
            id: position_value
            text: "0"
            font_name: "RobotoMono-Regular"
            bold: True
    
    BoxLayout:
        size_hint_y: 0.1
        Label:
            text: "Target Pos:"
        TextInput:
            id: target_spinbox
            input_filter: 'int'
            multiline: False
            text: "0"
        Button:
            text: "Go To"
            on_press: root.on_goto_clicked()
            background_color: 0.8, 1, 0.8, 1
    
    BoxLayout:
        size_hint_y: 0.2
        orientation: 'vertical'
        Label:
            text: "Speed:"
            size_hint_y: 0.3
        BoxLayout:
            Slider:
                id: speed_slider
                min: root.min_speed
                max: root.max_speed
                value: root.default_speed
            TextInput:
                id: speed_spinbox
                input_filter: 'int'
                multiline: False
                text: str(root.default_speed)
                size_hint_x: 0.3
            Button:
                text: "Set"
                on_press: root.on_set_speed()
                size_hint_x: 0.2
                background_color: 0.8, 0.8, 1, 1
    
    GridLayout:
        cols: 2
        size_hint_y: 0.3
        Button:
            text: "Start"
            on_press: root.on_start_clicked()
            background_color: 0.8, 1, 0.8, 1
        Button:
            text: "Home"
            on_press: root.on_home_clicked()
            background_color: 1, 1, 0.8, 1
        Button:
            text: "Pause"
            on_press: root.on_pause_clicked()
            background_color: 1, 1, 0.8, 1
        Button:
            text: "Resume"
            on_press: root.on_resume_clicked()
            background_color: 0.8, 1, 0.8, 1
        Button:
            text: "Stop/Reset"
            on_press: root.on_stop_clicked()
            background_color: 1, 0.8, 0.8, 1
    
    BoxLayout:
        size_hint_y: 0.1
        TextInput:
            id: custom_command
            multiline: False
            hint_text: "Custom command..."
        Button:
            text: "Send"
            on_press: root.on_send_custom()
            size_hint_x: 0.3
''')


class SlaveControlPanel(BoxLayout):
    """Panel kontrol untuk satu slave stepper dengan Absolute Positioning"""
    slave_id = StringProperty('')
    min_speed = NumericProperty(1000)
    max_speed = NumericProperty(10000)
    default_speed = NumericProperty(1000)

    # Custom events
    __events__ = ('on_command', 'on_position_change')

    def __init__(self, slave_id, **kwargs):
        super(SlaveControlPanel, self).__init__(**kwargs)
        self.slave_id = slave_id
        self.current_speed = self.default_speed
        self.current_position = 0
        self.target_position = 0

        # Bind speed slider and text input
        self.ids.speed_slider.bind(value=self.update_speed_display)
        self.ids.speed_spinbox.bind(text=self.update_speed_slider)

    def update_speed_display(self, instance, value):
        """Update speed display when slider changes"""
        self.ids.speed_spinbox.text = str(int(value))
        self.current_speed = int(value)

    def update_speed_slider(self, instance, value):
        """Update slider when text input changes"""
        try:
            val = int(value)
            if self.min_speed <= val <= self.max_speed:
                self.ids.speed_slider.value = val
                self.current_speed = val
        except ValueError:
            pass

    def on_goto_clicked(self):
        """Send command to go to the target position"""
        try:
            target = int(self.ids.target_spinbox.text)
            self.target_position = target

            # Format: x(position) - absolute positioning
            move_cmd = f"{self.slave_id}({self.target_position})"
            self.dispatch('on_command', move_cmd)

            self.ids.status_value.text = f"Moving to {self.target_position}"
            self.ids.status_value.color = (0, 1, 0, 1)  # Green

            # Update the position
            self.current_position = self.target_position
            self.ids.position_value.text = str(self.current_position)
            self.dispatch('on_position_change', self.slave_id, self.current_position)
        except ValueError:
            pass

    def on_set_speed(self):
        """Send speed command"""
        from palletizer.utils.config import CMD_SPEED_FORMAT
        command = CMD_SPEED_FORMAT.format(self.slave_id, self.current_speed)
        self.dispatch('on_command', command)
        self.ids.status_value.text = f"Setting Speed: {self.current_speed}"
        self.ids.status_value.color = (0.5, 0, 0.5, 1)  # Purple

    def on_start_clicked(self):
        """Send start command"""
        from palletizer.utils.config import CMD_START
        self.dispatch('on_command', CMD_START)
        self.ids.status_value.text = "Starting..."
        self.ids.status_value.color = (0, 1, 0, 1)  # Green

    def on_home_clicked(self):
        """Send home command"""
        from palletizer.utils.config import CMD_ZERO
        self.dispatch('on_command', CMD_ZERO)
        self.ids.status_value.text = "Homing..."
        self.ids.status_value.color = (1, 0.5, 0, 1)  # Orange

        # Reset position
        self.current_position = 0
        self.target_position = 0
        self.ids.target_spinbox.text = "0"
        self.ids.position_value.text = "0"
        self.dispatch('on_position_change', self.slave_id, 0)

    def on_pause_clicked(self):
        """Send pause command"""
        from palletizer.utils.config import CMD_PAUSE
        self.dispatch('on_command', CMD_PAUSE)
        self.ids.status_value.text = "Paused"
        self.ids.status_value.color = (1, 0.5, 0, 1)  # Orange

    def on_resume_clicked(self):
        """Send resume command"""
        from palletizer.utils.config import CMD_RESUME
        self.dispatch('on_command', CMD_RESUME)
        self.ids.status_value.text = "Resuming"
        self.ids.status_value.color = (0, 1, 0, 1)  # Green

    def on_stop_clicked(self):
        """Send stop/reset command"""
        from palletizer.utils.config import CMD_RESET
        self.dispatch('on_command', CMD_RESET)
        self.ids.status_value.text = "Stopped"
        self.ids.status_value.color = (1, 0, 0, 1)  # Red

    def on_send_custom(self):
        """Send custom command"""
        command = self.ids.custom_command.text.strip()
        if command:
            self.dispatch('on_command', command)
            self.ids.custom_command.text = ""

    def set_position(self, position):
        """Set the position display"""
        self.current_position = position
        self.ids.position_value.text = str(position)

    def update_status(self, message):
        """Update status based on received message"""
        if "POS:" in message:
            parts = message.split()
            for part in parts:
                if part.startswith("POS:"):
                    position = part.split(':')[1]
                    try:
                        pos_value = int(position)
                        self.current_position = pos_value
                        self.ids.position_value.text = position
                        self.ids.target_spinbox.text = str(pos_value)
                        self.dispatch('on_position_change', self.slave_id, pos_value)
                    except ValueError:
                        self.ids.position_value.text = position

        # Update status display based on messages
        if "ZERO DONE" in message:
            self.ids.status_value.text = "Homed"
            self.ids.status_value.color = (0, 0, 1, 1)  # Blue
            self.current_position = 0
            self.target_position = 0
            self.ids.target_spinbox.text = "0"
            self.ids.position_value.text = "0"
            self.dispatch('on_position_change', self.slave_id, 0)
        elif "MOVING" in message:
            self.ids.status_value.text = "Moving"
            self.ids.status_value.color = (0, 1, 0, 1)  # Green

    # Event handlers
    def on_command(self, command):
        """Event dispatched when a command is sent"""
        pass

    def on_position_change(self, axis_id, position):
        """Event dispatched when position changes"""
        pass
