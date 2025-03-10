import os
import yaml
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
                             QLabel, QPushButton, QGroupBox, QLineEdit, QTextEdit,
                             QSpinBox, QCheckBox, QTabWidget, QComboBox, QSplitter,
                             QScrollArea, QFrame, QSizePolicy, QListWidget, QMessageBox,
                             QInputDialog, QFileDialog)
from PyQt5.QtCore import Qt, pyqtSignal
from ..utils.config import *


class SequencePanel(QWidget):
    """Panel untuk mengatur dan menjalankan sekuens gerakan dengan sistem row dan sequence"""
    sequence_command = pyqtSignal(str)
    global_command = pyqtSignal(str)
    speed_command = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.sequence_rows = []  # List of rows, each row is a dict with axis sequences
        self.current_row = {}    # Current row being edited
        self.selected_row_index = -1  # Currently selected row for editing or running

        # Variables for sequence step-by-step execution
        self.current_sequence_index = -1  # Index of the current row being executed
        self.sequence_execution_active = False  # Flag to track if sequence execution is in progress

        # Variables for saved sequences
        self.saved_sequences = {}  # Dictionary to store named sequences
        self.current_sequence_name = "Default"

        self.setup_ui()

    def setup_ui(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)  # Reduce margins
        main_layout.setSpacing(5)  # Tighter spacing

        # ======== Global Commands Section ========
        global_group = QGroupBox("Global Commands")
        global_layout = QHBoxLayout()
        global_layout.setSpacing(5)  # Tighter spacing

        # Global command buttons in a row
        self.start_btn = QPushButton("START")
        self.start_btn.clicked.connect(lambda: self.global_command.emit(CMD_START))
        self.start_btn.setStyleSheet(BUTTON_START)
        self.start_btn.setMinimumHeight(40)

        self.zero_btn = QPushButton("HOME")
        self.zero_btn.clicked.connect(lambda: self.global_command.emit(CMD_ZERO))
        self.zero_btn.setStyleSheet(BUTTON_HOME)
        self.zero_btn.setMinimumHeight(40)

        self.pause_btn = QPushButton("PAUSE")
        self.pause_btn.clicked.connect(lambda: self.global_command.emit(CMD_PAUSE))
        self.pause_btn.setStyleSheet(BUTTON_PAUSE)
        self.pause_btn.setMinimumHeight(40)

        self.resume_btn = QPushButton("RESUME")
        self.resume_btn.clicked.connect(lambda: self.global_command.emit(CMD_RESUME))
        self.resume_btn.setStyleSheet(BUTTON_RESUME)
        self.resume_btn.setMinimumHeight(40)

        self.reset_btn = QPushButton("STOP/RESET")
        self.reset_btn.clicked.connect(lambda: self.global_command.emit(CMD_RESET))
        self.reset_btn.setStyleSheet(BUTTON_STOP)
        self.reset_btn.setMinimumHeight(40)

        # Add buttons to global layout
        global_layout.addWidget(self.start_btn)
        global_layout.addWidget(self.zero_btn)
        global_layout.addWidget(self.pause_btn)
        global_layout.addWidget(self.resume_btn)
        global_layout.addWidget(self.reset_btn)

        global_group.setLayout(global_layout)
        main_layout.addWidget(global_group)

        # ======== Speed Control Section ========
        speed_group = QGroupBox("Global Speed Control")
        speed_layout = QHBoxLayout()
        speed_layout.setSpacing(5)  # Tighter spacing

        speed_layout.addWidget(QLabel("Set Speed for All Motors:"))

        self.global_speed_spinbox = QSpinBox()
        self.global_speed_spinbox.setRange(MIN_SPEED, MAX_SPEED)
        self.global_speed_spinbox.setValue(DEFAULT_SPEED)
        self.global_speed_spinbox.setSingleStep(SPEED_STEP)

        self.set_global_speed_btn = QPushButton("Apply Speed")
        self.set_global_speed_btn.clicked.connect(self.on_set_global_speed)
        self.set_global_speed_btn.setStyleSheet(BUTTON_SPEED)

        speed_layout.addWidget(self.global_speed_spinbox)
        speed_layout.addWidget(self.set_global_speed_btn)
        speed_layout.addStretch()

        speed_group.setLayout(speed_layout)
        main_layout.addWidget(speed_group)

        # ======== Main Sequence Section ========
        # Using QSplitter for better resizing control
        sequence_splitter = QSplitter(Qt.Horizontal)
        sequence_splitter.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        # Left side - Sequence management and row list
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(5)  # Tighter spacing

        # ======== Sequence File Management Section ========
        file_group = QGroupBox("Sequence File Management")
        file_layout = QVBoxLayout()
        file_layout.setSpacing(5)  # Tighter spacing

        # Current sequence name display
        sequence_name_layout = QHBoxLayout()
        sequence_name_layout.addWidget(QLabel("Current Sequence:"))
        self.sequence_name_label = QLabel(self.current_sequence_name)
        self.sequence_name_label.setStyleSheet("font-weight: bold;")
        sequence_name_layout.addWidget(self.sequence_name_label)
        sequence_name_layout.addStretch()

        # Saved sequences selection
        self.saved_sequences_list = QListWidget()
        self.saved_sequences_list.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.saved_sequences_list.setMinimumHeight(80)  # Give it some minimum height
        self.saved_sequences_list.itemDoubleClicked.connect(self.on_sequence_selected)

        # File operation buttons
        file_buttons_layout = QHBoxLayout()
        file_buttons_layout.setSpacing(5)  # Tighter spacing

        self.new_sequence_btn = QPushButton("New Sequence")
        self.new_sequence_btn.clicked.connect(self.on_new_sequence)
        self.new_sequence_btn.setStyleSheet(BUTTON_SPEED)

        self.save_sequence_btn = QPushButton("Save Sequence")
        self.save_sequence_btn.clicked.connect(self.on_save_sequence)
        self.save_sequence_btn.setStyleSheet("background-color: #e0ffea;")

        self.save_as_sequence_btn = QPushButton("Save As...")
        self.save_as_sequence_btn.clicked.connect(self.on_save_as_sequence)
        self.save_as_sequence_btn.setStyleSheet("background-color: #e0ffea;")

        self.load_sequence_btn = QPushButton("Load From File")
        self.load_sequence_btn.clicked.connect(self.on_load_sequence)
        self.load_sequence_btn.setStyleSheet("background-color: #fff9e0;")

        file_buttons_layout.addWidget(self.new_sequence_btn)
        file_buttons_layout.addWidget(self.save_sequence_btn)
        file_buttons_layout.addWidget(self.save_as_sequence_btn)
        file_buttons_layout.addWidget(self.load_sequence_btn)

        file_layout.addLayout(sequence_name_layout)
        file_layout.addWidget(self.saved_sequences_list)
        file_layout.addLayout(file_buttons_layout)

        file_group.setLayout(file_layout)
        left_layout.addWidget(file_group)

        # Row list section
        row_list_group = QGroupBox("Sequence Rows")
        row_list_layout = QVBoxLayout()
        row_list_layout.setSpacing(5)  # Tighter spacing

        # Row list widget
        self.row_list = QTextEdit()
        self.row_list.setReadOnly(True)
        self.row_list.setPlaceholderText("No sequences added yet...")
        self.row_list.setMinimumHeight(120)
        self.row_list.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        # Row control buttons
        row_control_layout = QHBoxLayout()
        row_control_layout.setSpacing(5)  # Tighter spacing

        self.run_all_btn = QPushButton("Run All Rows")
        self.run_all_btn.clicked.connect(self.run_all_rows)
        self.run_all_btn.setStyleSheet(BUTTON_START)

        self.run_selected_btn = QPushButton("Run Selected Row")
        self.run_selected_btn.clicked.connect(self.run_selected_row)
        self.run_selected_btn.setStyleSheet("background-color: #ccffdd;")

        # Add a Next button for step-by-step execution
        self.next_btn = QPushButton("Next Row")
        self.next_btn.clicked.connect(self.run_next_row)
        self.next_btn.setEnabled(False)  # Disabled by default
        self.next_btn.setStyleSheet("background-color: #ffddaa; font-weight: bold;")

        self.clear_all_rows_btn = QPushButton("Clear All")
        self.clear_all_rows_btn.clicked.connect(self.clear_all_rows)
        self.clear_all_rows_btn.setStyleSheet(BUTTON_STOP)

        row_control_layout.addWidget(self.run_all_btn)
        row_control_layout.addWidget(self.run_selected_btn)
        row_control_layout.addWidget(self.next_btn)  # Add the Next button
        row_control_layout.addWidget(self.clear_all_rows_btn)

        row_list_layout.addWidget(self.row_list)
        row_list_layout.addLayout(row_control_layout)

        row_list_group.setLayout(row_list_layout)
        left_layout.addWidget(row_list_group)

        # Sequence Builder Section
        builder_group = QGroupBox("Edit Sequence")
        builder_layout = QVBoxLayout()
        builder_layout.setSpacing(5)  # Tighter spacing

        # Create a scroll area for the sequence builder
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setFrameShape(QFrame.NoFrame)

        # Widget to contain all tabs
        tab_container = QWidget()
        tab_layout = QVBoxLayout(tab_container)
        tab_layout.setContentsMargins(0, 0, 0, 0)

        # Tabs for each axis
        self.axis_tabs = QTabWidget()
        self.axis_tabs.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        # Create tabs for each axis
        axes = SLAVE_IDS
        self.axis_inputs = {}

        for axis in axes:
            axis_tab = QWidget()
            axis_layout = QVBoxLayout(axis_tab)
            axis_layout.setSpacing(5)  # Tighter spacing

            # Create 5 step input rows for this axis
            step_inputs = []
            steps_layout = QGridLayout()
            steps_layout.setSpacing(5)  # Tighter spacing
            steps_layout.setVerticalSpacing(10)  # Slightly more vertical space

            for i in range(5):
                step_layout = QHBoxLayout()

                # Step checkbox and value
                step_check = QCheckBox(f"Step {i+1}:")
                step_value = QSpinBox()
                step_value.setRange(-MAX_STEPS, MAX_STEPS)
                step_value.setSingleStep(100)
                step_value.setEnabled(False)  # Disabled until checked

                # Connect checkbox to enable/disable input
                step_check.stateChanged.connect(
                    lambda state, val=step_value: val.setEnabled(state == Qt.Checked)
                )

                # Delay checkbox for this step
                delay_check = QCheckBox("Delay")
                delay_value = QSpinBox()
                delay_value.setRange(0, 10000)
                delay_value.setSingleStep(100)
                delay_value.setValue(500)
                delay_value.setEnabled(False)  # Disabled until checked

                # Connect checkbox to enable/disable delay input
                delay_check.stateChanged.connect(
                    lambda state, val=delay_value: val.setEnabled(state == Qt.Checked)
                )

                # Add to grid layout
                steps_layout.addWidget(step_check, i, 0)
                steps_layout.addWidget(step_value, i, 1)
                steps_layout.addWidget(delay_check, i, 2)
                steps_layout.addWidget(delay_value, i, 3)

                # Save references to inputs
                step_inputs.append({
                    'step_check': step_check,
                    'step_value': step_value,
                    'delay_check': delay_check,
                    'delay_value': delay_value
                })

            # Add steps layout to tab
            axis_layout.addLayout(steps_layout)
            axis_layout.addStretch()

            # Add to tabs
            self.axis_tabs.addTab(axis_tab, axis.upper())

            # Save references to all inputs for this axis
            self.axis_inputs[axis.lower()] = step_inputs

        # Add axis tabs to the container
        tab_layout.addWidget(self.axis_tabs)

        # Set the container as the scroll area widget
        scroll_area.setWidget(tab_container)

        # Add the scroll area to the builder layout
        builder_layout.addWidget(scroll_area)

        # Row control buttons
        row_action_layout = QHBoxLayout()
        row_action_layout.setSpacing(5)  # Tighter spacing

        self.add_row_btn = QPushButton("Add as New Row")
        self.add_row_btn.clicked.connect(self.add_new_row)
        self.add_row_btn.setStyleSheet(BUTTON_START)

        self.update_row_btn = QPushButton("Update Selected Row")
        self.update_row_btn.clicked.connect(self.update_selected_row)
        self.update_row_btn.setEnabled(False)  # Initially disabled

        self.clear_form_btn = QPushButton("Clear Form")
        self.clear_form_btn.clicked.connect(self.clear_form)

        row_action_layout.addWidget(self.add_row_btn)
        row_action_layout.addWidget(self.update_row_btn)
        row_action_layout.addWidget(self.clear_form_btn)

        builder_layout.addLayout(row_action_layout)
        builder_group.setLayout(builder_layout)

        left_layout.addWidget(builder_group)

        # Right side - Row testing and preview
        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        right_layout.setContentsMargins(0, 0, 0, 0)
        right_layout.setSpacing(5)  # Tighter spacing

        # Row selection and editing
        selection_group = QGroupBox("Row Selection")
        selection_layout = QVBoxLayout()
        selection_layout.setSpacing(5)  # Tighter spacing

        selection_row_layout = QHBoxLayout()
        selection_row_layout.addWidget(QLabel("Select Row:"))

        self.row_selector = QComboBox()
        self.row_selector.addItem("-- None --")
        self.row_selector.currentIndexChanged.connect(self.on_row_selected)

        self.edit_row_btn = QPushButton("Edit Selected")
        self.edit_row_btn.clicked.connect(self.edit_selected_row)
        self.edit_row_btn.setEnabled(False)  # Initially disabled

        self.delete_row_btn = QPushButton("Delete Selected")
        self.delete_row_btn.clicked.connect(self.delete_selected_row)
        self.delete_row_btn.setEnabled(False)  # Initially disabled

        selection_row_layout.addWidget(self.row_selector)
        selection_row_layout.addWidget(self.edit_row_btn)
        selection_row_layout.addWidget(self.delete_row_btn)

        selection_layout.addLayout(selection_row_layout)

        # Run specific axis
        axis_run_layout = QHBoxLayout()
        axis_run_layout.setSpacing(5)  # Tighter spacing
        axis_run_layout.addWidget(QLabel("Run Single Axis:"))

        for axis in axes:
            axis_btn = QPushButton(f"Run {axis.upper()}")
            axis_btn.clicked.connect(lambda checked, a=axis.lower(): self.run_single_axis(a))
            axis_btn.setStyleSheet(BUTTON_SPEED)
            axis_run_layout.addWidget(axis_btn)

        selection_layout.addLayout(axis_run_layout)
        selection_group.setLayout(selection_layout)

        # Command preview
        preview_group = QGroupBox("Command Preview")
        preview_layout = QVBoxLayout()
        preview_layout.setSpacing(5)  # Tighter spacing

        self.command_preview = QTextEdit()
        self.command_preview.setReadOnly(True)
        self.command_preview.setPlaceholderText("Command preview will appear here...")
        self.command_preview.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        preview_layout.addWidget(self.command_preview)
        preview_group.setLayout(preview_layout)

        right_layout.addWidget(selection_group)
        right_layout.addWidget(preview_group)

        # Add both panels to the splitter
        sequence_splitter.addWidget(left_panel)
        sequence_splitter.addWidget(right_panel)

        # Set initial sizes for the splitter (approximately 60% / 40%)
        sequence_splitter.setSizes([600, 400])

        main_layout.addWidget(sequence_splitter, 1)  # Give more space to sequence container

    def on_set_global_speed(self):
        speed_value = self.global_speed_spinbox.value()
        # Format for all slaves: SPEED;value
        speed_cmd = f"SPEED;{speed_value}"
        self.global_command.emit(speed_cmd)

    def create_sequence_from_inputs(self):
        """Create sequence strings for each axis based on current inputs"""
        sequences = {}

        for axis, steps in self.axis_inputs.items():
            sequence_parts = []

            for step in steps:
                if step['delay_check'].isChecked():
                    delay_val = step['delay_value'].value()
                    sequence_parts.append(f"d{delay_val}")

                if step['step_check'].isChecked():
                    value = step['step_value'].value()
                    sequence_parts.append(str(value))

            if sequence_parts:
                # Create sequence string: "x(100,d500,200)" or "z(d500)"
                sequences[axis] = f"{axis}({','.join(sequence_parts)})"

        return sequences

    def add_new_row(self):
        """Add current inputs as a new row"""
        sequences = self.create_sequence_from_inputs()

        if not sequences:
            QMessageBox.warning(self, "Empty Sequence", "No sequence steps have been checked.")
            return

        # Add to rows list
        self.sequence_rows.append(sequences)

        # Update UI
        self.update_row_list()
        self.update_row_selector()
        self.clear_form()

        # Show success message
        QMessageBox.information(self, "Row Added", f"Row {len(self.sequence_rows)} has been added.")

    def update_selected_row(self):
        """Update the selected row with current inputs"""
        if self.selected_row_index < 0:
            return

        sequences = self.create_sequence_from_inputs()

        if not sequences:
            QMessageBox.warning(self, "Empty Sequence", "No sequence steps have been checked.")
            return

        # Update row
        self.sequence_rows[self.selected_row_index] = sequences

        # Update UI
        self.update_row_list()
        self.update_command_preview()

        # Show success message
        QMessageBox.information(self, "Row Updated", f"Row {self.selected_row_index + 1} has been updated.")

    def clear_form(self):
        """Clear all inputs in the sequence builder"""
        for axis, steps in self.axis_inputs.items():
            for step in steps:
                step['step_check'].setChecked(False)
                step['step_value'].setValue(0)
                step['delay_check'].setChecked(False)
                step['delay_value'].setValue(500)

        # Reset selected row
        self.selected_row_index = -1
        self.update_row_btn.setEnabled(False)

    def update_row_list(self):
        """Update the row list display with current rows"""
        text = ""

        for i, row in enumerate(self.sequence_rows):
            row_text = f"Row {i+1}: "
            row_parts = []

            for axis, sequence in row.items():
                row_parts.append(sequence)

            row_text += ", ".join(row_parts)
            text += row_text + "\n"

        self.row_list.setPlainText(text)
        self.update_command_preview()

    def update_row_selector(self):
        """Update the row selector dropdown"""
        self.row_selector.clear()
        self.row_selector.addItem("-- None --")

        for i in range(len(self.sequence_rows)):
            self.row_selector.addItem(f"Row {i+1}")

    def on_row_selected(self, index):
        """Handle row selection from dropdown"""
        if index == 0:  # None
            self.selected_row_index = -1
            self.edit_row_btn.setEnabled(False)
            self.delete_row_btn.setEnabled(False)
        else:
            self.selected_row_index = index - 1
            self.edit_row_btn.setEnabled(True)
            self.delete_row_btn.setEnabled(True)

            # Update command preview for selected row
            self.update_command_preview()

    def edit_selected_row(self):
        """Load the selected row into the editor"""
        if self.selected_row_index < 0:
            return

        # Clear form first
        self.clear_form()

        # Load row data
        row = self.sequence_rows[self.selected_row_index]

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

        # Enable update button
        self.update_row_btn.setEnabled(True)

        # Switch to first tab
        self.axis_tabs.setCurrentIndex(0)

    def delete_selected_row(self):
        """Delete the selected row"""
        if self.selected_row_index < 0:
            return

        # Confirm deletion
        reply = QMessageBox.question(
            self,
            "Delete Row",
            f"Are you sure you want to delete Row {self.selected_row_index + 1}?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )

        if reply == QMessageBox.Yes:
            # Delete row
            del self.sequence_rows[self.selected_row_index]

            # Update UI
            self.update_row_list()
            self.update_row_selector()

            # Reset selection
            self.selected_row_index = -1
            self.row_selector.setCurrentIndex(0)

    def clear_all_rows(self):
        """Clear all rows"""
        if not self.sequence_rows:
            return

        # Confirm deletion
        reply = QMessageBox.question(
            self,
            "Clear All Rows",
            "Are you sure you want to delete all rows?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )

        if reply == QMessageBox.Yes:
            # Clear rows
            self.sequence_rows = []

            # Update UI
            self.update_row_list()
            self.update_row_selector()

            # Reset selection
            self.selected_row_index = -1

    def update_command_preview(self):
        """Update the command preview based on selected row or all rows"""
        if self.selected_row_index >= 0:
            # Show preview for selected row
            row = self.sequence_rows[self.selected_row_index]
            cmd_parts = []

            for axis, sequence in row.items():
                cmd_parts.append(sequence)

            self.command_preview.setPlainText("Selected Row Command:\n" + ", ".join(cmd_parts))
        else:
            # Show preview for all rows
            if not self.sequence_rows:
                self.command_preview.clear()
                return

            preview = "All Rows Commands:\n\n"

            for i, row in enumerate(self.sequence_rows):
                cmd_parts = []

                for axis, sequence in row.items():
                    cmd_parts.append(sequence)

                preview += f"Row {i+1}:\n" + ", ".join(cmd_parts) + "\n\n"

            self.command_preview.setPlainText(preview)

    def run_next_row(self):
        """Run the next row in the sequence"""
        if not self.sequence_rows:
            QMessageBox.warning(self, "No Rows", "No sequence rows to run.")
            return

        if self.current_sequence_index >= len(self.sequence_rows) - 1:
            # We're at the end of the sequence
            QMessageBox.information(self, "Sequence Complete", "All rows have been executed.")
            self.sequence_execution_active = False
            self.current_sequence_index = -1
            self.next_btn.setEnabled(False)
            return

        # Move to the next row
        self.current_sequence_index += 1
        row_index = self.current_sequence_index

        # Update the row selector to show the current row
        self.row_selector.setCurrentIndex(row_index + 1)  # +1 because index 0 is "-- None --"

        # Run the current row
        row = self.sequence_rows[row_index]
        cmd_parts = []

        for axis, sequence in row.items():
            cmd_parts.append(sequence)

        # Only send START command if this is the first row or after a reset
        if row_index == 0 or not self.sequence_execution_active:
            self.global_command.emit(CMD_START)
            self.sequence_execution_active = True

        # Send row command
        full_command = ", ".join(cmd_parts)
        self.sequence_command.emit(full_command)

        # Disable the Next button until we receive completion feedback
        self.next_btn.setEnabled(False)

        # Show status message
        QMessageBox.information(self, "Running Sequence", f"Running Row {row_index + 1}")

    # Handle slave completion feedback
    def handle_slave_completion(self):
        """Called when all slaves have completed their tasks"""
        # Re-enable the Next button if we're in sequence execution mode
        if self.sequence_execution_active:
            self.next_btn.setEnabled(True)

            # Get the parent window to access statusBar
            parent_window = self.window()
            if parent_window and hasattr(parent_window, 'statusBar'):
                parent_window.statusBar().showMessage("Sequence step completed. Ready for next row.")

    def run_all_rows(self):
        """Start running the sequence row by row"""
        if not self.sequence_rows:
            QMessageBox.warning(self, "No Rows", "No sequence rows to run.")
            return

        # Confirm run
        reply = QMessageBox.question(
            self,
            "Run All Rows",
            f"Are you sure you want to run all {len(self.sequence_rows)} rows in sequence?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )

        if reply == QMessageBox.Yes:
            # Reset sequence execution state
            self.current_sequence_index = -1
            self.sequence_execution_active = False

            # Start with the first row
            self.run_next_row()

    def run_selected_row(self):
        """Run only the selected row"""
        if self.selected_row_index < 0:
            QMessageBox.warning(self, "No Selection", "No row selected to run.")
            return

        # Get selected row
        row = self.sequence_rows[self.selected_row_index]
        cmd_parts = []

        for axis, sequence in row.items():
            cmd_parts.append(sequence)

        # Send START command first
        self.global_command.emit(CMD_START)

        # Then send row command
        full_command = ", ".join(cmd_parts)
        self.sequence_command.emit(full_command)

        # Show status message
        QMessageBox.information(self, "Running Sequence", f"Running Row {self.selected_row_index + 1}")

    def run_single_axis(self, axis):
        """Run only a specific axis from the selected row"""
        if self.selected_row_index < 0:
            QMessageBox.warning(self, "No Selection", "No row selected to run a single axis from.")
            return

        # Get selected row
        row = self.sequence_rows[self.selected_row_index]

        if axis not in row:
            QMessageBox.warning(self, "No Axis", f"Selected row does not contain a sequence for axis {axis.upper()}.")
            return

        # Send START command first
        self.global_command.emit(CMD_START)

        # Then send axis command
        self.sequence_command.emit(row[axis])

        # Show status message
        QMessageBox.information(self, "Running Sequence", f"Running Axis {axis.upper()} from Row {self.selected_row_index + 1}")

    # ========== Methods for file operations ==========

    def on_new_sequence(self):
        """Create a new, empty sequence"""
        if self.sequence_rows:
            # Ask if user wants to save current sequence
            reply = QMessageBox.question(
                self,
                "Save Current Sequence?",
                "Do you want to save the current sequence before creating a new one?",
                QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                QMessageBox.Yes
            )

            if reply == QMessageBox.Cancel:
                return

            if reply == QMessageBox.Yes:
                self.on_save_sequence()

        # Create new sequence
        name, ok = QInputDialog.getText(
            self, "New Sequence", "Enter name for new sequence:",
            text="New Sequence"
        )

        if ok and name:
            self.sequence_rows = []
            self.current_sequence_name = name
            self.sequence_name_label.setText(name)
            self.update_row_list()
            self.update_row_selector()
            self.selected_row_index = -1

            # Add to saved sequences
            self.saved_sequences[name] = []
            self.update_saved_sequences_list()

    def on_save_sequence(self):
        """Save the current sequence to the saved sequences list"""
        if not self.sequence_rows:
            QMessageBox.warning(self, "Empty Sequence", "No sequence rows to save.")
            return

        # Save to current sequence name
        self.saved_sequences[self.current_sequence_name] = self.sequence_rows.copy()

        # Update the list
        self.update_saved_sequences_list()

        # Offer to save to file
        reply = QMessageBox.question(
            self,
            "Save to File?",
            "Do you want to save this sequence to a file?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.Yes
        )

        if reply == QMessageBox.Yes:
            self.save_sequence_to_file()
        else:
            QMessageBox.information(self, "Sequence Saved", f"Sequence '{self.current_sequence_name}' saved in memory.")

    def on_save_as_sequence(self):
        """Save the current sequence with a new name"""
        if not self.sequence_rows:
            QMessageBox.warning(self, "Empty Sequence", "No sequence rows to save.")
            return

        # Ask for new name
        name, ok = QInputDialog.getText(
            self, "Save Sequence As", "Enter name for sequence:",
            text=self.current_sequence_name + "_copy"
        )

        if ok and name:
            self.current_sequence_name = name
            self.sequence_name_label.setText(name)

            # Save with new name
            self.saved_sequences[name] = self.sequence_rows.copy()
            self.update_saved_sequences_list()

            # Save to file
            self.save_sequence_to_file()

    def on_load_sequence(self):
        """Load a sequence from a file"""
        # Ask if user wants to save current sequence
        if self.sequence_rows:
            reply = QMessageBox.question(
                self,
                "Save Current Sequence?",
                "Do you want to save the current sequence before loading a new one?",
                QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                QMessageBox.Yes
            )

            if reply == QMessageBox.Cancel:
                return

            if reply == QMessageBox.Yes:
                self.on_save_sequence()

        # Show file dialog to select a YAML file
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Load Sequence", "", "YAML Files (*.yaml *.yml);;All Files (*)"
        )

        if file_path:
            self.load_sequence_from_file(file_path)

    def on_sequence_selected(self, item):
        """Handle selection of a saved sequence from the list"""
        name = item.text()

        # Ask if user wants to save current sequence
        if self.sequence_rows and name != self.current_sequence_name:
            reply = QMessageBox.question(
                self,
                "Save Current Sequence?",
                f"Do you want to save the current sequence '{self.current_sequence_name}' before loading '{name}'?",
                QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                QMessageBox.Yes
            )

            if reply == QMessageBox.Cancel:
                return

            if reply == QMessageBox.Yes:
                # Save current sequence
                self.saved_sequences[self.current_sequence_name] = self.sequence_rows.copy()

        # Load selected sequence
        self.sequence_rows = self.saved_sequences[name].copy()
        self.current_sequence_name = name
        self.sequence_name_label.setText(name)

        # Update UI
        self.update_row_list()
        self.update_row_selector()
        self.selected_row_index = -1

    def save_sequence_to_file(self):
        """Save the current sequence to a YAML file"""
        suggested_filename = self.current_sequence_name.replace(" ", "_") + ".yaml"

        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save Sequence", suggested_filename, "YAML Files (*.yaml *.yml);;All Files (*)"
        )

        if file_path:
            try:
                # Prepare data for YAML
                sequence_data = {
                    'name': self.current_sequence_name,
                    'rows': []
                }

                for row in self.sequence_rows:
                    sequence_data['rows'].append(row)

                # Write to file
                with open(file_path, 'w') as f:
                    yaml.dump(sequence_data, f, default_flow_style=False)

                QMessageBox.information(self, "Save Successful", f"Sequence saved to {file_path}")
            except Exception as e:
                QMessageBox.critical(self, "Save Failed", f"Failed to save sequence: {str(e)}")

    def load_sequence_from_file(self, file_path):
        """Load a sequence from a YAML file"""
        try:
            # Read YAML file
            with open(file_path, 'r') as f:
                sequence_data = yaml.safe_load(f)

            # Extract data
            name = sequence_data.get('name', os.path.basename(file_path).split('.')[0])
            rows = sequence_data.get('rows', [])

            # Update current sequence
            self.sequence_rows = rows
            self.current_sequence_name = name
            self.sequence_name_label.setText(name)

            # Add to saved sequences if not already there
            if name not in self.saved_sequences:
                self.saved_sequences[name] = rows.copy()
                self.update_saved_sequences_list()

            # Update UI
            self.update_row_list()
            self.update_row_selector()
            self.selected_row_index = -1

            QMessageBox.information(self, "Load Successful", f"Loaded sequence '{name}' from {file_path}")
        except Exception as e:
            QMessageBox.critical(self, "Load Failed", f"Failed to load sequence: {str(e)}")

    def update_saved_sequences_list(self):
        """Update the list of saved sequences"""
        self.saved_sequences_list.clear()
        for name in sorted(self.saved_sequences.keys()):
            self.saved_sequences_list.addItem(name)

            # Select the current sequence
            if name == self.current_sequence_name:
                self.saved_sequences_list.setCurrentRow(self.saved_sequences_list.count() - 1)
