"""
UI Builder for the visualization panel.
"""
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QGroupBox,
                             QPushButton, QLabel, QSlider, QCheckBox, QSplitter, QFrame,
                             QSpinBox, QScrollArea, QSizePolicy, QFileDialog, QScrollBar)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont

from ...utils.config import SLAVE_IDS


class UIBuilder:
    """Builds the user interface for the visualization panel."""

    def __init__(self, parent):
        """Initialize the UI builder with a reference to the parent panel."""
        self.parent = parent
        self.sliders = {}
        self.min_inputs = {}
        self.max_inputs = {}
        self.invert_checkboxes = {}
        self.rail_length_inputs = {}
        self.rel_pos_inputs = {}
        self.pos_labels = {}

    def build_ui(self):
        """Build the complete UI for the visualization panel."""
        main_layout = QVBoxLayout(self.parent)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.setSpacing(5)

        # Create a splitter for the main areas
        splitter = QSplitter(Qt.Horizontal)
        splitter.setChildrenCollapsible(False)

        # Build the visualization area
        vis_widget = self._build_visualization_area()

        # Build the control area
        control_scroll = self._build_control_area()

        # Add widgets to splitter
        splitter.addWidget(vis_widget)
        splitter.addWidget(control_scroll)

        # Set initial size ratio (60%/40%)
        splitter.setSizes([600, 400])

        # Add splitter to main layout
        main_layout.addWidget(splitter)

    def _build_visualization_area(self):
        """Build the visualization area including 3D view and controls."""
        vis_widget = QWidget()
        vis_layout = QVBoxLayout(vis_widget)
        vis_layout.setContentsMargins(0, 0, 0, 0)

        # Container for the 3D view and control widgets
        view_container = QWidget()
        view_container_layout = QVBoxLayout(view_container)
        view_container_layout.setContentsMargins(0, 0, 0, 0)
        view_container_layout.setSpacing(0)

        # 3D View frame
        view_frame = QFrame()
        view_frame.setFrameShape(QFrame.StyledPanel)
        view_frame.setFrameShadow(QFrame.Sunken)
        view_frame.setMinimumSize(400, 300)
        view_frame.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        # Layout for 3D view
        self.view_layout = QVBoxLayout(view_frame)
        self.view_layout.setContentsMargins(0, 0, 0, 0)

        # Placeholder until 3D view is initialized
        self.view_placeholder = QLabel("Initializing 3D View...")
        self.view_placeholder.setAlignment(Qt.AlignCenter)
        self.view_placeholder.setStyleSheet("background-color: #f0f0f0; border: 1px solid #cccccc;")
        self.view_layout.addWidget(self.view_placeholder)

        # Create camera control widgets
        self._create_camera_controls(view_container_layout, view_frame)

        # Information group box
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
            "Y affects Z, T, G. The gripper moves along Z.\n\n"
            "Navigation Controls:\n"
            "- Use the horizontal scrollbar to rotate view left/right (azimuth)\n"
            "- Use the vertical scrollbar to rotate view up/down (elevation)\n"
            "- Use the distance slider to zoom in/out\n"
            "- Use the horizontal pan slider to move camera left/right\n"
            "- Use the vertical pan slider to move camera up/down\n"
            "- Use the line width slider to adjust the thickness of axis lines"
        )
        info_text.setWordWrap(True)
        info_layout.addWidget(info_text)

        # Add components to main visualization layout
        vis_layout.addWidget(view_container, 1)
        vis_layout.addWidget(info_group, 0)

        return vis_widget

    def _create_camera_controls(self, parent_layout, view_frame):
        """Create all camera control widgets (scrollbars, sliders, etc.)."""
        # Create azimuth (horizontal rotation) scrollbar
        self.azimuth_scrollbar = self._create_azimuth_scrollbar()
        azimuth_layout = QHBoxLayout()
        azimuth_layout.addWidget(QLabel("Azimuth (Horizontal Rotation):"))
        azimuth_layout.addWidget(self.azimuth_scrollbar)

        # Create elevation (vertical rotation) scrollbar
        self.elevation_scrollbar = self._create_elevation_scrollbar()
        elev_label = QLabel("Elevation\n(Vertical\nRotation)")
        elev_label.setAlignment(Qt.AlignCenter)
        elev_label.setFixedWidth(70)

        # Distance slider for zoom
        self.distance_slider = self._create_distance_slider()
        distance_layout = QVBoxLayout()
        distance_layout.addWidget(QLabel("Distance (Zoom):"))
        distance_layout.addWidget(self.distance_slider)

        # Horizontal pan slider
        self.pan_slider = self._create_pan_slider()
        pan_layout = QVBoxLayout()
        pan_layout.addWidget(QLabel("Pan Camera (Left/Right):"))
        pan_layout.addWidget(self.pan_slider)

        # Vertical pan slider
        self.pan_vertical_slider = self._create_vertical_pan_slider()
        pan_vertical_layout = QVBoxLayout()
        pan_vertical_layout.addWidget(QLabel("Vertical Pan (Up/Down):"))
        pan_vertical_layout.addWidget(self.pan_vertical_slider)

        # Line width slider
        self.line_width_slider = self._create_line_width_slider()
        line_width_layout = QVBoxLayout()
        line_width_layout.addWidget(QLabel("Line Thickness:"))
        line_width_layout.addWidget(self.line_width_slider)

        # Layout for view frame and elevation scrollbar
        view_and_controls = QHBoxLayout()

        # Add elevation controls on the left
        elev_controls = QVBoxLayout()
        elev_controls.addWidget(elev_label)
        elev_controls.addWidget(self.elevation_scrollbar)
        view_and_controls.addLayout(elev_controls)

        # Add view frame in the middle
        view_and_controls.addWidget(view_frame, 1)

        # Add all layouts to parent container
        parent_layout.addLayout(view_and_controls)
        parent_layout.addLayout(azimuth_layout)
        parent_layout.addLayout(distance_layout)
        parent_layout.addLayout(pan_layout)
        parent_layout.addLayout(pan_vertical_layout)
        parent_layout.addLayout(line_width_layout)

    def _create_azimuth_scrollbar(self):
        """Create and configure the azimuth scrollbar."""
        scrollbar = self._create_horizontal_scrollbar(0, 360, 45, 15)
        scrollbar.valueChanged.connect(self.parent.camera_ctrl.on_azimuth_changed)
        return scrollbar

    def _create_elevation_scrollbar(self):
        """Create and configure the elevation scrollbar."""
        scrollbar = self._create_vertical_scrollbar(-90, 90, 30, 10)
        scrollbar.valueChanged.connect(self.parent.camera_ctrl.on_elevation_changed)
        return scrollbar

    def _create_distance_slider(self):
        """Create and configure the distance (zoom) slider."""
        slider = self._create_horizontal_slider(500, 5000, 2000, 100)
        slider.valueChanged.connect(self.parent.camera_ctrl.on_distance_changed)
        return slider

    def _create_pan_slider(self):
        """Create and configure the horizontal pan slider."""
        slider = self._create_horizontal_slider(-500, 500, 0, 20)
        slider.valueChanged.connect(self.parent.camera_ctrl.on_pan_changed)
        return slider

    def _create_vertical_pan_slider(self):
        """Create and configure the vertical pan slider."""
        slider = self._create_horizontal_slider(-500, 500, 0, 20)
        slider.valueChanged.connect(self.parent.camera_ctrl.on_pan_vertical_changed)
        return slider

    def _create_line_width_slider(self):
        """Create and configure the line width slider."""
        slider = self._create_horizontal_slider(1, 10, self.parent.model_ctrl.line_width, 1)
        slider.valueChanged.connect(self.parent.model_ctrl.on_line_width_changed)
        return slider

    def _create_horizontal_scrollbar(self, min_val, max_val, value, page_step):
        """Helper method to create a horizontal scrollbar with common settings."""
        scrollbar = self._create_scrollbar(Qt.Horizontal, min_val, max_val, value, page_step)
        return scrollbar

    def _create_vertical_scrollbar(self, min_val, max_val, value, page_step):
        """Helper method to create a vertical scrollbar with common settings."""
        scrollbar = self._create_scrollbar(Qt.Vertical, min_val, max_val, value, page_step)
        return scrollbar

    def _create_scrollbar(self, orientation, min_val, max_val, value, page_step):
        """Create a scrollbar with the given parameters."""
        scrollbar = QScrollBar(orientation)
        scrollbar.setRange(min_val, max_val)
        scrollbar.setValue(value)
        scrollbar.setPageStep(page_step)
        return scrollbar

    def _create_horizontal_slider(self, min_val, max_val, value, page_step):
        """Helper method to create a horizontal slider with common settings."""
        slider = QSlider(Qt.Horizontal)
        slider.setRange(min_val, max_val)
        slider.setValue(value)
        slider.setPageStep(page_step)
        return slider

    def _build_control_area(self):
        """Build the control panel area with settings."""
        control_scroll = QScrollArea()
        control_scroll.setWidgetResizable(True)
        control_scroll.setFrameShape(QFrame.NoFrame)
        control_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

        control_widget = QWidget()
        control_layout = QVBoxLayout(control_widget)
        control_layout.setContentsMargins(5, 5, 5, 5)
        control_layout.setSpacing(5)

        # Add configuration management section
        config_group = self._create_config_group()
        control_layout.addWidget(config_group)

        # Add position display section
        position_group = self._create_position_group()
        control_layout.addWidget(position_group)

        # Add rail length settings section
        rail_length_group = self._create_rail_length_group()
        control_layout.addWidget(rail_length_group)

        # Add axis base offsets section
        rel_pos_group = self._create_rel_pos_group()
        control_layout.addWidget(rel_pos_group)

        # Add axis range settings section
        range_group = self._create_range_group()
        control_layout.addWidget(range_group)

        # Add axis movement section
        movement_group = self._create_movement_group()
        control_layout.addWidget(movement_group)

        # Add view controls section
        view_group = self._create_view_controls_group()
        control_layout.addWidget(view_group)

        # Add stretch to push everything to the top
        control_layout.addStretch()

        # Set the control widget as the scroll area's widget
        control_scroll.setWidget(control_widget)

        return control_scroll

    def _create_config_group(self):
        """Create the configuration management group."""
        config_group = QGroupBox("Configuration Management")
        config_layout = QVBoxLayout()
        config_layout.setSpacing(5)

        # Current configuration name display
        name_layout = QHBoxLayout()
        name_layout.addWidget(QLabel("Current Config:"))
        self.config_name_label = QLabel(self.parent.config_mgr.config_name)
        self.config_name_label.setStyleSheet("font-weight: bold;")
        name_layout.addWidget(self.config_name_label)
        name_layout.addStretch()
        config_layout.addLayout(name_layout)

        # Configuration buttons
        buttons_layout = QHBoxLayout()
        buttons_layout.setSpacing(5)

        # Save configuration button
        self.save_config_btn = QPushButton("Save Configuration")
        self.save_config_btn.clicked.connect(self.parent.config_mgr.save_configuration)
        self.save_config_btn.setStyleSheet("background-color: #ccffcc;")

        # Load configuration button
        self.load_config_btn = QPushButton("Load Configuration")
        self.load_config_btn.clicked.connect(self.parent.config_mgr.load_configuration)
        self.load_config_btn.setStyleSheet("background-color: #e0e0ff;")

        buttons_layout.addWidget(self.save_config_btn)
        buttons_layout.addWidget(self.load_config_btn)
        config_layout.addLayout(buttons_layout)

        config_group.setLayout(config_layout)
        return config_group

    def _create_position_group(self):
        """Create the current positions display group."""
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

        return position_group

    def _create_rail_length_group(self):
        """Create the rail length settings group."""
        rail_length_group = QGroupBox("Rail Length Settings")
        rail_length_layout = QGridLayout(rail_length_group)
        rail_length_layout.setVerticalSpacing(5)

        self.rail_length_inputs = {}

        row = 0
        for axis in ['x', 'y', 'z']:
            rail_length_layout.addWidget(QLabel(f"{axis.upper()}-Rail Length:"), row, 0)

            length_input = QSpinBox()
            length_input.setRange(100, 3000)
            length_input.setValue(self.parent.rail_lengths[axis])
            length_input.setSingleStep(100)
            length_input.valueChanged.connect(lambda v, a=axis: self.parent.model_ctrl.on_rail_length_changed(a, v))
            self.rail_length_inputs[axis] = length_input
            rail_length_layout.addWidget(length_input, row, 1)

            row += 1

        # Apply and reset buttons
        apply_rail_btn = QPushButton("Apply Rail Lengths")
        apply_rail_btn.clicked.connect(self.parent.model_ctrl.apply_rail_lengths)
        apply_rail_btn.setStyleSheet("background-color: #ccffcc;")
        rail_length_layout.addWidget(apply_rail_btn, row, 0, 1, 2)

        row += 1

        reset_rail_btn = QPushButton("Reset to Default Lengths")
        reset_rail_btn.clicked.connect(self.parent.model_ctrl.reset_rail_lengths)
        reset_rail_btn.setStyleSheet("background-color: #ffcccc;")
        rail_length_layout.addWidget(reset_rail_btn, row, 0, 1, 2)

        return rail_length_group

    def _create_rel_pos_group(self):
        """Create the axis base offsets group."""
        rel_pos_group = QGroupBox("Axis Base Offsets")
        rel_pos_layout = QGridLayout(rel_pos_group)
        rel_pos_layout.setVerticalSpacing(5)

        self.rel_pos_inputs = {}

        info_label = QLabel("These offsets set the base positions of each axis. The mechanical dependencies (X→Y→Z) are preserved.")
        info_label.setWordWrap(True)
        info_label.setStyleSheet("font-style: italic; color: #666666;")
        rel_pos_layout.addWidget(info_label, 0, 0, 1, 2)

        row = 1
        for axis in ['x', 'y', 'z']:
            rel_pos_layout.addWidget(QLabel(f"{axis.upper()}-Axis Base Offset:"), row, 0)
            offset_input = QSpinBox()
            offset_input.setRange(-1000, 1000)
            offset_input.setValue(self.parent.relative_positions[f'{axis}_offset'])
            offset_input.setSingleStep(10)
            offset_input.valueChanged.connect(lambda v, key=f'{axis}_offset': self.parent.model_ctrl.on_rel_pos_changed(key, v))
            self.rel_pos_inputs[f'{axis}_offset'] = offset_input
            rel_pos_layout.addWidget(offset_input, row, 1)
            row += 1

        # Apply and reset buttons
        apply_pos_btn = QPushButton("Apply Base Offsets")
        apply_pos_btn.clicked.connect(self.parent.model_ctrl.apply_relative_positions)
        apply_pos_btn.setStyleSheet("background-color: #ccffcc;")
        rel_pos_layout.addWidget(apply_pos_btn, row, 0, 1, 2)

        row += 1

        reset_pos_btn = QPushButton("Reset Base Offsets")
        reset_pos_btn.clicked.connect(self.parent.model_ctrl.reset_relative_positions)
        reset_pos_btn.setStyleSheet("background-color: #ffcccc;")
        rel_pos_layout.addWidget(reset_pos_btn, row, 0, 1, 2)

        return rel_pos_group

    def _create_range_group(self):
        """Create the axis range settings group."""
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
            min_input.setValue(self.parent.axis_ranges[axis.lower()]['min'])
            min_input.setSingleStep(100)
            min_input.valueChanged.connect(lambda v, a=axis.lower(): self.parent.model_ctrl.on_min_changed(a, v))
            self.min_inputs[axis.lower()] = min_input
            range_layout.addWidget(min_input, row, 1, 1, 2)
            row += 1

            range_layout.addWidget(QLabel("Max:"), row, 0)
            max_input = QSpinBox()
            max_input.setRange(-100000, 100000)
            max_input.setValue(self.parent.axis_ranges[axis.lower()]['max'])
            max_input.setSingleStep(100)
            max_input.valueChanged.connect(lambda v, a=axis.lower(): self.parent.model_ctrl.on_max_changed(a, v))
            self.max_inputs[axis.lower()] = max_input
            range_layout.addWidget(max_input, row, 1, 1, 2)
            row += 1

            if axis != SLAVE_IDS[-1]:
                spacer = QLabel("")
                spacer.setFixedHeight(5)
                range_layout.addWidget(spacer, row, 0, 1, 3)
                row += 1

        # Apply and reset buttons
        button_layout = QHBoxLayout()

        apply_btn = QPushButton("Apply All Range Settings")
        apply_btn.clicked.connect(self.parent.model_ctrl.apply_all_ranges)
        apply_btn.setStyleSheet("background-color: #e0e0ff;")

        reset_ranges_btn = QPushButton("Reset to Default Ranges")
        reset_ranges_btn.clicked.connect(self.parent.model_ctrl.reset_ranges)
        reset_ranges_btn.setStyleSheet("background-color: #ffe0e0;")

        button_layout.addWidget(apply_btn)
        button_layout.addWidget(reset_ranges_btn)

        range_layout.addLayout(button_layout, row, 0, 1, 3)
        return range_group

    def _create_movement_group(self):
        """Create the axis movement control group."""
        movement_group = QGroupBox("Axis Movement")
        movement_layout = QVBoxLayout(movement_group)
        movement_layout.setSpacing(5)

        self.sliders = {}
        self.invert_checkboxes = {}

        for axis in SLAVE_IDS:
            axis_layout = QHBoxLayout()

            axis_layout.addWidget(QLabel(f"{axis}:"))

            slider = QSlider(Qt.Horizontal)
            slider.setMinimum(self.parent.axis_ranges[axis.lower()]['min'])
            slider.setMaximum(self.parent.axis_ranges[axis.lower()]['max'])
            slider.setValue(0)
            slider.setTickPosition(QSlider.TicksBelow)
            slider.setTickInterval(100)
            slider.valueChanged.connect(lambda v, a=axis.lower(): self.parent.model_ctrl.on_slider_changed(a, v))
            self.sliders[axis.lower()] = slider

            invert_cb = QCheckBox("Invert")
            invert_cb.setChecked(self.parent.axis_inverted[axis.lower()])
            invert_cb.stateChanged.connect(lambda state, a=axis.lower(): self.parent.model_ctrl.on_invert_changed(a, state))
            self.invert_checkboxes[axis.lower()] = invert_cb

            axis_layout.addWidget(slider)
            axis_layout.addWidget(invert_cb)

            movement_layout.addLayout(axis_layout)

        reset_btn = QPushButton("Reset All Positions")
        reset_btn.clicked.connect(self.parent.model_ctrl.reset_positions)
        reset_btn.setStyleSheet("background-color: #ffffcc;")
        movement_layout.addWidget(reset_btn)

        return movement_group

    def _create_view_controls_group(self):
        """Create the view controls group."""
        view_group = QGroupBox("View Controls")
        view_layout = QVBoxLayout(view_group)
        view_layout.setSpacing(5)

        self.top_view_cb = QCheckBox("Top View")
        self.top_view_cb.clicked.connect(lambda: self.parent.camera_ctrl.set_view('top'))

        self.side_view_cb = QCheckBox("Side View")
        self.side_view_cb.clicked.connect(lambda: self.parent.camera_ctrl.set_view('side'))

        self.front_view_cb = QCheckBox("Front View")
        self.front_view_cb.clicked.connect(lambda: self.parent.camera_ctrl.set_view('front'))

        self.isometric_view_cb = QCheckBox("Isometric View")
        self.isometric_view_cb.setChecked(True)
        self.isometric_view_cb.clicked.connect(lambda: self.parent.camera_ctrl.set_view('isometric'))

        view_layout.addWidget(self.top_view_cb)
        view_layout.addWidget(self.side_view_cb)
        view_layout.addWidget(self.front_view_cb)
        view_layout.addWidget(self.isometric_view_cb)

        return view_group
