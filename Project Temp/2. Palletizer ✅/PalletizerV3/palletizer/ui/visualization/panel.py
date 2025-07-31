"""
Main visualization panel class that coordinates all visualization components.
"""
import numpy as np
from PyQt5.QtWidgets import QWidget, QVBoxLayout, QSplitter, QMessageBox, QLabel
from PyQt5.QtCore import Qt, QTimer

from ...utils.config import SLAVE_IDS

from .ui_builder import UIBuilder
from .camera_controls import CameraController
from .model_controls import ModelController
from .config_manager import ConfigManager


class VisualizationPanel(QWidget):
    """Panel for 3D visualization of the palletizer system."""

    def __init__(self, parent=None):
        """Initialize the visualization panel."""
        super().__init__(parent)

        # Initialize state variables
        self.positions = {axis: 0 for axis in SLAVE_IDS}
        self.axis_ranges = {
            axis: {'min': -1000, 'max': 1000} for axis in SLAVE_IDS
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
            axis: False for axis in SLAVE_IDS
        }
        self.axis_inverted['x'] = True  # X-axis is inverted by default

        # GL items dictionary to track 3D objects
        self.gl_items = {}

        # Initialization flag
        self.initialized = False

        # Create controllers
        self.ui_builder = UIBuilder(self)
        self.camera_ctrl = CameraController(self)
        self.model_ctrl = ModelController(self)
        self.config_mgr = ConfigManager(self)

        # Set up the UI
        self.setup_ui()

        # Delay initialization of 3D view to allow UI to render first
        QTimer.singleShot(100, self.initialize_visualization)

    def setup_ui(self):
        """Set up the user interface."""
        self.ui_builder.build_ui()

    def initialize_visualization(self):
        """Initialize the 3D visualization components."""
        try:
            self.model_ctrl.initialize_gl_view()
            self.initialized = True
            self.camera_ctrl.set_view('isometric')
            self.model_ctrl.update_visualization()
        except Exception as e:
            error_label = QLabel(f"3D Visualization Error: {str(e)}\n\nPlease check if PyQtGraph and PyOpenGL are properly installed.")
            error_label.setWordWrap(True)
            error_label.setStyleSheet("color: red; background-color: #ffeeee; padding: 10px; border: 1px solid red;")
            self.ui_builder.view_layout.addWidget(error_label)

    def update_position(self, axis_id, position):
        """Update the position of a specific axis."""
        if axis_id.lower() in self.positions:
            self.positions[axis_id.lower()] = position
            if hasattr(self.ui_builder, 'pos_labels'):
                self.ui_builder.pos_labels[axis_id.lower()].setText(str(position))

            if hasattr(self.ui_builder, 'sliders'):
                min_val = self.ui_builder.sliders[axis_id.lower()].minimum()
                max_val = self.ui_builder.sliders[axis_id.lower()].maximum()

                slider_value = position
                if self.axis_inverted[axis_id.lower()]:
                    slider_value = max_val - (position - min_val)

                if min_val <= slider_value <= max_val:
                    self.ui_builder.sliders[axis_id.lower()].setValue(slider_value)
                else:
                    if slider_value < min_val:
                        self.ui_builder.min_inputs[axis_id.lower()].setValue(min_val)
                        self.axis_ranges[axis_id.lower()]['min'] = min_val
                    elif slider_value > max_val:
                        self.ui_builder.max_inputs[axis_id.lower()].setValue(max_val)
                        self.axis_ranges[axis_id.lower()]['max'] = max_val

                    self.model_ctrl.apply_all_ranges()
                    self.ui_builder.sliders[axis_id.lower()].setValue(slider_value)

            self.model_ctrl.update_visualization()

    def reset_all_positions(self):
        """Reset all axis positions to zero."""
        for axis_id in SLAVE_IDS:
            self.update_position(axis_id, 0)

    def resizeEvent(self, event):
        """Handle window resize events."""
        super().resizeEvent(event)
        if self.initialized and hasattr(self.model_ctrl, 'view'):
            self.model_ctrl.view.update()
