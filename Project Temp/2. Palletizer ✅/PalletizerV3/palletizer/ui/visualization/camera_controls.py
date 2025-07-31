"""
Camera controller for 3D visualization.
"""
import numpy as np
from PyQt5.QtGui import QVector3D


class CameraController:
    """Controls camera movement and positioning in the 3D view."""

    def __init__(self, parent):
        """Initialize the camera controller with reference to the parent panel."""
        self.parent = parent
        self.camera_distance = 2000
        self.camera_elevation = 30
        self.camera_azimuth = 45
        self.camera_pan_x = 0
        self.camera_pan_y = 0
        self.camera_position = [0, 0, 0]

    def on_azimuth_changed(self, value):
        """Handle changes to the azimuth (horizontal rotation) value."""
        if not self.parent.initialized:
            return

        self.camera_azimuth = value
        self.update_camera_position()

    def on_elevation_changed(self, value):
        """Handle changes to the elevation (vertical rotation) value."""
        if not self.parent.initialized:
            return

        self.camera_elevation = value
        self.update_camera_position()

    def on_distance_changed(self, value):
        """Handle changes to the camera distance (zoom) value."""
        if not self.parent.initialized:
            return

        self.camera_distance = value
        self.update_camera_position()

    def reset_camera_pan(self):
        """Reset camera panning to center position."""
        self.camera_pan_x = 0
        self.camera_pan_y = 0
        self.camera_position = [0, 0, 0]

        # Block signals to prevent recursive callbacks
        self.parent.ui_builder.pan_slider.blockSignals(True)
        self.parent.ui_builder.pan_vertical_slider.blockSignals(True)

        self.parent.ui_builder.pan_slider.setValue(0)
        self.parent.ui_builder.pan_vertical_slider.setValue(0)

        self.parent.ui_builder.pan_slider.blockSignals(False)
        self.parent.ui_builder.pan_vertical_slider.blockSignals(False)

    def on_pan_changed(self, value):
        """Handle changes to the horizontal panning value."""
        if not self.parent.initialized:
            return

        self.camera_pan_x = value
        self.update_camera_position()

    def on_pan_vertical_changed(self, value):
        """Handle changes to the vertical panning value."""
        if not self.parent.initialized:
            return

        self.camera_pan_y = value
        self.update_camera_position()

    def update_camera_position(self):
        """Update the camera position based on current settings."""
        if not self.parent.initialized or not hasattr(self.parent.model_ctrl, 'view'):
            return

        try:
            # Convert azimuth to radians for trigonometric calculations
            rad_azimuth = np.radians(self.camera_azimuth)

            # Calculate camera offset based on pan values and azimuth
            dx = self.camera_pan_x * np.cos(rad_azimuth) + self.camera_pan_y * np.sin(rad_azimuth)
            dy = -self.camera_pan_x * np.sin(rad_azimuth) + self.camera_pan_y * np.cos(rad_azimuth)

            # Apply scaling factor for pan sensitivity
            scale_factor = 0.5

            # Update camera target position
            self.camera_position = [dx * scale_factor, dy * scale_factor, 0]

            # Update camera in the 3D view
            self.parent.model_ctrl.view.setCameraPosition(
                pos=QVector3D(self.camera_position[0], self.camera_position[1], self.camera_position[2]),
                distance=self.camera_distance,
                elevation=self.camera_elevation,
                azimuth=self.camera_azimuth
            )

            # Force update to ensure changes are applied
            self.parent.model_ctrl.view.update()
        except Exception as e:
            print(f"Camera update error: {str(e)}")

    def set_view(self, view_type):
        """Set the camera to a predefined view orientation."""
        if not self.parent.initialized or not hasattr(self.parent.model_ctrl, 'view'):
            return

        # Uncheck all view checkboxes
        self.parent.ui_builder.top_view_cb.setChecked(False)
        self.parent.ui_builder.side_view_cb.setChecked(False)
        self.parent.ui_builder.front_view_cb.setChecked(False)
        self.parent.ui_builder.isometric_view_cb.setChecked(False)

        # Set camera parameters based on view type
        if view_type == 'top':
            self.parent.ui_builder.top_view_cb.setChecked(True)
            self.camera_distance = 2000
            self.camera_elevation = 90
            self.camera_azimuth = 0
            self.reset_camera_pan()
        elif view_type == 'side':
            self.parent.ui_builder.side_view_cb.setChecked(True)
            self.camera_distance = 2000
            self.camera_elevation = 0
            self.camera_azimuth = 0
            self.reset_camera_pan()
        elif view_type == 'front':
            self.parent.ui_builder.front_view_cb.setChecked(True)
            self.camera_distance = 2000
            self.camera_elevation = 0
            self.camera_azimuth = 90
            self.reset_camera_pan()
        elif view_type == 'isometric':
            self.parent.ui_builder.isometric_view_cb.setChecked(True)
            self.camera_distance = 2000
            self.camera_elevation = 30
            self.camera_azimuth = 45
            self.reset_camera_pan()

        # Block signals while updating sliders to avoid recursive callbacks
        self.parent.ui_builder.distance_slider.blockSignals(True)
        self.parent.ui_builder.elevation_scrollbar.blockSignals(True)
        self.parent.ui_builder.azimuth_scrollbar.blockSignals(True)

        # Update slider values to match camera settings
        self.parent.ui_builder.distance_slider.setValue(self.camera_distance)
        self.parent.ui_builder.elevation_scrollbar.setValue(self.camera_elevation)
        self.parent.ui_builder.azimuth_scrollbar.setValue(self.camera_azimuth)

        # Re-enable signals
        self.parent.ui_builder.distance_slider.blockSignals(False)
        self.parent.ui_builder.elevation_scrollbar.blockSignals(False)
        self.parent.ui_builder.azimuth_scrollbar.blockSignals(False)

        # Update camera position
        self.update_camera_position()
