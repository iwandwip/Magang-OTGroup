"""
Model controller for 3D visualization, manages 3D objects and their properties.
"""
import numpy as np
from PyQt5.QtWidgets import QLabel
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QVector3D, QColor
import pyqtgraph.opengl as gl

from ...utils.config import SLAVE_IDS


class ModelController:
    """Controls the 3D model visualization and manipulation."""

    def __init__(self, parent):
        """Initialize the model controller with reference to the parent panel."""
        self.parent = parent
        self.line_width = 3
        self.view = None

    def initialize_gl_view(self):
        """Initialize the OpenGL view and create 3D objects."""
        # Clear any existing widgets from the view layout
        for i in reversed(range(self.parent.ui_builder.view_layout.count())):
            item = self.parent.ui_builder.view_layout.itemAt(i)
            if item.widget():
                item.widget().setParent(None)

        # Create a new GLViewWidget
        self.view = gl.GLViewWidget()

        # Set initial camera position
        self.view.setCameraPosition(
            distance=self.parent.camera_ctrl.camera_distance,
            elevation=self.parent.camera_ctrl.camera_elevation,
            azimuth=self.parent.camera_ctrl.camera_azimuth
        )

        # Set white background
        self.view.setBackgroundColor(255, 255, 255, 255)

        # Add the view to the layout
        self.parent.ui_builder.view_layout.addWidget(self.view)

        # Add a grid to the view
        self._create_grid()

        # Create axis lines and labels
        self._create_axes()

        # Create rails, carriage and other components
        self._create_mechanical_components()

    def _create_grid(self):
        """Create a grid for the 3D view."""
        grid = gl.GLGridItem()
        grid.setSize(x=2000, y=2000)
        grid.setSpacing(x=100, y=100)
        self.view.addItem(grid)
        self.parent.gl_items['grid'] = grid

    def _create_axes(self):
        """Create the X, Y, Z axes visualizations."""
        x_length = self.parent.rail_lengths['x']
        y_length = self.parent.rail_lengths['y']
        z_length = self.parent.rail_lengths['z']

        # X-axis line (red)
        x_axis_line = np.array([[0, 0, 0], [-x_length/2, 0, 0]])
        x_axis = gl.GLLinePlotItem(pos=x_axis_line, color=(1, 0, 0, 1), width=self.line_width)
        self.view.addItem(x_axis)
        self.parent.gl_items['x_axis'] = x_axis

        # Y-axis line (green)
        y_axis_line = np.array([[0, 0, 0], [0, -y_length, 0]])
        y_axis = gl.GLLinePlotItem(pos=y_axis_line, color=(0, 1, 0, 1), width=self.line_width)
        self.view.addItem(y_axis)
        self.parent.gl_items['y_axis'] = y_axis

        # Z-axis line (blue)
        z_axis_line = np.array([[0, 0, 0], [0, 0, z_length/2]])
        z_axis = gl.GLLinePlotItem(pos=z_axis_line, color=(0, 0, 1, 1), width=self.line_width)
        self.view.addItem(z_axis)
        self.parent.gl_items['z_axis'] = z_axis

        # Add axis labels
        self._create_axis_labels(x_length, y_length, z_length)

    def _create_axis_labels(self, x_length, y_length, z_length):
        """Create text labels for the axes."""
        x_label = gl.GLTextItem(pos=np.array([-x_length/2 - 50, 0, 0]), text='X-AXIS', color=(1, 0, 0, 1))
        y_label = gl.GLTextItem(pos=np.array([0, -y_length - 20, 0]), text='Y-AXIS', color=(0, 1, 0, 1))
        z_label = gl.GLTextItem(pos=np.array([0, 0, z_length/2 + 50]), text='Z-AXIS', color=(0, 0, 1, 1))

        self.view.addItem(x_label)
        self.view.addItem(y_label)
        self.view.addItem(z_label)

        self.parent.gl_items['x_label'] = x_label
        self.parent.gl_items['y_label'] = y_label
        self.parent.gl_items['z_label'] = z_label

    def _create_mechanical_components(self):
        """Create the palletizer mechanical components (rails, carriage, etc.)."""
        x_length = self.parent.rail_lengths['x']
        y_length = self.parent.rail_lengths['y']
        z_length = self.parent.rail_lengths['z']

        x_offset = self.parent.relative_positions['x_offset']
        y_offset = self.parent.relative_positions['y_offset']
        z_offset = self.parent.relative_positions['z_offset']

        # Create X-rail (gray box representing the X-axis rail)
        x_rail = gl.GLBoxItem(size=QVector3D(x_length, 50, 50))
        x_rail.setColor(QColor(128, 128, 128, 255))
        x_rail.translate(x_length/2 + x_offset, 0, 0)
        self.view.addItem(x_rail)
        self.parent.gl_items['x_rail'] = x_rail

        # Create Y-rail (dark gray box representing the Y-axis rail)
        y_rail = gl.GLBoxItem(size=QVector3D(50, y_length, 50))
        y_rail.setColor(QColor(150, 150, 150, 255))
        self.view.addItem(y_rail)
        self.parent.gl_items['y_rail'] = y_rail

        # Create Z-rail (blue box representing the Z-axis rail)
        z_width = 50
        z_depth = 50
        z_rail = gl.GLBoxItem(size=QVector3D(z_width, z_depth, z_length))
        z_rail.setColor(QColor(100, 100, 255, 255))
        self.view.addItem(z_rail)
        self.parent.gl_items['z_rail'] = z_rail

        # Create carriage (gray box that moves along Z-axis)
        carriage_size = 80
        carriage_height = 60
        z_carriage = gl.GLBoxItem(size=QVector3D(carriage_size, carriage_size, carriage_height))
        z_carriage.setColor(QColor(180, 180, 180, 255))
        self.view.addItem(z_carriage)
        self.parent.gl_items['z_carriage'] = z_carriage

        # Create T component (gray box that rotates)
        t_size = 180
        t_height = 40
        t_part = gl.GLBoxItem(size=QVector3D(t_size, t_size, t_height))
        t_part.setColor(QColor(200, 200, 200, 255))
        self.view.addItem(t_part)
        self.parent.gl_items['t_part'] = t_part

        # Create gripper components (light gray boxes that open/close)
        gripper_thickness = 30
        gripper_length = 200
        gripper_height = 100

        g_left = gl.GLBoxItem(size=QVector3D(gripper_thickness, gripper_length, gripper_height))
        g_left.setColor(QColor(220, 220, 220, 255))
        self.view.addItem(g_left)
        self.parent.gl_items['g_left'] = g_left

        g_right = gl.GLBoxItem(size=QVector3D(gripper_thickness, gripper_length, gripper_height))
        g_right.setColor(QColor(220, 220, 220, 255))
        self.view.addItem(g_right)
        self.parent.gl_items['g_right'] = g_right

    def on_line_width_changed(self, value):
        """Handle changes to the line width value."""
        if not self.parent.initialized:
            return

        self.line_width = value
        self.update_line_widths()

    def update_line_widths(self):
        """Update the width of all axis lines."""
        if not self.parent.initialized or not hasattr(self, 'view'):
            return

        for axis in ['x', 'y', 'z']:
            axis_key = f'{axis}_axis'
            if axis_key in self.parent.gl_items:
                # Get axis parameters
                if axis == 'x':
                    x_length = self.parent.rail_lengths['x']
                    axis_line = np.array([[0, 0, 0], [-x_length/2, 0, 0]])
                    color = (1, 0, 0, 1)
                elif axis == 'y':
                    y_length = self.parent.rail_lengths['y']
                    axis_line = np.array([[0, 0, 0], [0, -y_length, 0]])
                    color = (0, 1, 0, 1)
                else:  # axis == 'z'
                    z_length = self.parent.rail_lengths['z']
                    axis_line = np.array([[0, 0, 0], [0, 0, z_length/2]])
                    color = (0, 0, 1, 1)

                # Remove old line
                self.view.removeItem(self.parent.gl_items[axis_key])

                # Create new line with updated width
                new_line = gl.GLLinePlotItem(pos=axis_line, color=color, width=self.line_width)
                self.view.addItem(new_line)
                self.parent.gl_items[axis_key] = new_line

        # Force redraw
        self.view.update()

    def on_slider_changed(self, axis, value):
        """Handle changes to axis position sliders."""
        if self.parent.axis_inverted[axis]:
            min_val = self.parent.axis_ranges[axis]['min']
            max_val = self.parent.axis_ranges[axis]['max']
            inverted_value = max_val - (value - min_val)
            self.parent.positions[axis] = inverted_value
        else:
            self.parent.positions[axis] = value

        # Update position label
        self.parent.ui_builder.pos_labels[axis].setText(str(self.parent.positions[axis]))

        # Update 3D visualization
        self.update_visualization()

    def on_invert_changed(self, axis, state):
        """Handle changes to axis inversion checkboxes."""
        self.parent.axis_inverted[axis] = (state == Qt.Checked)
        self.update_visualization()

    def on_rail_length_changed(self, axis, value):
        """Handle changes to rail length inputs."""
        self.parent.rail_lengths[axis] = value

    def on_rel_pos_changed(self, position_key, value):
        """Handle changes to axis base offset inputs."""
        self.parent.relative_positions[position_key] = value

    def on_min_changed(self, axis, value):
        """Handle changes to axis minimum range inputs."""
        if value >= self.parent.ui_builder.max_inputs[axis].value():
            self.parent.ui_builder.min_inputs[axis].setValue(self.parent.ui_builder.max_inputs[axis].value() - 1)
            return
        self.parent.axis_ranges[axis]['min'] = value

    def on_max_changed(self, axis, value):
        """Handle changes to axis maximum range inputs."""
        if value <= self.parent.ui_builder.min_inputs[axis].value():
            self.parent.ui_builder.max_inputs[axis].setValue(self.parent.ui_builder.min_inputs[axis].value() + 1)
            return
        self.parent.axis_ranges[axis]['max'] = value

    def reset_positions(self):
        """Reset all axis positions to zero."""
        for axis in SLAVE_IDS:
            axis_lower = axis.lower()
            self.parent.positions[axis_lower] = 0
            self.parent.ui_builder.sliders[axis_lower].setValue(0)
            self.parent.ui_builder.pos_labels[axis_lower].setText("0")
            self.parent.ui_builder.invert_checkboxes[axis_lower].setChecked(False)
            self.parent.axis_inverted[axis_lower] = False

            # X-axis is inverted by default
            if axis_lower == 'x':
                self.parent.axis_inverted[axis_lower] = True
                self.parent.ui_builder.invert_checkboxes[axis_lower].setChecked(True)

        self.update_visualization()

    def apply_rail_lengths(self):
        """Apply the current rail length settings to the 3D model."""
        if not self.parent.initialized or not hasattr(self, 'view'):
            return

        try:
            x_length = self.parent.rail_lengths['x']
            y_length = self.parent.rail_lengths['y']
            z_length = self.parent.rail_lengths['z']

            # Update X-rail
            self._update_x_rail(x_length)

            # Update Y-rail
            self._update_y_rail(y_length)

            # Update Z-rail
            self._update_z_rail(z_length)

            # Update visualization to reflect changes
            self.update_visualization()

            # Force redraw
            self.view.update()
        except Exception as e:
            print(f"Error applying rail lengths: {str(e)}")

    def _update_x_rail(self, x_length):
        """Update the X-rail with a new length."""
        if 'x_rail' in self.parent.gl_items:
            self.view.removeItem(self.parent.gl_items['x_rail'])
            x_rail = gl.GLBoxItem(size=QVector3D(x_length, 50, 50))
            x_rail.setColor(QColor(128, 128, 128, 255))
            x_rail.translate(x_length/2 + self.parent.relative_positions['x_offset'], 0, 0)
            self.view.addItem(x_rail)
            self.parent.gl_items['x_rail'] = x_rail

        if 'x_axis' in self.parent.gl_items:
            self.view.removeItem(self.parent.gl_items['x_axis'])
            x_axis_line = np.array([[0, 0, 0], [-x_length/2, 0, 0]])
            x_axis = gl.GLLinePlotItem(pos=x_axis_line, color=(1, 0, 0, 1), width=self.line_width)
            self.view.addItem(x_axis)
            self.parent.gl_items['x_axis'] = x_axis

            if 'x_label' in self.parent.gl_items:
                self.view.removeItem(self.parent.gl_items['x_label'])
                x_label = gl.GLTextItem(pos=np.array([-x_length/2 - 50, 0, 0]), text='X-AXIS', color=(1, 0, 0, 1))
                self.view.addItem(x_label)
                self.parent.gl_items['x_label'] = x_label

    def _update_y_rail(self, y_length):
        """Update the Y-rail with a new length."""
        if 'y_rail' in self.parent.gl_items:
            self.view.removeItem(self.parent.gl_items['y_rail'])
            y_rail = gl.GLBoxItem(size=QVector3D(50, y_length, 50))
            y_rail.setColor(QColor(150, 150, 150, 255))
            self.view.addItem(y_rail)
            self.parent.gl_items['y_rail'] = y_rail

        if 'y_axis' in self.parent.gl_items:
            self.view.removeItem(self.parent.gl_items['y_axis'])
            y_axis_line = np.array([[0, 0, 0], [0, -y_length, 0]])
            y_axis = gl.GLLinePlotItem(pos=y_axis_line, color=(0, 1, 0, 1), width=self.line_width)
            self.view.addItem(y_axis)
            self.parent.gl_items['y_axis'] = y_axis

            if 'y_label' in self.parent.gl_items:
                self.view.removeItem(self.parent.gl_items['y_label'])
                y_label = gl.GLTextItem(pos=np.array([0, -y_length - 20, 0]), text='Y-AXIS', color=(0, 1, 0, 1))
                self.view.addItem(y_label)
                self.parent.gl_items['y_label'] = y_label

    def _update_z_rail(self, z_length):
        """Update the Z-rail with a new length."""
        if 'z_rail' in self.parent.gl_items:
            self.view.removeItem(self.parent.gl_items['z_rail'])
            z_width = 50
            z_depth = 50
            z_rail = gl.GLBoxItem(size=QVector3D(z_width, z_depth, z_length))
            z_rail.setColor(QColor(100, 100, 255, 255))
            self.view.addItem(z_rail)
            self.parent.gl_items['z_rail'] = z_rail

        if 'z_axis' in self.parent.gl_items:
            self.view.removeItem(self.parent.gl_items['z_axis'])
            z_axis_line = np.array([[0, 0, 0], [0, 0, z_length/2]])
            z_axis = gl.GLLinePlotItem(pos=z_axis_line, color=(0, 0, 1, 1), width=self.line_width)
            self.view.addItem(z_axis)
            self.parent.gl_items['z_axis'] = z_axis

            if 'z_label' in self.parent.gl_items:
                self.view.removeItem(self.parent.gl_items['z_label'])
                z_label = gl.GLTextItem(pos=np.array([0, 0, z_length/2 + 50]), text='Z-AXIS', color=(0, 0, 1, 1))
                self.view.addItem(z_label)
                self.parent.gl_items['z_label'] = z_label

    def reset_rail_lengths(self):
        """Reset rail lengths to default values."""
        default_lengths = {
            'x': 1500,
            'y': 500,
            'z': 1200
        }

        self.parent.rail_lengths = default_lengths.copy()

        for axis, value in default_lengths.items():
            self.parent.ui_builder.rail_length_inputs[axis].setValue(value)

        self.apply_rail_lengths()

    def apply_relative_positions(self):
        """Apply the current relative position offsets to the 3D model."""
        if not self.parent.initialized or not hasattr(self, 'view'):
            return

        try:
            self.update_visualization()
            self.view.update()
        except Exception as e:
            print(f"Error applying relative positions: {str(e)}")

    def reset_relative_positions(self):
        """Reset relative positions to default values (all zeros)."""
        default_positions = {
            'x_offset': 0,
            'y_offset': 0,
            'z_offset': 0
        }

        self.parent.relative_positions = default_positions.copy()

        for key, value in default_positions.items():
            self.parent.ui_builder.rel_pos_inputs[key].setValue(value)

        self.apply_relative_positions()

    def apply_all_ranges(self):
        """Apply all axis range settings to sliders and validate positions."""
        for axis in SLAVE_IDS:
            axis_lower = axis.lower()

            # Update slider ranges
            self.parent.ui_builder.sliders[axis_lower].setMinimum(self.parent.axis_ranges[axis_lower]['min'])
            self.parent.ui_builder.sliders[axis_lower].setMaximum(self.parent.axis_ranges[axis_lower]['max'])

            # Validate current position against new ranges
            current_pos = self.parent.positions[axis_lower]
            min_val = self.parent.axis_ranges[axis_lower]['min']
            max_val = self.parent.axis_ranges[axis_lower]['max']

            if current_pos < min_val:
                self.parent.update_position(axis_lower, min_val)
            elif current_pos > max_val:
                self.parent.update_position(axis_lower, max_val)

            # Update tick interval based on range
            range_size = max_val - min_val
            tick_interval = max(int(range_size / 10), 1)
            self.parent.ui_builder.sliders[axis_lower].setTickInterval(tick_interval)

    def reset_ranges(self):
        """Reset all axis ranges to default values."""
        default_min = -1000
        default_max = 1000

        for axis in SLAVE_IDS:
            self.parent.axis_ranges[axis.lower()]['min'] = default_min
            self.parent.axis_ranges[axis.lower()]['max'] = default_max

            self.parent.ui_builder.min_inputs[axis.lower()].setValue(default_min)
            self.parent.ui_builder.max_inputs[axis.lower()].setValue(default_max)

        self.apply_all_ranges()

    def update_visualization(self):
        """Update the 3D visualization based on current positions and settings."""
        if not self.parent.initialized or not hasattr(self, 'view'):
            return

        try:
            # Get current axis positions
            x_pos = self.parent.positions['x']
            y_pos = self.parent.positions['y']
            z_pos = self.parent.positions['z']
            t_pos = self.parent.positions['t']
            g_pos = self.parent.positions['g']

            # Get offset values
            x_offset = self.parent.relative_positions['x_offset']
            y_offset = self.parent.relative_positions['y_offset']
            z_offset = self.parent.relative_positions['z_offset']

            # Get rail lengths
            z_length = self.parent.rail_lengths['z']
            y_length = self.parent.rail_lengths['y']

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

            # Apply Y-axis inversion if necessary
            y_direction = -1 if self.parent.axis_inverted['y'] else 1

            # Calculate normalized Y position (0 to y_length)
            y_normalized = (y_pos / 1000) * y_length
            y_normalized = max(0, min(y_length, y_normalized))

            # Calculate Y-axis position
            y_axis_pos = y_direction * y_normalized

            # Update Y-axis if needed
            self._update_y_axis_position(y_length)

            # Update Y-rail position
            self._position_y_rail(x_pos, x_offset, y_length, y_offset)

            # Calculate end position on Y rail
            if self.parent.axis_inverted['y']:
                y_end_pos = y_offset - y_normalized
            else:
                y_end_pos = y_offset - y_normalized

            # Update Z-rail position (attached to Y-rail end)
            self._position_z_rail(x_pos, x_offset, y_end_pos, z_length, z_offset, z_width)

            # Calculate Z carriage position
            z_carriage_pos = self._calculate_z_carriage_position(z_pos, z_length, carriage_height)

            # Update Z carriage position
            self._position_z_carriage(x_pos, x_offset, y_end_pos, z_carriage_pos, z_offset, carriage_size)

            # Update T part position and rotation
            self._position_t_part(x_pos, x_offset, y_end_pos, z_carriage_pos, z_offset, t_pos, t_size, t_height)

            # Calculate gripper spacing based on G position
            gripper_spacing = max(30, 150 - g_pos / 10)

            # Update gripper positions and rotations
            self._position_grippers(x_pos, x_offset, y_end_pos, z_carriage_pos, z_offset, t_pos, t_height,
                                    gripper_spacing, gripper_thickness, gripper_length, gripper_height)

            # Force redraw
            self.view.update()
        except Exception as e:
            print(f"Error updating visualization: {str(e)}")

    def _update_y_axis_position(self, y_length):
        """Update the Y-axis position and label."""
        if 'y_axis' in self.parent.gl_items:
            self.view.removeItem(self.parent.gl_items['y_axis'])
            y_start = np.array([0, 0, 0])
            y_end = np.array([0, -y_length, 0])
            y_axis_line = np.array([y_start, y_end])
            y_axis = gl.GLLinePlotItem(pos=y_axis_line, color=(0, 1, 0, 1), width=self.line_width)
            self.view.addItem(y_axis)
            self.parent.gl_items['y_axis'] = y_axis

            if 'y_label' in self.parent.gl_items:
                self.view.removeItem(self.parent.gl_items['y_label'])
                y_label = gl.GLTextItem(pos=np.array([0, -y_length - 20, 0]), text='Y-AXIS', color=(0, 1, 0, 1))
                self.view.addItem(y_label)
                self.parent.gl_items['y_label'] = y_label

    def _position_y_rail(self, x_pos, x_offset, y_length, y_offset):
        """Position the Y-rail based on X position."""
        if 'y_rail' in self.parent.gl_items:
            self.parent.gl_items['y_rail'].resetTransform()
            self.parent.gl_items['y_rail'].translate(
                -x_pos + x_offset,
                -y_length/2 + y_offset,
                0
            )

    def _position_z_rail(self, x_pos, x_offset, y_end_pos, z_length, z_offset, z_width):
        """Position the Z-rail based on X and Y positions."""
        if 'z_rail' in self.parent.gl_items:
            self.parent.gl_items['z_rail'].resetTransform()
            self.parent.gl_items['z_rail'].translate(
                -x_pos-z_width/2 + x_offset,
                y_end_pos,
                -z_length + z_offset
            )

    def _calculate_z_carriage_position(self, z_pos, z_length, carriage_height):
        """Calculate the Z carriage vertical position."""
        z_carriage_pos = -carriage_height + (z_pos / 1000) * (-z_length + carriage_height + 50)
        z_carriage_pos = max(-z_length + carriage_height, min(-carriage_height, z_carriage_pos))
        return z_carriage_pos

    def _position_z_carriage(self, x_pos, x_offset, y_end_pos, z_carriage_pos, z_offset, carriage_size):
        """Position the Z carriage based on X, Y, Z positions."""
        if 'z_carriage' in self.parent.gl_items:
            self.parent.gl_items['z_carriage'].resetTransform()
            self.parent.gl_items['z_carriage'].translate(
                -x_pos-carriage_size/2 + x_offset,
                y_end_pos-carriage_size/2,
                z_carriage_pos + z_offset
            )

    def _position_t_part(self, x_pos, x_offset, y_end_pos, z_carriage_pos, z_offset, t_pos, t_size, t_height):
        """Position and rotate the T part based on X, Y, Z, T positions."""
        if 't_part' in self.parent.gl_items:
            self.parent.gl_items['t_part'].resetTransform()
            self.parent.gl_items['t_part'].rotate(t_pos / 10, 0, 0, 1)
            self.parent.gl_items['t_part'].translate(
                -x_pos-t_size/2 + x_offset,
                y_end_pos-t_size/2,
                z_carriage_pos-t_height + z_offset
            )

    def _position_grippers(self, x_pos, x_offset, y_end_pos, z_carriage_pos, z_offset, t_pos, t_height,
                           gripper_spacing, gripper_thickness, gripper_length, gripper_height):
        """Position and rotate the gripper parts based on all axis positions."""
        if 'g_left' in self.parent.gl_items and 'g_right' in self.parent.gl_items:
            # Reset transforms
            self.parent.gl_items['g_left'].resetTransform()
            self.parent.gl_items['g_right'].resetTransform()

            # Apply rotation from T-axis
            self.parent.gl_items['g_left'].rotate(t_pos / 10, 0, 0, 1)
            self.parent.gl_items['g_right'].rotate(t_pos / 10, 0, 0, 1)

            # Position left gripper
            self.parent.gl_items['g_left'].translate(
                -x_pos-gripper_spacing/2-gripper_thickness + x_offset,
                y_end_pos-gripper_length/2,
                z_carriage_pos-t_height-gripper_height + z_offset
            )

            # Position right gripper
            self.parent.gl_items['g_right'].translate(
                -x_pos+gripper_spacing/2 + x_offset,
                y_end_pos-gripper_length/2,
                z_carriage_pos-t_height-gripper_height + z_offset
            )
