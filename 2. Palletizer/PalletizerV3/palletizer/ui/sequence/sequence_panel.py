from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout,
                             QLabel, QPushButton, QGroupBox, QLineEdit, QTextEdit,
                             QSpinBox, QCheckBox, QTabWidget, QComboBox, QSplitter,
                             QScrollArea, QFrame, QSizePolicy, QListWidget, QMessageBox)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont

from .sequence_row_manager import SequenceRowManager
from .sequence_executor import SequenceExecutor
from .sequence_file_operations import SequenceFileManager
from ...utils.config import *


class SequencePanel(QWidget):
    """Panel untuk mengatur dan menjalankan sekuens gerakan dengan sistem row dan sequence"""
    sequence_command = pyqtSignal(str)
    global_command = pyqtSignal(str)
    speed_command = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

        # Create helper instances
        self.row_manager = SequenceRowManager(self)
        self.sequence_executor = SequenceExecutor(self)
        self.file_manager = SequenceFileManager(self)

        # Connect helper instances
        self.sequence_executor.set_row_manager(self.row_manager)
        self.file_manager.set_row_manager(self.row_manager)

        # Forward signals
        self.sequence_executor.global_command.connect(self.global_command)
        self.sequence_executor.sequence_command.connect(self.sequence_command)

        # Connect internal signals
        self.row_manager.row_updated.connect(self.on_rows_updated)
        self.file_manager.sequence_updated.connect(self.on_sequence_name_updated)
        self.file_manager.sequences_list_updated.connect(self.update_saved_sequences_list)
        self.sequence_executor.execution_state_changed.connect(self.on_execution_state_changed)

        # Position labels
        self.position_labels = {}

        # Setup UI
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
        self.sequence_name_label = QLabel(self.file_manager.current_sequence_name)
        self.sequence_name_label.setStyleSheet("font-weight: bold;")
        sequence_name_layout.addWidget(self.sequence_name_label)
        sequence_name_layout.addStretch()

        # Saved sequences selection
        self.saved_sequences_list = QListWidget()
        self.saved_sequences_list.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self.saved_sequences_list.setMinimumHeight(80)  # Give it some minimum height
        self.saved_sequences_list.itemDoubleClicked.connect(self.on_sequence_list_double_clicked)

        # File operation buttons
        file_buttons_layout = QHBoxLayout()
        file_buttons_layout.setSpacing(5)  # Tighter spacing

        self.new_sequence_btn = QPushButton("New Sequence")
        self.new_sequence_btn.clicked.connect(self.file_manager.on_new_sequence)
        self.new_sequence_btn.setStyleSheet(BUTTON_SPEED)

        self.save_sequence_btn = QPushButton("Save Sequence")
        self.save_sequence_btn.clicked.connect(self.file_manager.on_save_sequence)
        self.save_sequence_btn.setStyleSheet("background-color: #e0ffea;")

        self.save_as_sequence_btn = QPushButton("Save As...")
        self.save_as_sequence_btn.clicked.connect(self.file_manager.on_save_as_sequence)
        self.save_as_sequence_btn.setStyleSheet("background-color: #e0ffea;")

        self.load_sequence_btn = QPushButton("Load From File")
        self.load_sequence_btn.clicked.connect(self.file_manager.on_load_sequence)
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
        self.run_all_btn.clicked.connect(self.sequence_executor.run_all_rows)
        self.run_all_btn.setStyleSheet(BUTTON_START)

        self.run_selected_btn = QPushButton("Run Selected Row")
        self.run_selected_btn.clicked.connect(self.run_selected_row)
        self.run_selected_btn.setStyleSheet("background-color: #ccffdd;")

        # Add a Next button for step-by-step execution
        self.next_btn = QPushButton("Next Row")
        self.next_btn.clicked.connect(self.sequence_executor.run_next_row)
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
        axis_inputs = {}

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
                # Step checkbox and value
                step_check = QCheckBox(f"Step {i+1}:")
                step_value = QSpinBox()
                step_value.setRange(-MAX_STEPS*100, MAX_STEPS*100)  # Wider range for absolute positioning
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
            axis_inputs[axis.lower()] = step_inputs

        # Set the axis inputs in the row manager
        self.row_manager.set_axis_inputs(axis_inputs)

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
        self.add_row_btn.clicked.connect(self.row_manager.add_new_row)
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

        # ======== Current Positions Below Command Preview ========
        # Create compact positions display layout
        positions_layout = QHBoxLayout()
        positions_layout.setSpacing(10)

        # Create position labels for each axis
        for slave_id in SLAVE_IDS:
            # Create frame for axis label and position
            axis_frame = QFrame()
            axis_frame.setFrameShape(QFrame.StyledPanel)
            axis_frame.setStyleSheet("background-color: #f0f0f0; border-radius: 3px;")

            axis_frame_layout = QHBoxLayout(axis_frame)
            axis_frame_layout.setContentsMargins(5, 2, 5, 2)
            axis_frame_layout.setSpacing(5)

            # Axis label
            axis_label = QLabel(f"{slave_id.upper()}")
            axis_label.setAlignment(Qt.AlignCenter)
            axis_label.setStyleSheet("font-weight: bold;")

            # Position value label
            position_label = QLabel("0")
            position_label.setAlignment(Qt.AlignCenter)
            position_label.setStyleSheet("color: #006400; background-color: #ffffff; padding: 2px; border: 1px solid #cccccc;")
            position_label.setFont(QFont("Monospace", 10, QFont.Bold))
            position_label.setMinimumWidth(50)

            # Store reference to position label
            self.position_labels[slave_id.lower()] = position_label

            # Add to layout
            axis_frame_layout.addWidget(axis_label)
            axis_frame_layout.addWidget(position_label)

            # Add to positions layout
            positions_layout.addWidget(axis_frame)

        # Add stretch to ensure proper spacing
        positions_layout.addStretch()

        # Add positions layout to preview layout
        preview_layout.addLayout(positions_layout)

        preview_group.setLayout(preview_layout)

        right_layout.addWidget(selection_group)
        right_layout.addWidget(preview_group)

        # Add both panels to the splitter
        sequence_splitter.addWidget(left_panel)
        sequence_splitter.addWidget(right_panel)

        # Set initial sizes for the splitter (approximately 60% / 40%)
        sequence_splitter.setSizes([600, 400])

        main_layout.addWidget(sequence_splitter, 1)  # Give more space to sequence container

        # Initialize the saved sequences list
        self.update_saved_sequences_list()

    # ========== Methods for Position Handling ==========

    def update_position(self, axis_id, position):
        """Update the position display for a specific axis"""
        if axis_id.lower() in self.position_labels:
            self.position_labels[axis_id.lower()].setText(str(position))

            # Briefly highlight the background to indicate change
            current_style = self.position_labels[axis_id.lower()].styleSheet()
            self.position_labels[axis_id.lower()].setStyleSheet("color: #006400; background-color: #e0ffe0; padding: 2px; border: 1px solid #cccccc;")

            # Use a single shot timer to revert the style after a short delay
            from PyQt5.QtCore import QTimer
            QTimer.singleShot(300, lambda: self.position_labels[axis_id.lower()].setStyleSheet(current_style))

    def reset_all_positions(self):
        """Reset all position displays to zero"""
        for axis_id in self.position_labels:
            self.position_labels[axis_id].setText("0")

    # ======== Helper methods (mostly delegation to component classes) ========

    def on_set_global_speed(self):
        """Handle global speed setting"""
        speed_value = self.global_speed_spinbox.value()
        # Format for all slaves: SPEED;value
        speed_cmd = f"SPEED;{speed_value}"
        self.global_command.emit(speed_cmd)

    def clear_form(self):
        """Clear all inputs in the form and reset selection"""
        for axis, steps in self.row_manager.axis_inputs.items():
            for step in steps:
                step['step_check'].setChecked(False)
                step['step_value'].setValue(0)
                step['delay_check'].setChecked(False)
                step['delay_value'].setValue(500)

        # Reset selected row
        self.row_manager.selected_row_index = -1
        self.update_row_btn.setEnabled(False)

    def clear_all_rows(self):
        """Clear all rows after confirmation"""
        if not self.row_manager.sequence_rows:
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
            self.row_manager.clear_all_rows()
            self.update_ui_after_row_change()

    def on_rows_updated(self):
        """Handle row updates"""
        self.update_ui_after_row_change()

    def update_ui_after_row_change(self):
        """Update UI elements after rows have changed"""
        # Update row list text
        self.row_list.setPlainText(self.row_manager.get_row_text_representation())

        # Update row selector
        self.update_row_selector()

        # Update command preview
        self.update_command_preview()

    def update_row_selector(self):
        """Update the row selector dropdown"""
        # Store current index to restore selection if possible
        current_index = self.row_selector.currentIndex()
        current_text = self.row_selector.currentText() if current_index > 0 else ""

        # Clear and repopulate
        self.row_selector.clear()
        self.row_selector.addItem("-- None --")

        for i in range(len(self.row_manager.sequence_rows)):
            self.row_selector.addItem(f"Row {i+1}")

        # Try to restore previous selection if it still exists
        if current_text:
            index = self.row_selector.findText(current_text)
            if index >= 0:
                self.row_selector.setCurrentIndex(index)
                # Manually set selected_row_index since the signal might not fire
                if index > 0:
                    self.row_manager.selected_row_index = index - 1
                    self.edit_row_btn.setEnabled(True)
                    self.delete_row_btn.setEnabled(True)
            else:
                self.row_selector.setCurrentIndex(0)
                self.row_manager.selected_row_index = -1
                self.edit_row_btn.setEnabled(False)
                self.delete_row_btn.setEnabled(False)

    def on_row_selected(self, index):
        """Handle row selection from dropdown"""
        if index == 0:  # None
            self.row_manager.selected_row_index = -1
            self.edit_row_btn.setEnabled(False)
            self.delete_row_btn.setEnabled(False)
        else:
            # Calculate new index (adjust for 0-based indexing)
            new_index = index - 1

            # Validate that the index is within range
            if new_index >= 0 and new_index < len(self.row_manager.sequence_rows):
                self.row_manager.selected_row_index = new_index
                self.edit_row_btn.setEnabled(True)
                self.delete_row_btn.setEnabled(True)
            else:
                # If out of range, set to -1 and disable buttons
                self.row_manager.selected_row_index = -1
                self.edit_row_btn.setEnabled(False)
                self.delete_row_btn.setEnabled(False)
                # Set dropdown back to "None"
                self.row_selector.setCurrentIndex(0)
                return

        # Update command preview for selected row
        self.update_command_preview()

    def edit_selected_row(self):
        """Load the selected row into the editor"""
        # Get the current index from the dropdown directly
        dropdown_index = self.row_selector.currentIndex()

        if dropdown_index <= 0:  # None selected
            QMessageBox.warning(self, "No Selection", "No row selected to edit.")
            return

        row_index = dropdown_index - 1

        # Verify the row exists in our data
        if row_index >= len(self.row_manager.sequence_rows):
            QMessageBox.warning(self, "Invalid Selection", f"Row {row_index + 1} does not exist.")
            return

        # Clear form first
        self.clear_form()

        # Load row data
        if self.row_manager.load_row_data_to_form(row_index):
            # Enable update button
            self.update_row_btn.setEnabled(True)

            # Switch to first tab
            self.axis_tabs.setCurrentIndex(0)

    def update_selected_row(self):
        """Update the selected row with current inputs"""
        # Get the current index from the dropdown directly
        dropdown_index = self.row_selector.currentIndex()

        if dropdown_index <= 0:  # None selected
            QMessageBox.warning(self, "Invalid Selection", "No valid row selected to update.")
            return

        row_index = dropdown_index - 1
        self.row_manager.update_selected_row(row_index)

    def delete_selected_row(self):
        """Delete the selected row"""
        # Get the current index from the dropdown directly
        dropdown_index = self.row_selector.currentIndex()

        if dropdown_index <= 0:  # None selected
            QMessageBox.warning(self, "No Selection", "No row selected to delete.")
            return

        row_index = dropdown_index - 1

        # Confirm deletion
        reply = QMessageBox.question(
            self,
            "Delete Row",
            f"Are you sure you want to delete Row {row_index + 1}?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No
        )

        if reply == QMessageBox.Yes:
            # Delete row
            temp_index = row_index
            if self.row_manager.delete_row(row_index):
                # Set dropdown to appropriate index
                if len(self.row_manager.sequence_rows) > 0 and temp_index < len(self.row_manager.sequence_rows):
                    # Select the same position if possible
                    self.row_selector.setCurrentIndex(temp_index + 1)
                else:
                    # Otherwise select None
                    self.row_selector.setCurrentIndex(0)

    def update_command_preview(self):
        """Update the command preview based on selected row or all rows"""
        dropdown_index = self.row_selector.currentIndex()

        if dropdown_index > 0:
            # Show preview for selected row
            row_index = dropdown_index - 1
            preview_text = self.row_manager.get_command_preview(row_index)
            self.command_preview.setPlainText(preview_text)
        else:
            # Show preview for all rows
            preview_text = self.row_manager.get_command_preview()
            self.command_preview.setPlainText(preview_text)

    def run_selected_row(self):
        """Run the selected row"""
        # Get the current index from the dropdown directly
        dropdown_index = self.row_selector.currentIndex()

        if dropdown_index <= 0:  # None selected
            QMessageBox.warning(self, "No Selection", "No row selected to run.")
            return

        row_index = dropdown_index - 1
        self.sequence_executor.run_selected_row(row_index)

    def run_single_axis(self, axis):
        """Run a single axis from the selected row"""
        # Get the current index from the dropdown directly
        dropdown_index = self.row_selector.currentIndex()

        if dropdown_index <= 0:  # None selected
            QMessageBox.warning(self, "No Selection", "No row selected to run a single axis from.")
            return

        row_index = dropdown_index - 1
        self.sequence_executor.run_single_axis(row_index, axis)

    def on_sequence_list_double_clicked(self, item):
        """Handle double-click on a sequence in the list"""
        sequence_name = item.text()
        self.file_manager.on_sequence_selected(sequence_name)
        self.update_ui_after_row_change()

    def update_saved_sequences_list(self):
        """Update the saved sequences list"""
        self.saved_sequences_list.clear()
        for name in self.file_manager.get_saved_sequence_names():
            self.saved_sequences_list.addItem(name)

            # Select the current sequence
            if name == self.file_manager.current_sequence_name:
                self.saved_sequences_list.setCurrentRow(self.saved_sequences_list.count() - 1)

    def on_sequence_name_updated(self, name):
        """Handle sequence name updates"""
        self.sequence_name_label.setText(name)

    def on_execution_state_changed(self, is_executing):
        """Handle execution state changes"""
        self.next_btn.setEnabled(is_executing)

    def handle_slave_completion(self):
        """Called when all slaves have completed their tasks"""
        self.sequence_executor.handle_slave_completion()
