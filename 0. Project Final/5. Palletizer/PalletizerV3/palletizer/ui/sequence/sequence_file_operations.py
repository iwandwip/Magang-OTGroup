import os
import yaml
from PyQt5.QtWidgets import (QMessageBox, QInputDialog, QFileDialog)
from PyQt5.QtCore import QObject, pyqtSignal


class SequenceFileManager(QObject):
    """Class for handling sequence file operations"""
    sequence_updated = pyqtSignal(str)  # Signal emitted when sequence name changes
    sequences_list_updated = pyqtSignal()  # Signal emitted when saved sequences list is updated

    def __init__(self, parent=None):
        super().__init__(parent)
        self.saved_sequences = {}  # Dictionary to store named sequences
        self.current_sequence_name = "Default"
        self.row_manager = None  # Will be set from SequencePanel

    def set_row_manager(self, row_manager):
        """Set the reference to the row manager"""
        self.row_manager = row_manager

    def on_new_sequence(self):
        """Create a new, empty sequence"""
        if not self.row_manager:
            return False

        if self.row_manager.sequence_rows:
            # Ask if user wants to save current sequence
            reply = QMessageBox.question(
                None,
                "Save Current Sequence?",
                "Do you want to save the current sequence before creating a new one?",
                QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                QMessageBox.Yes
            )

            if reply == QMessageBox.Cancel:
                return False

            if reply == QMessageBox.Yes:
                self.on_save_sequence()

        # Create new sequence
        name, ok = QInputDialog.getText(
            None, "New Sequence", "Enter name for new sequence:",
            text="New Sequence"
        )

        if ok and name:
            self.row_manager.sequence_rows = []
            self.current_sequence_name = name

            # Emit signal that sequence name has changed
            self.sequence_updated.emit(name)

            # Add to saved sequences
            self.saved_sequences[name] = []
            self.sequences_list_updated.emit()

            return True

        return False

    def on_save_sequence(self):
        """Save the current sequence to the saved sequences list"""
        if not self.row_manager:
            return False

        if not self.row_manager.sequence_rows:
            QMessageBox.warning(None, "Empty Sequence", "No sequence rows to save.")
            return False

        # Save to current sequence name
        self.saved_sequences[self.current_sequence_name] = self.row_manager.sequence_rows.copy()

        # Update the list
        self.sequences_list_updated.emit()

        # Offer to save to file
        reply = QMessageBox.question(
            None,
            "Save to File?",
            "Do you want to save this sequence to a file?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.Yes
        )

        if reply == QMessageBox.Yes:
            return self.save_sequence_to_file()
        else:
            QMessageBox.information(None, "Sequence Saved", f"Sequence '{self.current_sequence_name}' saved in memory.")
            return True

    def on_save_as_sequence(self):
        """Save the current sequence with a new name"""
        if not self.row_manager:
            return False

        if not self.row_manager.sequence_rows:
            QMessageBox.warning(None, "Empty Sequence", "No sequence rows to save.")
            return False

        # Ask for new name
        name, ok = QInputDialog.getText(
            None, "Save Sequence As", "Enter name for sequence:",
            text=self.current_sequence_name + "_copy"
        )

        if ok and name:
            self.current_sequence_name = name
            self.sequence_updated.emit(name)

            # Save with new name
            self.saved_sequences[name] = self.row_manager.sequence_rows.copy()
            self.sequences_list_updated.emit()

            # Save to file
            return self.save_sequence_to_file()

        return False

    def on_load_sequence(self):
        """Load a sequence from a file"""
        if not self.row_manager:
            return False

        # Ask if user wants to save current sequence
        if self.row_manager.sequence_rows:
            reply = QMessageBox.question(
                None,
                "Save Current Sequence?",
                "Do you want to save the current sequence before loading a new one?",
                QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                QMessageBox.Yes
            )

            if reply == QMessageBox.Cancel:
                return False

            if reply == QMessageBox.Yes:
                self.on_save_sequence()

        # Show file dialog to select a YAML file
        file_path, _ = QFileDialog.getOpenFileName(
            None, "Load Sequence", "", "YAML Files (*.yaml *.yml);;All Files (*)"
        )

        if file_path:
            return self.load_sequence_from_file(file_path)

        return False

    def on_sequence_selected(self, sequence_name):
        """Handle selection of a saved sequence from the list"""
        if not self.row_manager:
            return False

        if sequence_name not in self.saved_sequences:
            return False

        # Ask if user wants to save current sequence
        if self.row_manager.sequence_rows and sequence_name != self.current_sequence_name:
            reply = QMessageBox.question(
                None,
                "Save Current Sequence?",
                f"Do you want to save the current sequence '{self.current_sequence_name}' before loading '{sequence_name}'?",
                QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                QMessageBox.Yes
            )

            if reply == QMessageBox.Cancel:
                return False

            if reply == QMessageBox.Yes:
                # Save current sequence
                self.saved_sequences[self.current_sequence_name] = self.row_manager.sequence_rows.copy()

        # Load selected sequence
        self.row_manager.sequence_rows = self.saved_sequences[sequence_name].copy()
        self.current_sequence_name = sequence_name
        self.sequence_updated.emit(sequence_name)

        return True

    def save_sequence_to_file(self):
        """Save the current sequence to a YAML file"""
        if not self.row_manager:
            return False

        suggested_filename = self.current_sequence_name.replace(" ", "_") + ".yaml"

        file_path, _ = QFileDialog.getSaveFileName(
            None, "Save Sequence", suggested_filename, "YAML Files (*.yaml *.yml);;All Files (*)"
        )

        if file_path:
            try:
                # Prepare data for YAML
                sequence_data = {
                    'name': self.current_sequence_name,
                    'rows': []
                }

                for row in self.row_manager.sequence_rows:
                    sequence_data['rows'].append(row)

                # Write to file
                with open(file_path, 'w') as f:
                    yaml.dump(sequence_data, f, default_flow_style=False)

                QMessageBox.information(None, "Save Successful", f"Sequence saved to {file_path}")
                return True
            except Exception as e:
                QMessageBox.critical(None, "Save Failed", f"Failed to save sequence: {str(e)}")

        return False

    def load_sequence_from_file(self, file_path):
        """Load a sequence from a YAML file"""
        if not self.row_manager:
            return False

        try:
            # Read YAML file
            with open(file_path, 'r') as f:
                sequence_data = yaml.safe_load(f)

            # Extract data
            name = sequence_data.get('name', os.path.basename(file_path).split('.')[0])
            rows = sequence_data.get('rows', [])

            # Update current sequence
            self.row_manager.sequence_rows = rows
            self.current_sequence_name = name
            self.sequence_updated.emit(name)

            # Add to saved sequences if not already there
            if name not in self.saved_sequences:
                self.saved_sequences[name] = rows.copy()
                self.sequences_list_updated.emit()

            QMessageBox.information(None, "Load Successful", f"Loaded sequence '{name}' from {file_path}")
            return True
        except Exception as e:
            QMessageBox.critical(None, "Load Failed", f"Failed to load sequence: {str(e)}")

        return False

    def get_saved_sequence_names(self):
        """Get list of saved sequence names"""
        return sorted(self.saved_sequences.keys())
