from PyQt5.QtCore import QObject, pyqtSignal
from ..utils.config import SLAVE_IDS


class PositionTracker(QObject):
    """
    Tracks the position of each axis based on commands sent.
    Uses absolute positioning model - each command specifies the target position.
    """
    position_updated = pyqtSignal(str, int)  # Emits (axis_id, new_position) when position changes
    target_position_updated = pyqtSignal(str, int)  # Emits when a target position is set

    def __init__(self, parent=None):
        super().__init__(parent)
        self.positions = {}
        self.target_positions = {}
        self.reset_all_positions()

    def reset_all_positions(self):
        """Reset all axis positions to zero"""
        for slave_id in SLAVE_IDS:
            self.positions[slave_id] = 0
            self.target_positions[slave_id] = 0
            self.position_updated.emit(slave_id, 0)

    def reset_position(self, axis_id):
        """Reset a specific axis position to zero"""
        if axis_id.lower() in self.positions:
            self.positions[axis_id.lower()] = 0
            self.target_positions[axis_id.lower()] = 0
            self.position_updated.emit(axis_id.lower(), 0)
            return True
        return False

    def set_position(self, axis_id, position):
        """
        Set the position of an axis directly.
        In absolute positioning, this is the current actual position.
        """
        if axis_id.lower() in self.positions:
            self.positions[axis_id.lower()] = position
            self.position_updated.emit(axis_id.lower(), position)
            return True
        return False

    def set_target_position(self, axis_id, position):
        """
        Set the target position for an axis.
        This is the position the axis will move to.
        """
        if axis_id.lower() in self.target_positions:
            self.target_positions[axis_id.lower()] = position
            self.target_position_updated.emit(axis_id.lower(), position)
            # Once the target is reached, the actual position becomes the target
            self.set_position(axis_id, position)
            return True
        return False

    def get_position(self, axis_id):
        """Get the current position of an axis"""
        return self.positions.get(axis_id.lower(), 0)

    def get_target_position(self, axis_id):
        """Get the target position of an axis"""
        return self.target_positions.get(axis_id.lower(), 0)

    def get_all_positions(self):
        """Get a dictionary of all current positions"""
        return self.positions.copy()

    def parse_command(self, command):
        """
        Parse a command and update positions accordingly.
        Handles absolute positioning commands.
        """
        if not command:
            return False

        # Handle multi-axis commands like "x(100),y(200)"
        if ',' in command and ')' in command and '(' in command:
            # Check if the command contains parentheses - complex command vs simple list
            if '(' in command:
                # Split by commas outside parentheses
                parts = []
                current_part = ""
                paren_level = 0

                for char in command:
                    if char == '(':
                        paren_level += 1
                        current_part += char
                    elif char == ')':
                        paren_level -= 1
                        current_part += char
                    elif char == ',' and paren_level == 0:
                        # This is a comma outside parentheses, split here
                        if current_part:
                            parts.append(current_part)
                        current_part = ""
                    else:
                        current_part += char

                if current_part:
                    parts.append(current_part)

                # Process each part
                updated = False
                for part in parts:
                    if self.parse_single_command(part):
                        updated = True

                return updated
            else:
                # Simple comma-separated values within one axis command
                return self.parse_single_command(command)
        else:
            # Handle single-axis commands
            return self.parse_single_command(command)

    def parse_single_command(self, command):
        """
        Parse a single-axis command like "x(100)" or "x(100,200,300)".
        For absolute positioning, the last value is the target position.
        """
        try:
            # Extract axis and values
            if '(' in command and ')' in command:
                axis_id = command.split('(')[0].strip().lower()
                values_str = command.split('(')[1].split(')')[0]

                # Handle simple values like "x(100)"
                if axis_id in self.positions:
                    # Process values inside parentheses
                    values = values_str.split(',')
                    absolute_position = None

                    # In absolute positioning, we take the last valid position value
                    for value in reversed(values):
                        # Skip delay values (starts with 'd')
                        if value.startswith('d'):
                            continue

                        try:
                            position = int(value)
                            absolute_position = position
                            break
                        except ValueError:
                            # Not a number, might be a delay or other parameter
                            continue

                    # Update position with absolute position
                    if absolute_position is not None:
                        self.set_target_position(axis_id, absolute_position)
                        return True

            return False
        except Exception as e:
            print(f"Error parsing command '{command}': {str(e)}")
            return False
