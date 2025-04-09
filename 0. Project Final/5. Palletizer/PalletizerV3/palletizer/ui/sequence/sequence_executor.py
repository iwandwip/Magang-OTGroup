from PyQt5.QtWidgets import QMessageBox
from PyQt5.QtCore import QObject, pyqtSignal
from ...utils.config import CMD_START


class SequenceExecutor(QObject):
    """Class for executing sequences"""
    global_command = pyqtSignal(str)
    sequence_command = pyqtSignal(str)
    execution_state_changed = pyqtSignal(bool)  # Signal emitted when execution state changes

    def __init__(self, parent=None):
        super().__init__(parent)
        self.current_sequence_index = -1  # Index of the current row being executed
        self.sequence_execution_active = False  # Flag to track if sequence execution is in progress
        self.row_manager = None  # Will be set from SequencePanel
        self.waiting_for_completion = False  # Flag to track if we're waiting for ALL_SLAVES_COMPLETED

    def set_row_manager(self, row_manager):
        """Set the reference to the row manager"""
        self.row_manager = row_manager

    def run_next_row(self):
        """Run the next row in the sequence"""
        if not self.row_manager or not self.row_manager.sequence_rows:
            QMessageBox.warning(None, "No Rows", "No sequence rows to run.")
            return False

        if self.current_sequence_index >= len(self.row_manager.sequence_rows) - 1:
            # We're at the end of the sequence
            # Only show message in manual mode
            if not self.parent().auto_execution:
                QMessageBox.information(None, "Sequence Complete", "All rows have been executed.")

            self.sequence_execution_active = False
            self.waiting_for_completion = False
            self.current_sequence_index = -1
            self.execution_state_changed.emit(False)
            # Update running row display to show completion
            self.parent().update_running_row_display(-1)
            return False

        # Move to the next row
        self.current_sequence_index += 1
        row_index = self.current_sequence_index

        # Get the current row command
        row_command = self.row_manager.get_row_command(row_index)

        if not row_command:
            return False

        # Only send START command if this is the first row or after a reset
        if row_index == 0 or not self.sequence_execution_active:
            self.global_command.emit(CMD_START)
            self.sequence_execution_active = True

        # Send row command
        self.sequence_command.emit(row_command)

        # Set waiting flag
        self.waiting_for_completion = True

        # Update execution state
        self.execution_state_changed.emit(True)

        # Update running row display
        self.parent().update_running_row_display(row_index)

        # Show status message only in manual mode
        if not self.parent().auto_execution:
            QMessageBox.information(None, "Running Sequence", f"Running Row {row_index + 1}")

        return True

    def run_all_rows(self):
        """Start running the sequence row by row"""
        if not self.row_manager or not self.row_manager.sequence_rows:
            QMessageBox.warning(None, "No Rows", "No sequence rows to run.")
            return False

        # Confirm run
        reply = QMessageBox.question(
            None,
            "Run All Rows",
            f"Are you sure you want to run all {len(self.row_manager.sequence_rows)} rows in sequence?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )

        if reply == QMessageBox.Yes:
            # Reset sequence execution state
            self.current_sequence_index = -1
            self.sequence_execution_active = False
            self.waiting_for_completion = False

            # Start with the first row
            return self.run_next_row()

        return False

    def run_selected_row(self, row_index):
        """Run only the selected row"""
        if not self.row_manager:
            return False

        if row_index < 0 or row_index >= len(self.row_manager.sequence_rows):
            QMessageBox.warning(None, "Invalid Selection", f"Row {row_index + 1} does not exist.")
            return False

        # Get the row command
        row_command = self.row_manager.get_row_command(row_index)

        if not row_command:
            return False

        # Send START command first
        self.global_command.emit(CMD_START)

        # Then send row command
        self.sequence_command.emit(row_command)

        # Not setting sequence_execution_active to True because this is not part of sequenced execution
        # But we are waiting for completion
        self.waiting_for_completion = True

        # Update running row display
        self.parent().update_running_row_display(row_index)

        # Show status message only in manual mode
        if not self.parent().auto_execution:
            QMessageBox.information(None, "Running Sequence", f"Running Row {row_index + 1}")

        return True

    def run_single_axis(self, row_index, axis):
        """Run only a specific axis from the selected row"""
        if not self.row_manager:
            return False

        if row_index < 0 or row_index >= len(self.row_manager.sequence_rows):
            QMessageBox.warning(None, "Invalid Selection", f"Row {row_index + 1} does not exist.")
            return False

        # Get the row
        row = self.row_manager.sequence_rows[row_index]

        if axis not in row:
            QMessageBox.warning(None, "No Axis", f"Selected row does not contain a sequence for axis {axis.upper()}.")
            return False

        # Send START command first
        self.global_command.emit(CMD_START)

        # Then send axis command
        self.sequence_command.emit(row[axis])

        # Not setting sequence_execution_active to True because this is only a single axis
        # But we are waiting for completion
        self.waiting_for_completion = True

        # Update running row display with indication of which axis is running
        self.parent().running_row_value.setText(f"{row_index+1}.{axis.upper()}")
        self.parent().running_row_frame.setStyleSheet("background-color: #e0e0ff; border-radius: 3px;")

        # Show status message only in manual mode
        if not self.parent().auto_execution:
            QMessageBox.information(None, "Running Sequence", f"Running Axis {axis.upper()} from Row {row_index + 1}")

        return True

    def handle_slave_completion(self):
        """Called when all slaves have completed their tasks"""
        # If we're not waiting for completion, then ignore this signal
        if not self.waiting_for_completion:
            return False

        # Reset the waiting flag
        self.waiting_for_completion = False

        # Re-enable the Next button if we're in sequence execution mode
        if self.sequence_execution_active:
            self.execution_state_changed.emit(True)
            return True

        return False
