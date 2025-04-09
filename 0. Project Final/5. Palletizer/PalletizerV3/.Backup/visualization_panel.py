import numpy as np
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QGroupBox,
                             QPushButton, QLabel, QSlider, QCheckBox, QSplitter, QFrame,
                             QSpinBox, QDoubleSpinBox, QFormLayout, QGroupBox)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QVector3D, QColor
import pyqtgraph as pg
import pyqtgraph.opengl as gl
from ..utils.config import *


class VisualizationPanel(QWidget):
    """Panel untuk visualisasi sistem sumbu palletizer (X, Y, Z, T, G)"""

    def __init__(self, parent=None):
        super().__init__(parent)
        # Initialize current positions
        self.positions = {'x': 0, 'y': 0, 'z': 0, 't': 0, 'g': 0}

        # Initialize axis ranges with default values
        self.axis_ranges = {
            'x': {'min': -1000, 'max': 1000},
            'y': {'min': -1000, 'max': 1000},
            'z': {'min': -1000, 'max': 1000},
            't': {'min': -1000, 'max': 1000},
            'g': {'min': -1000, 'max': 1000}
        }

        # Flag to track initialization
        self.initialized = False

        # Setup UI
        self.setup_ui()

        # Initialize visualization with a timer to ensure proper rendering
        QTimer.singleShot(100, self.initialize_visualization)

    def setup_ui(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)

        # Create splitter for visualization and controls
        splitter = QSplitter(Qt.Horizontal)

        # Left side - Visualization
        vis_widget = QWidget()
        vis_layout = QVBoxLayout(vis_widget)

        # Frame for the OpenGL view
        view_frame = QFrame()
        view_frame.setFrameShape(QFrame.StyledPanel)
        view_frame.setFrameShadow(QFrame.Sunken)
        view_frame.setMinimumSize(600, 500)  # Ensure minimum size
        view_layout = QVBoxLayout(view_frame)

        # Create 3D visualization placeholder
        self.view_placeholder = QLabel("Initializing 3D View...")
        self.view_placeholder.setAlignment(Qt.AlignCenter)
        self.view_placeholder.setStyleSheet("background-color: #f0f0f0; border: 1px solid #cccccc;")
        view_layout.addWidget(self.view_placeholder)

        vis_layout.addWidget(view_frame)

        # Store the view_frame and view_layout for later use
        self.view_frame = view_frame
        self.view_layout = view_layout

        # Add information about the visualization
        info_group = QGroupBox("Visualization Information")
        info_layout = QVBoxLayout()

        info_text = QLabel(
            "This visualization shows the palletizer axis system:\n\n"
            "- X-AXIS (Red): Main horizontal axis\n"
            "- Y-AXIS (Green): Secondary horizontal axis\n"
            "- Z-AXIS (Blue): Vertical axis with gripper moving along it\n"
            "- T-AXIS: Gripper rotation\n"
            "- G-AXIS: Gripper clamp\n\n"
            "The X axis is the main axis that affects all others.\n"
            "Y affects Z, T, G. The gripper moves along Z."
        )
        info_text.setWordWrap(True)
        info_layout.addWidget(info_text)

        info_group.setLayout(info_layout)
        vis_layout.addWidget(info_group)

        # Right side - Controls
        control_widget = QWidget()
        control_layout = QVBoxLayout(control_widget)

        # Current Position Display
        position_group = QGroupBox("Current Positions")
        position_layout = QVBoxLayout()

        # Create position labels for each axis
        self.pos_labels = {}
        for axis in SLAVE_IDS:
            axis_layout = QHBoxLayout()
            axis_layout.addWidget(QLabel(f"{axis.upper()}-AXIS:"))

            pos_label = QLabel("0")
            pos_label.setStyleSheet("font-family: monospace; font-weight: bold;")
            self.pos_labels[axis.lower()] = pos_label

            axis_layout.addWidget(pos_label)
            axis_layout.addStretch()
            position_layout.addLayout(axis_layout)

        position_group.setLayout(position_layout)
        control_layout.addWidget(position_group)

        # NEW: Axis Range Controls
        range_group = QGroupBox("Axis Range Settings")
        range_layout = QVBoxLayout()

        # Create min/max inputs for each axis
        self.min_inputs = {}
        self.max_inputs = {}

        for axis in SLAVE_IDS:
            axis_range_frame = QFrame()
            axis_range_frame.setFrameShape(QFrame.StyledPanel)
            axis_range_frame.setStyleSheet("background-color: #f8f8f8; margin: 2px;")

            axis_range_layout = QVBoxLayout(axis_range_frame)
            axis_range_layout.setContentsMargins(5, 5, 5, 5)

            axis_header = QLabel(f"{axis.upper()}-AXIS Range")
            axis_header.setStyleSheet("font-weight: bold;")
            axis_range_layout.addWidget(axis_header)

            range_form = QFormLayout()
            range_form.setContentsMargins(0, 0, 0, 0)
            range_form.setSpacing(5)

            # Min value input
            min_input = QSpinBox()
            min_input.setRange(-100000, 100000)
            min_input.setValue(self.axis_ranges[axis.lower()]['min'])
            min_input.setSingleStep(100)
            min_input.valueChanged.connect(lambda v, a=axis.lower(): self.on_min_changed(a, v))
            self.min_inputs[axis.lower()] = min_input

            # Max value input
            max_input = QSpinBox()
            max_input.setRange(-100000, 100000)
            max_input.setValue(self.axis_ranges[axis.lower()]['max'])
            max_input.setSingleStep(100)
            max_input.valueChanged.connect(lambda v, a=axis.lower(): self.on_max_changed(a, v))
            self.max_inputs[axis.lower()] = max_input

            range_form.addRow("Min:", min_input)
            range_form.addRow("Max:", max_input)

            axis_range_layout.addLayout(range_form)
            range_layout.addWidget(axis_range_frame)

        # Apply button for all ranges
        apply_btn = QPushButton("Apply All Range Settings")
        apply_btn.clicked.connect(self.apply_all_ranges)
        apply_btn.setStyleSheet("background-color: #e0e0ff; font-weight: bold;")

        # Reset ranges button
        reset_ranges_btn = QPushButton("Reset to Default Ranges")
        reset_ranges_btn.clicked.connect(self.reset_ranges)
        reset_ranges_btn.setStyleSheet("background-color: #ffe0e0;")

        range_buttons = QHBoxLayout()
        range_buttons.addWidget(apply_btn)
        range_buttons.addWidget(reset_ranges_btn)

        range_layout.addLayout(range_buttons)
        range_group.setLayout(range_layout)

        # Add the range controls to the control layout
        control_layout.addWidget(range_group)

        # Axis Movement Controls
        movement_group = QGroupBox("Axis Movement")
        movement_layout = QVBoxLayout()

        self.sliders = {}
        for axis in SLAVE_IDS:
            axis_layout = QHBoxLayout()
            axis_layout.addWidget(QLabel(f"{axis}:"))

            slider = QSlider(Qt.Horizontal)
            slider.setMinimum(self.axis_ranges[axis.lower()]['min'])
            slider.setMaximum(self.axis_ranges[axis.lower()]['max'])
            slider.setValue(0)
            slider.setTickPosition(QSlider.TicksBelow)
            slider.setTickInterval(100)
            slider.valueChanged.connect(lambda v, a=axis.lower(): self.on_slider_changed(a, v))
            self.sliders[axis.lower()] = slider

            axis_layout.addWidget(slider)
            movement_layout.addLayout(axis_layout)

        # Reset button
        reset_btn = QPushButton("Reset All Positions")
        reset_btn.clicked.connect(self.reset_positions)
        reset_btn.setStyleSheet(BUTTON_HOME)
        movement_layout.addWidget(reset_btn)

        # View controls
        view_group = QGroupBox("View Controls")
        view_layout = QVBoxLayout()

        # Add some checkboxes for different views
        self.top_view_cb = QCheckBox("Top View")
        self.top_view_cb.clicked.connect(lambda: self.set_view('top'))

        self.side_view_cb = QCheckBox("Side View")
        self.side_view_cb.clicked.connect(lambda: self.set_view('side'))

        self.front_view_cb = QCheckBox("Front View")
        self.front_view_cb.clicked.connect(lambda: self.set_view('front'))

        self.isometric_view_cb = QCheckBox("Isometric View")
        self.isometric_view_cb.setChecked(True)
        self.isometric_view_cb.clicked.connect(lambda: self.set_view('isometric'))

        view_layout.addWidget(self.top_view_cb)
        view_layout.addWidget(self.side_view_cb)
        view_layout.addWidget(self.front_view_cb)
        view_layout.addWidget(self.isometric_view_cb)

        view_group.setLayout(view_layout)

        movement_group.setLayout(movement_layout)
        control_layout.addWidget(movement_group)
        control_layout.addWidget(view_group)
        control_layout.addStretch()

        # Add both panels to the splitter
        splitter.addWidget(vis_widget)
        splitter.addWidget(control_widget)

        # Set initial sizes for the splitter (approximately 70% / 30%)
        splitter.setSizes([700, 300])

        main_layout.addWidget(splitter, 1)  # Give more space to sequence container

    def on_min_changed(self, axis, value):
        """Handle min value change for an axis"""
        # Ensure min doesn't exceed max
        if value >= self.max_inputs[axis].value():
            self.min_inputs[axis].setValue(self.max_inputs[axis].value() - 1)
            return

        # Store the new range value
        self.axis_ranges[axis]['min'] = value

    def on_max_changed(self, axis, value):
        """Handle max value change for an axis"""
        # Ensure max doesn't go below min
        if value <= self.min_inputs[axis].value():
            self.max_inputs[axis].setValue(self.min_inputs[axis].value() + 1)
            return

        # Store the new range value
        self.axis_ranges[axis]['max'] = value

    def apply_all_ranges(self):
        """Apply all range settings to the sliders"""
        for axis in SLAVE_IDS:
            axis_lower = axis.lower()
            # Update slider ranges
            self.sliders[axis_lower].setMinimum(self.axis_ranges[axis_lower]['min'])
            self.sliders[axis_lower].setMaximum(self.axis_ranges[axis_lower]['max'])

            # If current position is outside the new range, adjust it
            current_pos = self.positions[axis_lower]
            min_val = self.axis_ranges[axis_lower]['min']
            max_val = self.axis_ranges[axis_lower]['max']

            if current_pos < min_val:
                self.update_position(axis_lower, min_val)
            elif current_pos > max_val:
                self.update_position(axis_lower, max_val)

            # Update tick interval based on range
            range_size = max_val - min_val
            tick_interval = max(int(range_size / 10), 1)  # Ensure at least 10 ticks
            self.sliders[axis_lower].setTickInterval(tick_interval)

    def reset_ranges(self):
        """Reset all ranges to default values"""
        default_min = -1000
        default_max = 1000

        for axis in SLAVE_IDS:
            # Update stored ranges
            self.axis_ranges[axis.lower()]['min'] = default_min
            self.axis_ranges[axis.lower()]['max'] = default_max

            # Update inputs
            self.min_inputs[axis.lower()].setValue(default_min)
            self.max_inputs[axis.lower()].setValue(default_max)

        # Apply the changes
        self.apply_all_ranges()

    def initialize_visualization(self):
        """Initialize the 3D visualization with proper error handling"""
        try:
            # Create a new OpenGL Widget
            self.view = gl.GLViewWidget()
            self.view.setCameraPosition(distance=2000, elevation=30, azimuth=45)
            self.view.setBackgroundColor(255, 255, 255, 255)  # White background

            # Clear the placeholder
            for i in reversed(range(self.view_layout.count())):
                self.view_layout.itemAt(i).widget().setParent(None)

            # Add the OpenGL view to the layout
            self.view_layout.addWidget(self.view)

            # Set up grid
            grid = gl.GLGridItem()
            grid.setSize(x=2000, y=2000)
            grid.setSpacing(x=100, y=100)
            self.view.addItem(grid)

            # Create axis lines with different colors
            # X-axis (red)
            x_axis_line = np.array([[0, 0, 0], [1000, 0, 0]])
            self.x_axis = gl.GLLinePlotItem(pos=x_axis_line, color=(1, 0, 0, 1), width=3)
            self.view.addItem(self.x_axis)

            # Y-axis (green)
            y_axis_line = np.array([[0, 0, 0], [0, 1000, 0]])
            self.y_axis = gl.GLLinePlotItem(pos=y_axis_line, color=(0, 1, 0, 1), width=3)
            self.view.addItem(self.y_axis)

            # Z-axis (blue)
            z_axis_line = np.array([[0, 0, 0], [0, 0, 1000]])
            self.z_axis = gl.GLLinePlotItem(pos=z_axis_line, color=(0, 0, 1, 1), width=3)
            self.view.addItem(self.z_axis)

            # Add text labels for axes
            self.x_label = gl.GLTextItem(pos=np.array([1050, 0, 0]), text='X-AXIS', color=(1, 0, 0, 1))
            self.view.addItem(self.x_label)

            self.y_label = gl.GLTextItem(pos=np.array([0, 1050, 0]), text='Y-AXIS', color=(0, 1, 0, 1))
            self.view.addItem(self.y_label)

            self.z_label = gl.GLTextItem(pos=np.array([0, 0, 1050]), text='Z-AXIS', color=(0, 0, 1, 1))
            self.view.addItem(self.z_label)

            # Create palletizer mechanical structure
            # X-axis rail (horizontal main axis)
            self.x_rail = gl.GLBoxItem(size=QVector3D(1500, 50, 50))
            self.x_rail.setColor(QColor(128, 128, 128, 255))
            # Position X-rail with 0 at center
            self.x_rail.translate(-750, 0, 0)
            self.view.addItem(self.x_rail)

            # Y-axis rail (horizontal secondary axis) - shortened to not extend beyond the X-axis
            # Only extending in the negative Y direction (no extension beyond the center)
            self.y_rail = gl.GLBoxItem(size=QVector3D(50, 500, 50))
            self.y_rail.setColor(QColor(150, 150, 150, 255))
            # Position centered on X, extending only in negative Y direction
            self.y_rail.translate(0, -500, 0)
            self.view.addItem(self.y_rail)

            # Define dimensions for Z-axis structure
            z_width = 50
            z_depth = 50
            z_length = 1200  # Total length of Z axis

            # Z-axis rail (vertical axis) - FIXED, doesn't move up/down
            self.z_rail = gl.GLBoxItem(size=QVector3D(z_width, z_depth, z_length))
            self.z_rail.setColor(QColor(100, 100, 255, 255))  # Blue color for Z
            self.z_rail.translate(-z_width/2, -z_depth/2, -z_length)  # Position it with top at origin, extending down
            self.view.addItem(self.z_rail)

            # Gripper carriage - this is what moves up and down along Z-axis
            carriage_size = 80
            carriage_height = 60
            self.z_carriage = gl.GLBoxItem(size=QVector3D(carriage_size, carriage_size, carriage_height))
            self.z_carriage.setColor(QColor(180, 180, 180, 255))
            # Initial position at top of Z-rail
            self.z_carriage.translate(-carriage_size/2, -carriage_size/2, -carriage_height)
            self.view.addItem(self.z_carriage)

            # T-axis (gripper rotation) - attached to Z carriage
            t_size = 180  # Width and depth of T
            t_height = 40  # Height of T

            # Create T as a platform attached to the Z carriage
            self.t_part = gl.GLBoxItem(size=QVector3D(t_size, t_size, t_height))
            self.t_part.setColor(QColor(200, 200, 200, 255))
            # Position beneath the Z carriage
            self.t_part.translate(-t_size/2, -t_size/2, -carriage_height-t_height)
            self.view.addItem(self.t_part)

            # G-axis (gripper) - attached to T-axis
            gripper_thickness = 30
            gripper_length = 200
            gripper_height = 100
            gripper_spacing = 150  # Initial distance between grippers

            # Left gripper part
            self.g_left = gl.GLBoxItem(size=QVector3D(gripper_thickness, gripper_length, gripper_height))
            self.g_left.setColor(QColor(220, 220, 220, 255))
            # Position directly at the bottom of T
            self.g_left.translate(-gripper_spacing/2-gripper_thickness, -gripper_length/2, -carriage_height-t_height-gripper_height)
            self.view.addItem(self.g_left)

            # Right gripper part
            self.g_right = gl.GLBoxItem(size=QVector3D(gripper_thickness, gripper_length, gripper_height))
            self.g_right.setColor(QColor(220, 220, 220, 255))
            # Position directly at the bottom of T
            self.g_right.translate(gripper_spacing/2, -gripper_length/2, -carriage_height-t_height-gripper_height)
            self.view.addItem(self.g_right)

            # Set the initialization flag
            self.initialized = True

            # Set initial view
            self.set_view('isometric')

            # Update the visualization
            self.update_visualization()

        except Exception as e:
            # If visualization fails, show error
            error_label = QLabel(f"3D Visualization Error: {str(e)}\n\nPlease check if PyQtGraph and PyOpenGL are properly installed.")
            error_label.setWordWrap(True)
            error_label.setStyleSheet("color: red; background-color: #ffeeee; padding: 10px; border: 1px solid red;")
            self.view_layout.addWidget(error_label)

    def on_slider_changed(self, axis, value):
        """Handle slider value changes"""
        self.positions[axis] = value
        self.pos_labels[axis].setText(str(value))
        self.update_visualization()

    def reset_positions(self):
        """Reset all positions to zero"""
        for axis in SLAVE_IDS:
            self.positions[axis.lower()] = 0
            self.sliders[axis.lower()].setValue(0)
            self.pos_labels[axis.lower()].setText("0")
        self.update_visualization()

    def update_visualization(self):
        """Update the 3D visualization based on current positions"""
        # Make sure visualization is initialized
        if not self.initialized:
            return

        try:
            # Get current positions
            x_pos = self.positions['x']
            y_pos = self.positions['y']
            z_pos = self.positions['z']
            t_pos = self.positions['t']
            g_pos = self.positions['g']

            # Get dimensions from initialization
            z_width = 50
            z_depth = 50
            z_length = 1200
            carriage_size = 80
            carriage_height = 60
            t_size = 180
            t_height = 40
            gripper_thickness = 30
            gripper_length = 200
            gripper_height = 100

            # Y axis moves with X - only in the negative Y direction
            self.y_rail.resetTransform()
            # Position y_rail at x_pos with only negative extension
            self.y_rail.translate(x_pos, -500, 0)

            # Z axis rail moves with Y and X, but doesn't move up/down
            self.z_rail.resetTransform()
            # Center Z rail at the current position
            self.z_rail.translate(x_pos-z_width/2, y_pos-z_depth/2, -z_length)

            # Z carriage moves up and down along the Z rail
            # Calculate Z carriage position:
            # - At z_pos = 0, it's at the top of the Z rail
            # - At z_pos = -1000, it's near the bottom of the Z rail
            # Map z_pos range (-1000 to 1000) to position along Z rail (0 to -z_length)
            z_carriage_pos = -carriage_height + (z_pos / 1000) * (-z_length + carriage_height + 50)
            z_carriage_pos = max(-z_length + carriage_height, min(-carriage_height, z_carriage_pos))

            self.z_carriage.resetTransform()
            self.z_carriage.translate(x_pos-carriage_size/2, y_pos-carriage_size/2, z_carriage_pos)

            # T axis (rotation platform) moves with Z carriage
            self.t_part.resetTransform()
            # First position T below the Z carriage
            self.t_part.translate(x_pos-t_size/2, y_pos-t_size/2, z_carriage_pos-t_height)
            # Then apply T rotation around Z-axis
            self.t_part.rotate(t_pos / 10, 0, 0, 1)  # Scale down rotation for visualization

            # Calculate gripper opening based on g_pos (0 = open, 1000 = closed)
            gripper_spacing = max(30, 150 - g_pos / 10)  # Min spacing = 30, Max = 150

            # Reset gripper parts
            self.g_left.resetTransform()
            self.g_right.resetTransform()

            # Apply T rotation to both gripper parts
            self.g_left.rotate(t_pos / 10, 0, 0, 1)
            self.g_right.rotate(t_pos / 10, 0, 0, 1)

            # Position grippers directly at the bottom of T, maintaining connection
            self.g_left.translate(x_pos-gripper_spacing/2-gripper_thickness, y_pos-gripper_length/2, z_carriage_pos-t_height-gripper_height)
            self.g_right.translate(x_pos+gripper_spacing/2, y_pos-gripper_length/2, z_carriage_pos-t_height-gripper_height)

        except Exception as e:
            print(f"Error updating visualization: {str(e)}")

    def set_view(self, view_type):
        """Set the camera to a specific view"""
        # Make sure visualization is initialized
        if not self.initialized:
            return

        # Uncheck all view checkboxes
        self.top_view_cb.setChecked(False)
        self.side_view_cb.setChecked(False)
        self.front_view_cb.setChecked(False)
        self.isometric_view_cb.setChecked(False)

        # Set the appropriate checkbox
        if view_type == 'top':
            self.top_view_cb.setChecked(True)
            self.view.setCameraPosition(distance=2000, elevation=90, azimuth=0)
        elif view_type == 'side':
            self.side_view_cb.setChecked(True)
            self.view.setCameraPosition(distance=2000, elevation=0, azimuth=0)
        elif view_type == 'front':
            self.front_view_cb.setChecked(True)
            self.view.setCameraPosition(distance=2000, elevation=0, azimuth=90)
        elif view_type == 'isometric':
            self.isometric_view_cb.setChecked(True)
            self.view.setCameraPosition(distance=2000, elevation=30, azimuth=45)

    def update_position(self, axis_id, position):
        """Update the position for a specific axis"""
        if axis_id.lower() in self.positions:
            self.positions[axis_id.lower()] = position
            self.pos_labels[axis_id.lower()].setText(str(position))

            # Only update slider if the value is within range
            min_val = self.sliders[axis_id.lower()].minimum()
            max_val = self.sliders[axis_id.lower()].maximum()

            if min_val <= position <= max_val:
                self.sliders[axis_id.lower()].setValue(position)
            else:
                # If position is outside slider range, update the range
                if position < min_val:
                    self.min_inputs[axis_id.lower()].setValue(position)
                    self.axis_ranges[axis_id.lower()]['min'] = position
                elif position > max_val:
                    self.max_inputs[axis_id.lower()].setValue(position)
                    self.axis_ranges[axis_id.lower()]['max'] = position

                # Apply the new range
                self.apply_all_ranges()

                # Now the position should be within range, set the slider
                self.sliders[axis_id.lower()].setValue(position)

            self.update_visualization()

    def reset_all_positions(self):
        """Reset all position displays to zero"""
        for axis_id in SLAVE_IDS:
            self.update_position(axis_id, 0)
