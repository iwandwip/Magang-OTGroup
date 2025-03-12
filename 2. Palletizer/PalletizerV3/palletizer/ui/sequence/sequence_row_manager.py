from PyQt5.QtWidgets import (QMessageBox, QSpinBox, QCheckBox)
from PyQt5.QtCore import Qt, QObject, pyqtSignal


class SequenceRowManager(QObject):
    """Class for managing sequence rows"""
    row_updated = pyqtSignal()  # Signal emitted when rows are updated

    def __init__(self, parent=None):
        super().__init__(parent)
        self.sequence_rows = []      # List of rows, each row is a dict with axis sequences
        self.selected_row_index = -1  # Currently selected row for editing or running
        self.axis_inputs = {}        # References to UI controls for each axis

    def set_axis_inputs(self, axis_inputs):
        """Set the reference to UI controls for axes inputs"""
        self.axis_inputs = axis_inputs

    def create_sequence_from_inputs(self):
        """Create sequence strings for each axis based on current inputs"""
        sequences = {}

        for axis, steps in self.axis_inputs.items():
            sequence_parts = []

            for step in steps:
                if step['step_check'].isChecked():
                    value = step['step_value'].value()

                    # Add delay if checked (before the position)
                    if step['delay_check'].isChecked():
                        delay_val = step['delay_value'].value()
                        sequence_parts.append(f"d{delay_val}")

                    sequence_parts.append(str(value))

            if sequence_parts:
                # Create sequence string: "x(100,d500,200)" or "z(d500)"
                sequences[axis] = f"{axis}({','.join(sequence_parts)})"

        return sequences

    def add_new_row(self):
        """Add current inputs as a new row"""
        sequences = self.create_sequence_from_inputs()

        if not sequences:
            QMessageBox.warning(None, "Empty Sequence", "No sequence steps have been checked.")
            return False

        # Add to rows list
        self.sequence_rows.append(sequences)

        # Emit signal that rows have been updated
        self.row_updated.emit()

        # Show success message
        QMessageBox.information(None, "Row Added", f"Row {len(self.sequence_rows)} has been added.")
        return True

    def update_selected_row(self, row_index):
        """Update the selected row with current inputs"""
        if row_index < 0 or row_index >= len(self.sequence_rows):
            QMessageBox.warning(None, "Invalid Selection", f"Row {row_index + 1} does not exist.")
            return False

        # Update our internal tracking
        self.selected_row_index = row_index

        sequences = self.create_sequence_from_inputs()

        if not sequences:
            QMessageBox.warning(None, "Empty Sequence", "No sequence steps have been checked.")
            return False

        # Update row
        self.sequence_rows[row_index] = sequences

        # Emit signal that rows have been updated
        self.row_updated.emit()

        # Show success message
        QMessageBox.information(None, "Row Updated", f"Row {row_index + 1} has been updated.")
        return True

    def delete_row(self, row_index):
        """Delete the specified row"""
        if row_index < 0 or row_index >= len(self.sequence_rows):
            QMessageBox.warning(None, "Invalid Selection", f"Row {row_index + 1} does not exist.")
            return False

        # Delete row
        del self.sequence_rows[row_index]

        # Emit signal that rows have been updated
        self.row_updated.emit()
        return True

    def clear_all_rows(self):
        """Clear all rows"""
        if not self.sequence_rows:
            return False

        # Clear rows
        self.sequence_rows = []

        # Emit signal that rows have been updated
        self.row_updated.emit()
        return True

    def load_row_data_to_form(self, row_index):
        """Load row data into the input form for editing"""
        if row_index < 0 or row_index >= len(self.sequence_rows):
            return False

        row = self.sequence_rows[row_index]

        for axis, sequence in row.items():
            # Parse sequence: "x(100,d500,200)" -> [100, d500, 200]
            if '(' in sequence and ')' in sequence:
                steps_str = sequence.split('(')[1].split(')')[0]
                steps = steps_str.split(',')

                step_index = 0
                i = 0

                while i < len(steps) and step_index < 5:
                    step = steps[i]

                    if step.startswith('d'):
                        # It's a delay
                        if step_index < 5:
                            delay_value = int(step[1:])
                            self.axis_inputs[axis][step_index]['delay_check'].setChecked(True)
                            self.axis_inputs[axis][step_index]['delay_value'].setValue(delay_value)

                        # Don't increment step_index yet, delay is part of the next position
                        i += 1

                        if i < len(steps) and step_index < 5:
                            # Get the position value that follows
                            position = int(steps[i])
                            self.axis_inputs[axis][step_index]['step_check'].setChecked(True)
                            self.axis_inputs[axis][step_index]['step_value'].setValue(position)

                            step_index += 1
                            i += 1
                    else:
                        # Regular position
                        position = int(step)
                        self.axis_inputs[axis][step_index]['step_check'].setChecked(True)
                        self.axis_inputs[axis][step_index]['step_value'].setValue(position)

                        step_index += 1
                        i += 1

        # Update selected row index
        self.selected_row_index = row_index

        return True

    def get_row_text_representation(self):
        """Generate text representation of all rows for display"""
        text = ""
        for i, row in enumerate(self.sequence_rows):
            row_text = f"Row {i+1}: "
            row_parts = []

            for axis, sequence in row.items():
                row_parts.append(sequence)

            row_text += ", ".join(row_parts)
            text += row_text + "\n"

        return text

    def get_row_command(self, row_index):
        """Get command string for a specific row"""
        if row_index < 0 or row_index >= len(self.sequence_rows):
            return ""

        row = self.sequence_rows[row_index]
        cmd_parts = []

        for axis, sequence in row.items():
            cmd_parts.append(sequence)

        return ", ".join(cmd_parts)

    def get_command_preview(self, row_index=-1):
        """Generate command preview text for display"""
        if row_index >= 0 and row_index < len(self.sequence_rows):
            # Show preview for selected row
            row = self.sequence_rows[row_index]
            cmd_parts = []

            for axis, sequence in row.items():
                cmd_parts.append(sequence)

            return "Selected Row Command:\n" + ", ".join(cmd_parts)
        else:
            # Show preview for all rows
            if not self.sequence_rows:
                return ""

            preview = "All Rows Commands:\n\n"

            for i, row in enumerate(self.sequence_rows):
                cmd_parts = []

                for axis, sequence in row.items():
                    cmd_parts.append(sequence)

                preview += f"Row {i+1}:\n" + ", ".join(cmd_parts) + "\n\n"

            return preview
