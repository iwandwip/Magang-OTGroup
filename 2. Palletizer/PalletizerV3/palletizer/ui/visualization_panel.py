import numpy as np
import yaml
import os
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QGroupBox,
                             QPushButton, QLabel, QSlider, QCheckBox, QSplitter, QFrame,
                             QSpinBox, QScrollArea, QSizePolicy, QFileDialog, QMessageBox)
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
            'x': {'min': -10000, 'max': 10000},
            'y': {'min': -10000, 'max': 10000},
            'z': {'min': -10000, 'max': 10000},
            't': {'min': -10000, 'max': 10000},
            'g': {'min': -10000, 'max': 10000}
        }

        self.rail_lengths = {
            'x': 1500,
            'y': 500,
            'z': 1200
        }

        self.relative_positions = {
            'x_offset': 0,
            'y_offset': 0,
            'z_offset': 0
        }

        self.axis_inverted = {
            'x': False,
            'y': False,
            'z': False,
            't': False,
            'g': False
        }

        # Configuration name for saving/loading
        self.config_name = "Default Configuration"

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

        # Configuration Save/Load Section
        config_group = QGroupBox("Configuration Management")
        config_layout = QVBoxLayout()
        config_layout.setSpacing(5)

        # Config name display
        name_layout = QHBoxLayout()
        name_layout.addWidget(QLabel("Current Config:"))
        self.config_name_label = QLabel(self.config_name)
        self.config_name_label.setStyleSheet("font-weight: bold;")
        name_layout.addWidget(self.config_name_label)
        name_layout.addStretch()
        config_layout.addLayout(name_layout)

        # Save/Load buttons
        buttons_layout = QHBoxLayout()
        buttons_layout.setSpacing(5)

        self.save_config_btn = QPushButton("Save Configuration")
        self.save_config_btn.clicked.connect(self.save_configuration)
        self.save_config_btn.setStyleSheet("background-color: #ccffcc;")

        self.load_config_btn = QPushButton("Load Configuration")
        self.load_config_btn.clicked.connect(self.load_configuration)
        self.load_config_btn.setStyleSheet("background-color: #e0e0ff;")

        buttons_layout.addWidget(self.save_config_btn)
        buttons_layout.addWidget(self.load_config_btn)
        config_layout.addLayout(buttons_layout)

        config_group.setLayout(config_layout)
        control_layout.addWidget(config_group)

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

        # Rail Lengths Group
        rail_length_group = QGroupBox("Rail Length Settings")
        rail_length_layout = QGridLayout(rail_length_group)
        rail_length_layout.setVerticalSpacing(5)

        self.rail_length_inputs = {}

        row = 0
        for axis in ['x', 'y', 'z']:
            rail_length_layout.addWidget(QLabel(f"{axis.upper()}-Rail Length:"), row, 0)

            length_input = QSpinBox()
            length_input.setRange(100, 3000)
            length_input.setValue(self.rail_lengths[axis])
            length_input.setSingleStep(100)
            length_input.valueChanged.connect(lambda v, a=axis: self.on_rail_length_changed(a, v))
            self.rail_length_inputs[axis] = length_input
            rail_length_layout.addWidget(length_input, row, 1)

            row += 1

        apply_rail_btn = QPushButton("Apply Rail Lengths")
        apply_rail_btn.clicked.connect(self.apply_rail_lengths)
        apply_rail_btn.setStyleSheet("background-color: #ccffcc;")
        rail_length_layout.addWidget(apply_rail_btn, row, 0, 1, 2)

        row += 1

        reset_rail_btn = QPushButton("Reset to Default Lengths")
        reset_rail_btn.clicked.connect(self.reset_rail_lengths)
        reset_rail_btn.setStyleSheet("background-color: #ffcccc;")
        rail_length_layout.addWidget(reset_rail_btn, row, 0, 1, 2)

        control_layout.addWidget(rail_length_group)

        # Relative Position Group - Now labeled "Axis Base Offsets"
        rel_pos_group = QGroupBox("Axis Base Offsets")
        rel_pos_layout = QGridLayout(rel_pos_group)
        rel_pos_layout.setVerticalSpacing(5)

        self.rel_pos_inputs = {}

        # Add info label explaining axis dependencies
        info_label = QLabel("These offsets set the base positions of each axis. The mechanical dependencies (X→Y→Z) are preserved.")
        info_label.setWordWrap(True)
        info_label.setStyleSheet("font-style: italic; color: #666666;")
        rel_pos_layout.addWidget(info_label, 0, 0, 1, 2)

        rel_pos_layout.addWidget(QLabel("X-Axis Base Offset:"), 1, 0)
        x_offset_input = QSpinBox()
        x_offset_input.setRange(-1000, 1000)
        x_offset_input.setValue(self.relative_positions['x_offset'])
        x_offset_input.setSingleStep(10)
        x_offset_input.valueChanged.connect(lambda v: self.on_rel_pos_changed('x_offset', v))
        self.rel_pos_inputs['x_offset'] = x_offset_input
        rel_pos_layout.addWidget(x_offset_input, 1, 1)

        rel_pos_layout.addWidget(QLabel("Y-Axis Base Offset:"), 2, 0)
        y_offset_input = QSpinBox()
        y_offset_input.setRange(-1000, 1000)
        y_offset_input.setValue(self.relative_positions['y_offset'])
        y_offset_input.setSingleStep(10)
        y_offset_input.valueChanged.connect(lambda v: self.on_rel_pos_changed('y_offset', v))
        self.rel_pos_inputs['y_offset'] = y_offset_input
        rel_pos_layout.addWidget(y_offset_input, 2, 1)

        rel_pos_layout.addWidget(QLabel("Z-Axis Base Offset:"), 3, 0)
        z_offset_input = QSpinBox()
        z_offset_input.setRange(-1000, 1000)
        z_offset_input.setValue(self.relative_positions['z_offset'])
        z_offset_input.setSingleStep(10)
        z_offset_input.valueChanged.connect(lambda v: self.on_rel_pos_changed('z_offset', v))
        self.rel_pos_inputs['z_offset'] = z_offset_input
        rel_pos_layout.addWidget(z_offset_input, 3, 1)

        apply_pos_btn = QPushButton("Apply Base Offsets")
        apply_pos_btn.clicked.connect(self.apply_relative_positions)
        apply_pos_btn.setStyleSheet("background-color: #ccffcc;")
        rel_pos_layout.addWidget(apply_pos_btn, 4, 0, 1, 2)

        reset_pos_btn = QPushButton("Reset Base Offsets")
        reset_pos_btn.clicked.connect(self.reset_relative_positions)
        reset_pos_btn.setStyleSheet("background-color: #ffcccc;")
        rel_pos_layout.addWidget(reset_pos_btn, 5, 0, 1, 2)

        control_layout.addWidget(rel_pos_group)

        # Axis Range Settings Group
        range_group = QGroupBox("Axis Range Settings")
        range_layout = QGridLayout(range_group)
        range_layout.setVerticalSpacing(2)

        self.min_inputs = {}
        self.max_inputs = {}

        row = 0
        for axis in SLAVE_IDS:
            range_layout.addWidget(QLabel(f"{axis.upper()}-AXIS Range"), row, 0, 1, 3)
            row += 1

            range_layout.addWidget(QLabel("Min:"), row, 0)
            min_input = QSpinBox()
            min_input.setRange(-100000, 100000)
            min_input.setValue(self.axis_ranges[axis.lower()]['min'])
            min_input.setSingleStep(100)
            min_input.valueChanged.connect(lambda v, a=axis.lower(): self.on_min_changed(a, v))
            self.min_inputs[axis.lower()] = min_input
            range_layout.addWidget(min_input, row, 1, 1, 2)
            row += 1

            range_layout.addWidget(QLabel("Max:"), row, 0)
            max_input = QSpinBox()
            max_input.setRange(-100000, 100000)
            max_input.setValue(self.axis_ranges[axis.lower()]['max'])
            max_input.setSingleStep(100)
            max_input.valueChanged.connect(lambda v, a=axis.lower(): self.on_max_changed(a, v))
            self.max_inputs[axis.lower()] = max_input
            range_layout.addWidget(max_input, row, 1, 1, 2)
            row += 1

            if axis != SLAVE_IDS[-1]:
                spacer = QLabel("")
                spacer.setFixedHeight(5)
                range_layout.addWidget(spacer, row, 0, 1, 3)
                row += 1

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

        # Axis Movement Group with invert checkboxes
        movement_group = QGroupBox("Axis Movement")
        movement_layout = QVBoxLayout(movement_group)
        movement_layout.setSpacing(5)

        self.sliders = {}
        self.invert_checkboxes = {}

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

            invert_cb = QCheckBox("Invert")
            invert_cb.setChecked(self.axis_inverted[axis.lower()])
            invert_cb.stateChanged.connect(lambda state, a=axis.lower(): self.on_invert_changed(a, state))
            self.invert_checkboxes[axis.lower()] = invert_cb

            axis_layout.addWidget(slider)
            axis_layout.addWidget(invert_cb)

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

        # Add stretch at the end to push everything up
        control_layout.addStretch()

        control_scroll.setWidget(control_widget)

        splitter.addWidget(vis_widget)
        splitter.addWidget(control_scroll)

        splitter.setSizes([600, 400])

        main_layout.addWidget(splitter)

    def save_configuration(self):
        """Save current configuration to a YAML file"""
        # Ask for a configuration name
        config_name, ok = QFileDialog.getSaveFileName(
            self, "Save Configuration", "", "YAML Files (*.yaml *.yml);;All Files (*)"
        )

        if not config_name:
            return

        # Set the config name to the filename without extension
        base_name = os.path.basename(config_name)
        self.config_name = os.path.splitext(base_name)[0]
        self.config_name_label.setText(self.config_name)

        try:
            # Prepare configuration data structure
            config_data = {
                'name': self.config_name,
                'rail_lengths': self.rail_lengths,
                'relative_positions': self.relative_positions,
                'axis_inverted': self.axis_inverted,
                'axis_ranges': self.axis_ranges,
                'camera': {
                    'distance': 2000,
                    'elevation': 30,
                    'azimuth': 45
                }
            }

            # Save to file
            with open(config_name, 'w') as file:
                yaml.dump(config_data, file, default_flow_style=False)

            QMessageBox.information(self, "Configuration Saved",
                                    f"Configuration saved successfully to {config_name}")
        except Exception as e:
            QMessageBox.critical(self, "Save Error", f"Error saving configuration: {str(e)}")

    def load_configuration(self):
        """Load configuration from a YAML file"""
        config_name, _ = QFileDialog.getOpenFileName(
            self, "Load Configuration", "", "YAML Files (*.yaml *.yml);;All Files (*)"
        )

        if not config_name:
            return

        try:
            # Load from file
            with open(config_name, 'r') as file:
                config_data = yaml.safe_load(file)

            # Update configuration name
            self.config_name = config_data.get('name', os.path.basename(config_name))
            self.config_name_label.setText(self.config_name)

            # Update rail lengths
            if 'rail_lengths' in config_data:
                self.rail_lengths = config_data['rail_lengths']
                for axis, value in self.rail_lengths.items():
                    if axis in self.rail_length_inputs:
                        self.rail_length_inputs[axis].setValue(value)

            # Update relative positions
            if 'relative_positions' in config_data:
                self.relative_positions = config_data['relative_positions']
                for key, value in self.relative_positions.items():
                    if key in self.rel_pos_inputs:
                        self.rel_pos_inputs[key].setValue(value)

            # Update axis inversion
            if 'axis_inverted' in config_data:
                self.axis_inverted = config_data['axis_inverted']
                for axis, value in self.axis_inverted.items():
                    if axis in self.invert_checkboxes:
                        self.invert_checkboxes[axis].setChecked(value)

            # Update axis ranges
            if 'axis_ranges' in config_data:
                self.axis_ranges = config_data['axis_ranges']
                for axis, range_values in self.axis_ranges.items():
                    if axis in self.min_inputs:
                        self.min_inputs[axis].setValue(range_values['min'])
                    if axis in self.max_inputs:
                        self.max_inputs[axis].setValue(range_values['max'])

            # Apply all settings
            self.apply_rail_lengths()
            self.apply_relative_positions()
            self.apply_all_ranges()

            # Update camera if included
            if 'camera' in config_data and self.initialized:
                camera = config_data['camera']
                if all(k in camera for k in ['distance', 'elevation', 'azimuth']):
                    self.view.setCameraPosition(
                        distance=camera['distance'],
                        elevation=camera['elevation'],
                        azimuth=camera['azimuth']
                    )

            QMessageBox.information(self, "Configuration Loaded",
                                    f"Configuration loaded successfully from {config_name}")
        except Exception as e:
            QMessageBox.critical(self, "Load Error", f"Error loading configuration: {str(e)}")

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

    def on_rail_length_changed(self, axis, value):
        self.rail_lengths[axis] = value

    def on_rel_pos_changed(self, position_key, value):
        self.relative_positions[position_key] = value

    def on_invert_changed(self, axis, state):
        self.axis_inverted[axis] = (state == Qt.Checked)

    def apply_rail_lengths(self):
        if not self.initialized:
            return

        try:
            x_length = self.rail_lengths['x']
            y_length = self.rail_lengths['y']
            z_length = self.rail_lengths['z']

            # X rail - base position with X offset
            if 'x_rail' in self.gl_items:
                self.view.removeItem(self.gl_items['x_rail'])
                self.x_rail = gl.GLBoxItem(size=QVector3D(x_length, 50, 50))
                self.x_rail.setColor(QColor(128, 128, 128, 255))
                self.x_rail.translate(-x_length/2 + self.relative_positions['x_offset'], 0, 0)
                self.view.addItem(self.x_rail)
                self.gl_items['x_rail'] = self.x_rail

            if 'x_axis' in self.gl_items:
                self.view.removeItem(self.gl_items['x_axis'])
                x_axis_line = np.array([[0, 0, 0], [x_length/2, 0, 0]])
                self.x_axis = gl.GLLinePlotItem(pos=x_axis_line, color=(1, 0, 0, 1), width=3)
                self.view.addItem(self.x_axis)
                self.gl_items['x_axis'] = self.x_axis

                if 'x_label' in self.gl_items:
                    self.view.removeItem(self.gl_items['x_label'])
                    self.x_label = gl.GLTextItem(pos=np.array([x_length/2 + 50, 0, 0]), text='X-AXIS', color=(1, 0, 0, 1))
                    self.view.addItem(self.x_label)
                    self.gl_items['x_label'] = self.x_label

            # Y rail - base position with Y offset (already dependent on X position in update_visualization)
            if 'y_rail' in self.gl_items:
                self.view.removeItem(self.gl_items['y_rail'])
                self.y_rail = gl.GLBoxItem(size=QVector3D(50, y_length, 50))
                self.y_rail.setColor(QColor(150, 150, 150, 255))
                # The actual position will be set in update_visualization()
                self.view.addItem(self.y_rail)
                self.gl_items['y_rail'] = self.y_rail

            if 'y_axis' in self.gl_items:
                self.view.removeItem(self.gl_items['y_axis'])
                y_axis_line = np.array([[0, 0, 0], [0, -y_length/2, 0]])
                self.y_axis = gl.GLLinePlotItem(pos=y_axis_line, color=(0, 1, 0, 1), width=3)
                self.view.addItem(self.y_axis)
                self.gl_items['y_axis'] = self.y_axis

                if 'y_label' in self.gl_items:
                    self.view.removeItem(self.gl_items['y_label'])
                    self.y_label = gl.GLTextItem(pos=np.array([0, -y_length/2 - 50, 0]), text='Y-AXIS', color=(0, 1, 0, 1))
                    self.view.addItem(self.y_label)
                    self.gl_items['y_label'] = self.y_label

            # Z rail - base position with Z offset (dependent on X and Y positions in update_visualization)
            if 'z_rail' in self.gl_items:
                self.view.removeItem(self.gl_items['z_rail'])
                z_width = 50
                z_depth = 50
                self.z_rail = gl.GLBoxItem(size=QVector3D(z_width, z_depth, z_length))
                self.z_rail.setColor(QColor(100, 100, 255, 255))
                # The actual position will be set in update_visualization()
                self.view.addItem(self.z_rail)
                self.gl_items['z_rail'] = self.z_rail

            if 'z_axis' in self.gl_items:
                self.view.removeItem(self.gl_items['z_axis'])
                z_axis_line = np.array([[0, 0, 0], [0, 0, z_length/2]])
                self.z_axis = gl.GLLinePlotItem(pos=z_axis_line, color=(0, 0, 1, 1), width=3)
                self.view.addItem(self.z_axis)
                self.gl_items['z_axis'] = self.z_axis

                if 'z_label' in self.gl_items:
                    self.view.removeItem(self.gl_items['z_label'])
                    self.z_label = gl.GLTextItem(pos=np.array([0, 0, z_length/2 + 50]), text='Z-AXIS', color=(0, 0, 1, 1))
                    self.view.addItem(self.z_label)
                    self.gl_items['z_label'] = self.z_label

            self.update_visualization()
        except Exception as e:
            print(f"Error applying rail lengths: {str(e)}")

    def reset_rail_lengths(self):
        self.rail_lengths = {
            'x': 1500,
            'y': 500,
            'z': 1200
        }

        for axis, value in self.rail_lengths.items():
            self.rail_length_inputs[axis].setValue(value)

        self.apply_rail_lengths()

    def apply_relative_positions(self):
        if not self.initialized:
            return

        try:
            # The rail positions will be updated in update_visualization
            # which properly applies the dependencies between axes
            self.update_visualization()
        except Exception as e:
            print(f"Error applying relative positions: {str(e)}")

    def reset_relative_positions(self):
        self.relative_positions = {
            'x_offset': 0,
            'y_offset': 0,
            'z_offset': 0
        }

        for key, value in self.relative_positions.items():
            self.rel_pos_inputs[key].setValue(value)

        self.apply_relative_positions()

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

            x_length = self.rail_lengths['x']
            y_length = self.rail_lengths['y']
            z_length = self.rail_lengths['z']

            x_offset = self.relative_positions['x_offset']
            y_offset = self.relative_positions['y_offset']
            z_offset = self.relative_positions['z_offset']

            # Create axis reference lines
            x_axis_line = np.array([[0, 0, 0], [x_length/2, 0, 0]])
            self.x_axis = gl.GLLinePlotItem(pos=x_axis_line, color=(1, 0, 0, 1), width=3)
            self.view.addItem(self.x_axis)
            self.gl_items['x_axis'] = self.x_axis

            y_axis_line = np.array([[0, 0, 0], [0, -y_length/2, 0]])
            self.y_axis = gl.GLLinePlotItem(pos=y_axis_line, color=(0, 1, 0, 1), width=3)
            self.view.addItem(self.y_axis)
            self.gl_items['y_axis'] = self.y_axis

            z_axis_line = np.array([[0, 0, 0], [0, 0, z_length/2]])
            self.z_axis = gl.GLLinePlotItem(pos=z_axis_line, color=(0, 0, 1, 1), width=3)
            self.view.addItem(self.z_axis)
            self.gl_items['z_axis'] = self.z_axis

            # Axis labels
            self.x_label = gl.GLTextItem(pos=np.array([x_length/2 + 50, 0, 0]), text='X-AXIS', color=(1, 0, 0, 1))
            self.view.addItem(self.x_label)
            self.gl_items['x_label'] = self.x_label

            self.y_label = gl.GLTextItem(pos=np.array([0, -y_length/2 - 50, 0]), text='Y-AXIS', color=(0, 1, 0, 1))
            self.view.addItem(self.y_label)
            self.gl_items['y_label'] = self.y_label

            self.z_label = gl.GLTextItem(pos=np.array([0, 0, z_length/2 + 50]), text='Z-AXIS', color=(0, 0, 1, 1))
            self.view.addItem(self.z_label)
            self.gl_items['z_label'] = self.z_label

            # X rail - base position only affected by X offset
            self.x_rail = gl.GLBoxItem(size=QVector3D(x_length, 50, 50))
            self.x_rail.setColor(QColor(128, 128, 128, 255))
            self.x_rail.translate(-x_length/2 + x_offset, 0, 0)
            self.view.addItem(self.x_rail)
            self.gl_items['x_rail'] = self.x_rail

            # Y rail - will be positioned in update_visualization
            self.y_rail = gl.GLBoxItem(size=QVector3D(50, y_length, 50))
            self.y_rail.setColor(QColor(150, 150, 150, 255))
            self.view.addItem(self.y_rail)
            self.gl_items['y_rail'] = self.y_rail

            # Z rail - will be positioned in update_visualization
            z_width = 50
            z_depth = 50
            self.z_rail = gl.GLBoxItem(size=QVector3D(z_width, z_depth, z_length))
            self.z_rail.setColor(QColor(100, 100, 255, 255))
            self.view.addItem(self.z_rail)
            self.gl_items['z_rail'] = self.z_rail

            # Carriage - will be positioned in update_visualization
            carriage_size = 80
            carriage_height = 60
            self.z_carriage = gl.GLBoxItem(size=QVector3D(carriage_size, carriage_size, carriage_height))
            self.z_carriage.setColor(QColor(180, 180, 180, 255))
            self.view.addItem(self.z_carriage)
            self.gl_items['z_carriage'] = self.z_carriage

            # T part - will be positioned in update_visualization
            t_size = 180
            t_height = 40
            self.t_part = gl.GLBoxItem(size=QVector3D(t_size, t_size, t_height))
            self.t_part.setColor(QColor(200, 200, 200, 255))
            self.view.addItem(self.t_part)
            self.gl_items['t_part'] = self.t_part

            # Grippers - will be positioned in update_visualization
            gripper_thickness = 30
            gripper_length = 200
            gripper_height = 100

            self.g_left = gl.GLBoxItem(size=QVector3D(gripper_thickness, gripper_length, gripper_height))
            self.g_left.setColor(QColor(220, 220, 220, 255))
            self.view.addItem(self.g_left)
            self.gl_items['g_left'] = self.g_left

            self.g_right = gl.GLBoxItem(size=QVector3D(gripper_thickness, gripper_length, gripper_height))
            self.g_right.setColor(QColor(220, 220, 220, 255))
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
        if self.axis_inverted[axis]:
            min_val = self.axis_ranges[axis]['min']
            max_val = self.axis_ranges[axis]['max']
            inverted_value = max_val - (value - min_val)
            self.positions[axis] = inverted_value
        else:
            self.positions[axis] = value

        self.pos_labels[axis].setText(str(self.positions[axis]))
        self.update_visualization()

    def reset_positions(self):
        for axis in SLAVE_IDS:
            axis_lower = axis.lower()
            self.positions[axis_lower] = 0
            self.sliders[axis_lower].setValue(0)
            self.pos_labels[axis_lower].setText("0")
            self.invert_checkboxes[axis_lower].setChecked(False)
            self.axis_inverted[axis_lower] = False
        self.update_visualization()

    def update_visualization(self):
        if not self.initialized:
            return

        try:
            # Get current position values
            x_pos = self.positions['x']
            y_pos = self.positions['y']
            z_pos = self.positions['z']
            t_pos = self.positions['t']
            g_pos = self.positions['g']

            # Get offset values
            x_offset = self.relative_positions['x_offset']
            y_offset = self.relative_positions['y_offset']
            z_offset = self.relative_positions['z_offset']

            # Get rail length
            z_length = self.rail_lengths['z']
            y_length = self.rail_lengths['y']

            # Component dimensions
            z_width = 50
            z_depth = 50
            carriage_size = 80
            carriage_height = 60
            t_size = 180
            t_height = 40
            gripper_thickness = 30
            gripper_length = 200
            gripper_height = 100

            # Y rail - affected by X position (moves with X)
            self.y_rail.resetTransform()
            self.y_rail.translate(
                -x_pos + x_offset,  # X position of Y rail follows X position
                -y_length/2 + y_offset,  # Y base position with offset
                0
            )

            # Z rail - affected by both X and Y positions (moves with X and Y)
            self.z_rail.resetTransform()
            self.z_rail.translate(
                -x_pos-z_width/2 + x_offset,  # X position follows X position
                -y_pos-z_depth/2 + y_offset,  # Y position follows Y position
                -z_length + z_offset  # Z base position with offset
            )

            # Z carriage - calculate vertical position along Z rail
            z_carriage_pos = -carriage_height + (z_pos / 1000) * (-z_length + carriage_height + 50)
            z_carriage_pos = max(-z_length + carriage_height, min(-carriage_height, z_carriage_pos))

            # Z carriage positioning - affected by X, Y, Z
            self.z_carriage.resetTransform()
            self.z_carriage.translate(
                -x_pos-carriage_size/2 + x_offset,  # X position follows X position
                -y_pos-carriage_size/2 + y_offset,  # Y position follows Y position
                z_carriage_pos + z_offset  # Z position based on Z motor + offset
            )

            # T part (rotational component) - affected by X, Y, Z, and T
            self.t_part.resetTransform()
            # First set rotation
            self.t_part.rotate(t_pos / 10, 0, 0, 1)
            # Then set position
            self.t_part.translate(
                -x_pos-t_size/2 + x_offset,  # X position follows X
                -y_pos-t_size/2 + y_offset,  # Y position follows Y
                z_carriage_pos-t_height + z_offset  # Z position follows Z
            )

            # Gripper spacing - based on G position
            gripper_spacing = max(30, 150 - g_pos / 10)

            # Left and right grippers
            self.g_left.resetTransform()
            self.g_right.resetTransform()

            # Apply rotation from T
            self.g_left.rotate(t_pos / 10, 0, 0, 1)
            self.g_right.rotate(t_pos / 10, 0, 0, 1)

            # Position grippers - affected by X, Y, Z, T, and G
            self.g_left.translate(
                -x_pos-gripper_spacing/2-gripper_thickness + x_offset,  # X position follows X + G spacing
                -y_pos-gripper_length/2 + y_offset,  # Y position follows Y
                z_carriage_pos-t_height-gripper_height + z_offset  # Z position follows Z
            )
            self.g_right.translate(
                -x_pos+gripper_spacing/2 + x_offset,  # X position follows X + G spacing
                -y_pos-gripper_length/2 + y_offset,  # Y position follows Y
                z_carriage_pos-t_height-gripper_height + z_offset  # Z position follows Z
            )

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

            slider_value = position
            if self.axis_inverted[axis_id.lower()]:
                slider_value = max_val - (position - min_val)

            if min_val <= slider_value <= max_val:
                self.sliders[axis_id.lower()].setValue(slider_value)
            else:
                if slider_value < min_val:
                    self.min_inputs[axis_id.lower()].setValue(min_val)
                    self.axis_ranges[axis_id.lower()]['min'] = min_val
                elif slider_value > max_val:
                    self.max_inputs[axis_id.lower()].setValue(max_val)
                    self.axis_ranges[axis_id.lower()]['max'] = max_val

                self.apply_all_ranges()
                self.sliders[axis_id.lower()].setValue(slider_value)

            self.update_visualization()

    def reset_all_positions(self):
        for axis_id in SLAVE_IDS:
            self.update_position(axis_id, 0)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if hasattr(self, 'view') and self.initialized:
            self.view.update()
