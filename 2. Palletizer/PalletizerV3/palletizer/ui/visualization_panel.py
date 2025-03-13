import numpy as np
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QGroupBox,
                             QPushButton, QLabel, QSlider, QCheckBox, QSplitter, QFrame,
                             QSpinBox, QScrollArea, QSizePolicy)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QVector3D, QColor
import pyqtgraph as pg
import pyqtgraph.opengl as gl
from ..utils.config import *


class VisualizationPanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.positions = {'x': 0, 'y': 0, 'z': 0, 't': 0, 'g': 0}
        self.axis_ranges = {
            'x': {'min': -1000, 'max': 1000},
            'y': {'min': -1000, 'max': 1000},
            'z': {'min': -1000, 'max': 1000},
            't': {'min': -1000, 'max': 1000},
            'g': {'min': -1000, 'max': 1000}
        }
        self.initialized = False
        self.gl_items = {}
        self.setup_ui()
        QTimer.singleShot(100, self.initialize_visualization)

    def setup_ui(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.setSpacing(5)

        splitter = QSplitter(Qt.Horizontal)
        splitter.setChildrenCollapsible(False)

        # Left side with 3D view
        vis_widget = QWidget()
        vis_layout = QVBoxLayout(vis_widget)
        vis_layout.setContentsMargins(0, 0, 0, 0)

        view_frame = QFrame()
        view_frame.setFrameShape(QFrame.StyledPanel)
        view_frame.setFrameShadow(QFrame.Sunken)
        view_frame.setMinimumSize(400, 300)
        view_frame.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.view_layout = QVBoxLayout(view_frame)
        self.view_layout.setContentsMargins(0, 0, 0, 0)

        self.view_placeholder = QLabel("Initializing 3D View...")
        self.view_placeholder.setAlignment(Qt.AlignCenter)
        self.view_placeholder.setStyleSheet("background-color: #f0f0f0; border: 1px solid #cccccc;")
        self.view_layout.addWidget(self.view_placeholder)

        vis_layout.addWidget(view_frame, 1)

        info_group = QGroupBox("Visualization Information")
        info_layout = QVBoxLayout(info_group)
        info_layout.setContentsMargins(5, 10, 5, 5)

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

        vis_layout.addWidget(info_group, 0)

        # Right side with controls - using scroll area for better display
        control_scroll = QScrollArea()
        control_scroll.setWidgetResizable(True)
        control_scroll.setFrameShape(QFrame.NoFrame)
        control_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        control_widget = QWidget()
        control_layout = QVBoxLayout(control_widget)
        control_layout.setContentsMargins(5, 5, 5, 5)
        control_layout.setSpacing(5)

        # Current Positions Group
        position_group = QGroupBox("Current Positions")
        position_layout = QGridLayout(position_group)
        position_layout.setVerticalSpacing(2)

        self.pos_labels = {}
        row = 0
        for axis in SLAVE_IDS:
            position_layout.addWidget(QLabel(f"{axis.upper()}-AXIS:"), row, 0)

            pos_label = QLabel("0")
            pos_label.setStyleSheet("font-family: monospace; font-weight: bold;")
            self.pos_labels[axis.lower()] = pos_label

            position_layout.addWidget(pos_label, row, 1)
            row += 1

        control_layout.addWidget(position_group)

        # Axis Range Settings Group
        range_group = QGroupBox("Axis Range Settings")
        range_layout = QGridLayout(range_group)
        range_layout.setVerticalSpacing(2)

        self.min_inputs = {}
        self.max_inputs = {}

        row = 0
        for axis in SLAVE_IDS:
            # Axis header
            range_layout.addWidget(QLabel(f"{axis.upper()}-AXIS Range"), row, 0, 1, 3)
            row += 1

            # Min value
            range_layout.addWidget(QLabel("Min:"), row, 0)
            min_input = QSpinBox()
            min_input.setRange(-100000, 100000)
            min_input.setValue(self.axis_ranges[axis.lower()]['min'])
            min_input.setSingleStep(100)
            min_input.valueChanged.connect(lambda v, a=axis.lower(): self.on_min_changed(a, v))
            self.min_inputs[axis.lower()] = min_input
            range_layout.addWidget(min_input, row, 1, 1, 2)
            row += 1

            # Max value
            range_layout.addWidget(QLabel("Max:"), row, 0)
            max_input = QSpinBox()
            max_input.setRange(-100000, 100000)
            max_input.setValue(self.axis_ranges[axis.lower()]['max'])
            max_input.setSingleStep(100)
            max_input.valueChanged.connect(lambda v, a=axis.lower(): self.on_max_changed(a, v))
            self.max_inputs[axis.lower()] = max_input
            range_layout.addWidget(max_input, row, 1, 1, 2)
            row += 1

            # Add a small spacer
            if axis != SLAVE_IDS[-1]:  # Not the last axis
                spacer = QLabel("")
                spacer.setFixedHeight(5)
                range_layout.addWidget(spacer, row, 0, 1, 3)
                row += 1

        # Add control buttons
        button_layout = QHBoxLayout()

        apply_btn = QPushButton("Apply All Range Settings")
        apply_btn.clicked.connect(self.apply_all_ranges)
        apply_btn.setStyleSheet("background-color: #e0e0ff;")

        reset_ranges_btn = QPushButton("Reset to Default Ranges")
        reset_ranges_btn.clicked.connect(self.reset_ranges)
        reset_ranges_btn.setStyleSheet("background-color: #ffe0e0;")

        button_layout.addWidget(apply_btn)
        button_layout.addWidget(reset_ranges_btn)

        range_layout.addLayout(button_layout, row, 0, 1, 3)
        control_layout.addWidget(range_group)

        # Axis Movement Group
        movement_group = QGroupBox("Axis Movement")
        movement_layout = QVBoxLayout(movement_group)
        movement_layout.setSpacing(5)

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

        reset_btn = QPushButton("Reset All Positions")
        reset_btn.clicked.connect(self.reset_positions)
        reset_btn.setStyleSheet("background-color: #ffffcc;")
        movement_layout.addWidget(reset_btn)

        control_layout.addWidget(movement_group)

        # View Controls Group
        view_group = QGroupBox("View Controls")
        view_layout = QVBoxLayout(view_group)
        view_layout.setSpacing(5)

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

        control_layout.addWidget(view_group)

        # Add a stretch at the end to push everything up
        control_layout.addStretch()

        # Set the widget to the scroll area
        control_scroll.setWidget(control_widget)

        # Add the main panels to the splitter
        splitter.addWidget(vis_widget)
        splitter.addWidget(control_scroll)

        # Set the splitter sizes (60% view, 40% controls)
        splitter.setSizes([600, 400])

        main_layout.addWidget(splitter)

    def on_min_changed(self, axis, value):
        if value >= self.max_inputs[axis].value():
            self.min_inputs[axis].setValue(self.max_inputs[axis].value() - 1)
            return
        self.axis_ranges[axis]['min'] = value

    def on_max_changed(self, axis, value):
        if value <= self.min_inputs[axis].value():
            self.max_inputs[axis].setValue(self.min_inputs[axis].value() + 1)
            return
        self.axis_ranges[axis]['max'] = value

    def apply_all_ranges(self):
        for axis in SLAVE_IDS:
            axis_lower = axis.lower()
            self.sliders[axis_lower].setMinimum(self.axis_ranges[axis_lower]['min'])
            self.sliders[axis_lower].setMaximum(self.axis_ranges[axis_lower]['max'])

            current_pos = self.positions[axis_lower]
            min_val = self.axis_ranges[axis_lower]['min']
            max_val = self.axis_ranges[axis_lower]['max']

            if current_pos < min_val:
                self.update_position(axis_lower, min_val)
            elif current_pos > max_val:
                self.update_position(axis_lower, max_val)

            range_size = max_val - min_val
            tick_interval = max(int(range_size / 10), 1)
            self.sliders[axis_lower].setTickInterval(tick_interval)

    def reset_ranges(self):
        default_min = -1000
        default_max = 1000

        for axis in SLAVE_IDS:
            self.axis_ranges[axis.lower()]['min'] = default_min
            self.axis_ranges[axis.lower()]['max'] = default_max

            self.min_inputs[axis.lower()].setValue(default_min)
            self.max_inputs[axis.lower()].setValue(default_max)

        self.apply_all_ranges()

    def initialize_visualization(self):
        try:
            for i in reversed(range(self.view_layout.count())):
                item = self.view_layout.itemAt(i)
                if item.widget():
                    item.widget().setParent(None)

            self.view = gl.GLViewWidget()
            self.view.setCameraPosition(distance=2000, elevation=30, azimuth=45)
            self.view.setBackgroundColor(255, 255, 255, 255)

            self.view_layout.addWidget(self.view)

            grid = gl.GLGridItem()
            grid.setSize(x=2000, y=2000)
            grid.setSpacing(x=100, y=100)
            self.view.addItem(grid)
            self.gl_items['grid'] = grid

            x_axis_line = np.array([[0, 0, 0], [1000, 0, 0]])
            self.x_axis = gl.GLLinePlotItem(pos=x_axis_line, color=(1, 0, 0, 1), width=3)
            self.view.addItem(self.x_axis)
            self.gl_items['x_axis'] = self.x_axis

            y_axis_line = np.array([[0, 0, 0], [0, 1000, 0]])
            self.y_axis = gl.GLLinePlotItem(pos=y_axis_line, color=(0, 1, 0, 1), width=3)
            self.view.addItem(self.y_axis)
            self.gl_items['y_axis'] = self.y_axis

            z_axis_line = np.array([[0, 0, 0], [0, 0, 1000]])
            self.z_axis = gl.GLLinePlotItem(pos=z_axis_line, color=(0, 0, 1, 1), width=3)
            self.view.addItem(self.z_axis)
            self.gl_items['z_axis'] = self.z_axis

            self.x_label = gl.GLTextItem(pos=np.array([1050, 0, 0]), text='X-AXIS', color=(1, 0, 0, 1))
            self.view.addItem(self.x_label)
            self.gl_items['x_label'] = self.x_label

            self.y_label = gl.GLTextItem(pos=np.array([0, 1050, 0]), text='Y-AXIS', color=(0, 1, 0, 1))
            self.view.addItem(self.y_label)
            self.gl_items['y_label'] = self.y_label

            self.z_label = gl.GLTextItem(pos=np.array([0, 0, 1050]), text='Z-AXIS', color=(0, 0, 1, 1))
            self.view.addItem(self.z_label)
            self.gl_items['z_label'] = self.z_label

            self.x_rail = gl.GLBoxItem(size=QVector3D(1500, 50, 50))
            self.x_rail.setColor(QColor(128, 128, 128, 255))
            self.x_rail.translate(-750, 0, 0)
            self.view.addItem(self.x_rail)
            self.gl_items['x_rail'] = self.x_rail

            self.y_rail = gl.GLBoxItem(size=QVector3D(50, 500, 50))
            self.y_rail.setColor(QColor(150, 150, 150, 255))
            self.y_rail.translate(0, -500, 0)
            self.view.addItem(self.y_rail)
            self.gl_items['y_rail'] = self.y_rail

            z_width = 50
            z_depth = 50
            z_length = 1200

            self.z_rail = gl.GLBoxItem(size=QVector3D(z_width, z_depth, z_length))
            self.z_rail.setColor(QColor(100, 100, 255, 255))
            self.z_rail.translate(-z_width/2, -z_depth/2, -z_length)
            self.view.addItem(self.z_rail)
            self.gl_items['z_rail'] = self.z_rail

            carriage_size = 80
            carriage_height = 60
            self.z_carriage = gl.GLBoxItem(size=QVector3D(carriage_size, carriage_size, carriage_height))
            self.z_carriage.setColor(QColor(180, 180, 180, 255))
            self.z_carriage.translate(-carriage_size/2, -carriage_size/2, -carriage_height)
            self.view.addItem(self.z_carriage)
            self.gl_items['z_carriage'] = self.z_carriage

            t_size = 180
            t_height = 40

            self.t_part = gl.GLBoxItem(size=QVector3D(t_size, t_size, t_height))
            self.t_part.setColor(QColor(200, 200, 200, 255))
            self.t_part.translate(-t_size/2, -t_size/2, -carriage_height-t_height)
            self.view.addItem(self.t_part)
            self.gl_items['t_part'] = self.t_part

            gripper_thickness = 30
            gripper_length = 200
            gripper_height = 100
            gripper_spacing = 150

            self.g_left = gl.GLBoxItem(size=QVector3D(gripper_thickness, gripper_length, gripper_height))
            self.g_left.setColor(QColor(220, 220, 220, 255))
            self.g_left.translate(-gripper_spacing/2-gripper_thickness, -gripper_length/2, -carriage_height-t_height-gripper_height)
            self.view.addItem(self.g_left)
            self.gl_items['g_left'] = self.g_left

            self.g_right = gl.GLBoxItem(size=QVector3D(gripper_thickness, gripper_length, gripper_height))
            self.g_right.setColor(QColor(220, 220, 220, 255))
            self.g_right.translate(gripper_spacing/2, -gripper_length/2, -carriage_height-t_height-gripper_height)
            self.view.addItem(self.g_right)
            self.gl_items['g_right'] = self.g_right

            self.initialized = True
            self.set_view('isometric')
            self.update_visualization()

        except Exception as e:
            error_label = QLabel(f"3D Visualization Error: {str(e)}\n\nPlease check if PyQtGraph and PyOpenGL are properly installed.")
            error_label.setWordWrap(True)
            error_label.setStyleSheet("color: red; background-color: #ffeeee; padding: 10px; border: 1px solid red;")
            self.view_layout.addWidget(error_label)

    def on_slider_changed(self, axis, value):
        self.positions[axis] = value
        self.pos_labels[axis].setText(str(value))
        self.update_visualization()

    def reset_positions(self):
        for axis in SLAVE_IDS:
            self.positions[axis.lower()] = 0
            self.sliders[axis.lower()].setValue(0)
            self.pos_labels[axis.lower()].setText("0")
        self.update_visualization()

    def update_visualization(self):
        if not self.initialized:
            return

        try:
            x_pos = self.positions['x']
            y_pos = self.positions['y']
            z_pos = self.positions['z']
            t_pos = self.positions['t']
            g_pos = self.positions['g']

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

            self.y_rail.resetTransform()
            self.y_rail.translate(-x_pos, -500, 0)

            self.z_rail.resetTransform()
            self.z_rail.translate(-x_pos-z_width/2, y_pos-z_depth/2, -z_length)

            z_carriage_pos = -carriage_height + (z_pos / 1000) * (-z_length + carriage_height + 50)
            z_carriage_pos = max(-z_length + carriage_height, min(-carriage_height, z_carriage_pos))

            self.z_carriage.resetTransform()
            self.z_carriage.translate(-x_pos-carriage_size/2, y_pos-carriage_size/2, z_carriage_pos)

            self.t_part.resetTransform()
            self.t_part.translate(-x_pos-t_size/2, y_pos-t_size/2, z_carriage_pos-t_height)
            self.t_part.rotate(t_pos / 10, 0, 0, 1)

            gripper_spacing = max(30, 150 - g_pos / 10)

            self.g_left.resetTransform()
            self.g_right.resetTransform()

            self.g_left.rotate(t_pos / 10, 0, 0, 1)
            self.g_right.rotate(t_pos / 10, 0, 0, 1)

            self.g_left.translate(-x_pos-gripper_spacing/2-gripper_thickness, y_pos-gripper_length/2, z_carriage_pos-t_height-gripper_height)
            self.g_right.translate(-x_pos+gripper_spacing/2, y_pos-gripper_length/2, z_carriage_pos-t_height-gripper_height)

        except Exception as e:
            print(f"Error updating visualization: {str(e)}")

    def set_view(self, view_type):
        if not self.initialized:
            return

        self.top_view_cb.setChecked(False)
        self.side_view_cb.setChecked(False)
        self.front_view_cb.setChecked(False)
        self.isometric_view_cb.setChecked(False)

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
        if axis_id.lower() in self.positions:
            self.positions[axis_id.lower()] = position
            self.pos_labels[axis_id.lower()].setText(str(position))

            min_val = self.sliders[axis_id.lower()].minimum()
            max_val = self.sliders[axis_id.lower()].maximum()

            if min_val <= position <= max_val:
                self.sliders[axis_id.lower()].setValue(position)
            else:
                if position < min_val:
                    self.min_inputs[axis_id.lower()].setValue(position)
                    self.axis_ranges[axis_id.lower()]['min'] = position
                elif position > max_val:
                    self.max_inputs[axis_id.lower()].setValue(position)
                    self.axis_ranges[axis_id.lower()]['max'] = position

                self.apply_all_ranges()
                self.sliders[axis_id.lower()].setValue(position)

            self.update_visualization()

    def reset_all_positions(self):
        for axis_id in SLAVE_IDS:
            self.update_position(axis_id, 0)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if hasattr(self, 'view') and self.initialized:
            self.view.update()
