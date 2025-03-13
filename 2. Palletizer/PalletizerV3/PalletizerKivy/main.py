import sys
from PyQt5.QtWidgets import QApplication
from palletizer.ui.main_window import PalletizerControlApp


def main():
    """Main entry point for the Palletizer Control application"""
    app = QApplication(sys.argv)
    window = PalletizerControlApp()
    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
