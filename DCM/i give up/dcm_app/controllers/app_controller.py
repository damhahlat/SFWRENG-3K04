from __future__ import annotations

import tkinter as tk
from tkinter import messagebox

from ..views.welcome_view import WelcomeView
from ..views.main_view import MainView
from ..services.user_storage import UserStorage
from ..services.parameter_storage import ParameterStorage
from ..services.serial_manager import SerialManager
from ..services.validation import validate_parameters
from ..models.parameters import ProgrammableParameters
from ..models.egram import EgramPoint

# Class to control the entire DCM from a centralized point
class AppController:

    def __init__(self, root: tk.Tk) -> None:
        self.root = root

        # Services
        self.user_storage = UserStorage()
        self.parameter_storage = ParameterStorage()
        self.serial_manager = SerialManager()

        # State
        self.current_user: str | None = None
        self.current_parameters = ProgrammableParameters()

        # Views
        self._welcome_view: WelcomeView | None = None
        self._main_view: MainView | None = None

        # Egram callback
        self._ui_egram_callback: callable | None = None

    # For switching views
    def _clear_root(self) -> None:
        for child in self.root.winfo_children():
            child.destroy()

    def show_welcome(self) -> None:
        self._clear_root()
        self._welcome_view = WelcomeView(self.root, controller=self)
        self._welcome_view.pack(fill="both", expand=True)

    def show_main(self) -> None:
        self._clear_root()
        self._main_view = MainView(self.root, controller=self)
        self._main_view.pack(fill="both", expand=True)
        self._main_view.update_user_label(self.current_user)

        mode = self.current_parameters.mode

        if self.current_user is not None:
            params = self.parameter_storage.load_parameters(self.current_user, mode)
            if params is None:
                params = ProgrammableParameters(mode=mode)
                self.parameter_storage.save_parameters(self.current_user, params)
            self.current_parameters = params
            self._main_view.set_parameters(self.current_parameters)

        self._main_view.update_mode_label(mode)
        self._main_view.on_mode_changed(mode)

    # Managing users
    def register_user(self, username: str, password: str) -> None:
        if not username or not password:
            messagebox.showerror("Error", "Username and password are required.")
            return

        success, msg = self.user_storage.register_user(username, password)
        if not success:
            messagebox.showerror("Error", msg)
            return

        messagebox.showinfo("Success", "User registered. You can now log in.")

    def login_user(self, username: str, password: str) -> None:
        if not self.user_storage.validate_credentials(username, password):
            messagebox.showerror("Login failed", "Invalid username or password.")
            return

        self.current_user = username
        self.current_parameters = ProgrammableParameters()
        self.show_main()

    def logout(self) -> None:
        self.stop_egram_stream()
        self.serial_manager.disconnect()

        self.current_user = None
        self.current_parameters = ProgrammableParameters()
        self.show_welcome()

    # For connecting to the pacemaker
    def list_serial_ports(self):
        return self.serial_manager.list_ports()

    def connect_device(self, port: str) -> None:
        if not port:
            messagebox.showerror("Error", "Please select a port.")
            return

        if self.serial_manager.connect(port):
            if self._main_view:
                self._main_view.update_connection_status(True, streaming=False)
            messagebox.showinfo("Connected", f"Connected to device on {port}.")
        else:
            messagebox.showerror("Error", f"Failed to connect to {port}.")

    def disconnect_device(self) -> None:
        self.stop_egram_stream()
        self.serial_manager.disconnect()
        if self._main_view:
            self._main_view.update_connection_status(False, streaming=False)

    # To handle modes and parameters
    def set_mode(self, mode: str) -> None:
        self.current_parameters.mode = mode

        if self.current_user is not None:
            params = self.parameter_storage.load_parameters(self.current_user, mode)
            if params is None:
                params = ProgrammableParameters(mode=mode)
                self.parameter_storage.save_parameters(self.current_user, params)
            self.current_parameters = params

        if self._main_view:
            self._main_view.update_mode_label(mode)
            self._main_view.set_parameters(self.current_parameters)
            self._main_view.on_mode_changed(mode)

    def reload_current_mode_parameters(self) -> ProgrammableParameters | None:
        if self.current_user is None:
            return None

        mode = self.current_parameters.mode
        params = self.parameter_storage.load_parameters(self.current_user, mode)
        if params is None:
            params = ProgrammableParameters(mode=mode)
            self.parameter_storage.save_parameters(self.current_user, params)

        self.current_parameters = params

        if self._main_view:
            self._main_view.set_parameters(self.current_parameters)
            self._main_view.on_mode_changed(mode)

        return params

    def save_parameters_locally(self, params: ProgrammableParameters) -> tuple[bool, str]:
        ok, msg = validate_parameters(params)
        if not ok:
            return False, msg

        self.current_parameters = params

        if self.current_user is not None:
            self.parameter_storage.save_parameters(self.current_user, params)

        return True, "Mode profile saved locally for this user."

    def validate_and_save_parameters(self, params: ProgrammableParameters) -> tuple[bool, str]:
        ok, msg = validate_parameters(params)
        if not ok:
            return False, msg

        self.current_parameters = params

        if self.current_user is not None:
            self.parameter_storage.save_parameters(self.current_user, params)

        if not self.serial_manager.is_connected():
            return True, "Parameters saved locally. Device is not connected."

        echo_ok = self.serial_manager.send_and_verify_parameters(params)

        if echo_ok:
            if self.serial_manager.uses_mock_mode():
                return True, "Parameters saved. Mock device echo verified."
            return True, "Parameters saved and verified with device echo."
        else:
            if self.serial_manager.uses_mock_mode():
                return True, "Parameters saved. Mock echo failed (check SerialManager)."
            return True, (
                "Parameters saved, but device echo failed or did not match. "
                "Check the Simulink Tx block, UART wiring, and frame layout."
            )

    def apply_parameters(self, params: ProgrammableParameters) -> None:
        ok, msg = self.validate_and_save_parameters(params)
        if ok:
            messagebox.showinfo("Parameters", msg)
        else:
            messagebox.showerror("Invalid parameters", msg)

    # For viewing the egram
    def start_egram_stream(self, ui_callback) -> None:
        if not self.serial_manager.is_connected():
            messagebox.showerror("Error", "Connect to a device before starting egrams.")
            return

        self._ui_egram_callback = ui_callback

        def _thread_callback(point: EgramPoint) -> None:
            if self.root and self._ui_egram_callback:
                self.root.after(0, lambda p=point: self._ui_egram_callback(p))

        self.serial_manager.start_egram_stream(_thread_callback)

        if self._main_view:
            self._main_view.update_connection_status(True, streaming=True)

    def stop_egram_stream(self) -> None:
        self.serial_manager.stop_egram_stream()
        self._ui_egram_callback = None
        if self._main_view:
            self._main_view.update_connection_status(self.serial_manager.is_connected(), streaming=False)
