from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGroupBox,
                             QLabel, QSizePolicy, QFrame)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont
from ..utils.config import SLAVE_IDS, STATUS_IDLE, STATUS_MOVING


class PositionDisplayWidget(QWidget):
    """Widget to display the current position of all axes"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.position_labels = {}  # Dictionary to store position labels
        self.setup_ui()

    def setup_ui(self):
        """Set up the UI components"""
        layout = QVBoxLayout(self)
        layout.setContentsMargins(5, 5, 5, 5)
        layout.setSpacing(5)

        # Create a group box to contain the position displays
        group_box = QGroupBox("Current Positions")
        group_box.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Minimum)

        # Create grid layout for the positions
        position_layout = QHBoxLayout()
        position_layout.setSpacing(10)

        # Create position displays for each axis
        for slave_id in SLAVE_IDS:
            # Create frame for each axis position
            axis_frame = QFrame()
            axis_frame.setFrameShape(QFrame.StyledPanel)
            axis_frame.setFrameShadow(QFrame.Raised)
            axis_frame.setStyleSheet("background-color: #f0f0f0; border-radius: 5px;")

            axis_layout = QVBoxLayout(axis_frame)
            axis_layout.setContentsMargins(10, 5, 10, 5)
            axis_layout.setSpacing(2)

            # Label for axis name
            axis_label = QLabel(f"{slave_id.upper()}")
            axis_label.setAlignment(Qt.AlignCenter)
            axis_label.setStyleSheet("font-weight: bold; color: #333333;")

            # Label for position value
            position_label = QLabel("0")
            position_label.setAlignment(Qt.AlignCenter)
            position_label.setFont(QFont("Monospace", 12, QFont.Bold))
            position_label.setStyleSheet("color: #006400; background-color: #ffffff; padding: 2px; border: 1px solid #cccccc;")
            position_label.setMinimumWidth(80)

            # Store reference to position label
            self.position_labels[slave_id.lower()] = position_label

            # Add to layout
            axis_layout.addWidget(axis_label)
            axis_layout.addWidget(position_label)

            # Add to main position layout
            position_layout.addWidget(axis_frame)

        # Add stretch to ensure proper spacing
        position_layout.addStretch()

        # Set the layout for the group box
        group_box.setLayout(position_layout)

        # Add the group box to the main layout
        layout.addWidget(group_box)

    def update_position(self, axis_id, position):
        """Update the position display for a specific axis"""
        if axis_id.lower() in self.position_labels:
            self.position_labels[axis_id.lower()].setText(str(position))

            # Briefly flash the background to indicate change
            current_style = self.position_labels[axis_id.lower()].styleSheet()
            self.position_labels[axis_id.lower()].setStyleSheet("color: #006400; background-color: #e0ffe0; padding: 2px; border: 1px solid #cccccc;")

            # Use a single shot timer to revert the style after a short delay
            from PyQt5.QtCore import QTimer
            QTimer.singleShot(300, lambda: self.position_labels[axis_id.lower()].setStyleSheet(current_style))

    def reset_all_positions(self):
        """Reset all position displays to zero"""
        for axis_id in self.position_labels:
            self.position_labels[axis_id].setText("0")

    def get_position(self, axis_id):
        """Get the current displayed position for an axis"""
        if axis_id.lower() in self.position_labels:
            try:
                return int(self.position_labels[axis_id.lower()].text())
            except ValueError:
                return 0
        return 0
