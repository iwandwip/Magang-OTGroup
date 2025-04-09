"""
Configuration manager for saving and loading visualization settings.
"""
import os
import yaml
from PyQt5.QtWidgets import QFileDialog, QMessageBox


class ConfigManager:
    """Manages saving and loading of visualization configuration."""

    def __init__(self, parent):
        """Initialize the config manager with reference to the parent panel."""
        self.parent = parent
        self.config_name = "Default Configuration"

    def save_configuration(self):
        """Save the current configuration to a YAML file."""
        config_name, ok = QFileDialog.getSaveFileName(
            self.parent, "Save Configuration", "", "YAML Files (*.yaml *.yml);;All Files (*)"
        )

        if not config_name:
            return

        base_name = os.path.basename(config_name)
        self.config_name = os.path.splitext(base_name)[0]
        self.parent.ui_builder.config_name_label.setText(self.config_name)

        try:
            # Prepare configuration data
            config_data = {
                'name': self.config_name,
                'rail_lengths': self.parent.rail_lengths,
                'relative_positions': self.parent.relative_positions,
                'axis_inverted': self.parent.axis_inverted,
                'axis_ranges': self.parent.axis_ranges,
                'camera': {
                    'distance': self.parent.camera_ctrl.camera_distance,
                    'elevation': self.parent.camera_ctrl.camera_elevation,
                    'azimuth': self.parent.camera_ctrl.camera_azimuth,
                    'pan_x': self.parent.camera_ctrl.camera_pan_x,
                    'pan_y': self.parent.camera_ctrl.camera_pan_y
                },
                'display': {
                    'line_width': self.parent.model_ctrl.line_width
                }
            }

            # Write configuration to file
            with open(config_name, 'w') as file:
                yaml.dump(config_data, file, default_flow_style=False)

            QMessageBox.information(self.parent, "Configuration Saved",
                                    f"Configuration saved successfully to {config_name}")
        except Exception as e:
            QMessageBox.critical(self.parent, "Save Error", f"Error saving configuration: {str(e)}")

    def load_configuration(self):
        """Load configuration from a YAML file."""
        config_name, _ = QFileDialog.getOpenFileName(
            self.parent, "Load Configuration", "", "YAML Files (*.yaml *.yml);;All Files (*)"
        )

        if not config_name:
            return

        try:
            # Read configuration from file
            with open(config_name, 'r') as file:
                config_data = yaml.safe_load(file)

            # Update configuration name
            self.config_name = config_data.get('name', os.path.basename(config_name))
            self.parent.ui_builder.config_name_label.setText(self.config_name)

            # Update rail lengths
            self._update_rail_lengths(config_data)

            # Update relative positions
            self._update_relative_positions(config_data)

            # Update axis inversion settings
            self._update_axis_inversion(config_data)

            # Update axis ranges
            self._update_axis_ranges(config_data)

            # Apply all settings
            self.parent.model_ctrl.apply_rail_lengths()
            self.parent.model_ctrl.apply_relative_positions()
            self.parent.model_ctrl.apply_all_ranges()

            # Update camera settings
            self._update_camera_settings(config_data)

            # Update display settings
            self._update_display_settings(config_data)

            QMessageBox.information(self.parent, "Configuration Loaded",
                                    f"Configuration loaded successfully from {config_name}")
        except Exception as e:
            QMessageBox.critical(self.parent, "Load Error", f"Error loading configuration: {str(e)}")

    def _update_rail_lengths(self, config_data):
        """Update rail lengths from configuration data."""
        if 'rail_lengths' in config_data:
            self.parent.rail_lengths = config_data['rail_lengths']
            for axis, value in self.parent.rail_lengths.items():
                if axis in self.parent.ui_builder.rail_length_inputs:
                    self.parent.ui_builder.rail_length_inputs[axis].setValue(value)

    def _update_relative_positions(self, config_data):
        """Update relative positions from configuration data."""
        if 'relative_positions' in config_data:
            self.parent.relative_positions = config_data['relative_positions']
            for key, value in self.parent.relative_positions.items():
                if key in self.parent.ui_builder.rel_pos_inputs:
                    self.parent.ui_builder.rel_pos_inputs[key].setValue(value)

    def _update_axis_inversion(self, config_data):
        """Update axis inversion settings from configuration data."""
        if 'axis_inverted' in config_data:
            self.parent.axis_inverted = config_data['axis_inverted']
            for axis, value in self.parent.axis_inverted.items():
                if axis in self.parent.ui_builder.invert_checkboxes:
                    self.parent.ui_builder.invert_checkboxes[axis].setChecked(value)

    def _update_axis_ranges(self, config_data):
        """Update axis ranges from configuration data."""
        if 'axis_ranges' in config_data:
            self.parent.axis_ranges = config_data['axis_ranges']
            for axis, range_values in self.parent.axis_ranges.items():
                if axis in self.parent.ui_builder.min_inputs:
                    self.parent.ui_builder.min_inputs[axis].setValue(range_values['min'])
                if axis in self.parent.ui_builder.max_inputs:
                    self.parent.ui_builder.max_inputs[axis].setValue(range_values['max'])

    def _update_camera_settings(self, config_data):
        """Update camera settings from configuration data."""
        if 'camera' in config_data and self.parent.initialized:
            camera = config_data['camera']
            if all(k in camera for k in ['distance', 'elevation', 'azimuth']):
                # Update camera controller values
                self.parent.camera_ctrl.camera_distance = camera['distance']
                self.parent.camera_ctrl.camera_elevation = camera['elevation']
                self.parent.camera_ctrl.camera_azimuth = camera['azimuth']
                self.parent.camera_ctrl.camera_pan_x = camera.get('pan_x', 0)
                self.parent.camera_ctrl.camera_pan_y = camera.get('pan_y', 0)

                # Block signals to prevent recursive callbacks
                ui = self.parent.ui_builder
                ui.distance_slider.blockSignals(True)
                ui.elevation_scrollbar.blockSignals(True)
                ui.azimuth_scrollbar.blockSignals(True)
                ui.pan_slider.blockSignals(True)
                ui.pan_vertical_slider.blockSignals(True)

                # Update UI controls
                ui.distance_slider.setValue(self.parent.camera_ctrl.camera_distance)
                ui.elevation_scrollbar.setValue(self.parent.camera_ctrl.camera_elevation)
                ui.azimuth_scrollbar.setValue(self.parent.camera_ctrl.camera_azimuth)
                ui.pan_slider.setValue(self.parent.camera_ctrl.camera_pan_x)
                ui.pan_vertical_slider.setValue(self.parent.camera_ctrl.camera_pan_y)

                # Re-enable signals
                ui.distance_slider.blockSignals(False)
                ui.elevation_scrollbar.blockSignals(False)
                ui.azimuth_scrollbar.blockSignals(False)
                ui.pan_slider.blockSignals(False)
                ui.pan_vertical_slider.blockSignals(False)

                # Update camera position
                self.parent.camera_ctrl.update_camera_position()

    def _update_display_settings(self, config_data):
        """Update display settings from configuration data."""
        if 'display' in config_data:
            display = config_data['display']
            if 'line_width' in display:
                # Update line width
                self.parent.model_ctrl.line_width = display['line_width']

                # Update UI control
                self.parent.ui_builder.line_width_slider.blockSignals(True)
                self.parent.ui_builder.line_width_slider.setValue(self.parent.model_ctrl.line_width)
                self.parent.ui_builder.line_width_slider.blockSignals(False)

                # Update line widths in visualization
                self.parent.model_ctrl.update_line_widths()
