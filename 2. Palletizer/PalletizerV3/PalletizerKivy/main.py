from kivy.app import App
from kivy.core.window import Window
from palletizer.ui.main_window import PalletizerControlApp


class PalletizerApp(App):
    def build(self):
        # Set ukuran window default
        Window.size = (1280, 720)
        # Inisialisasi main window
        return PalletizerControlApp()


if __name__ == '__main__':
    PalletizerApp().run()
