import tkinter as tk
from tkinter import ttk

from .base_frame import BaseFrame

# Login and registration screen
class WelcomeView(BaseFrame):

    def __init__(self, parent, controller=None, **kwargs):
        super().__init__(parent, controller=controller, **kwargs)

        self._build_ui()

    def _build_ui(self) -> None:
        title = ttk.Label(self, text="Pacemaker DCM", font=("Segoe UI", 20, "bold"))
        subtitle = ttk.Label(self, text="Login or register to continue", font=("Segoe UI", 11))

        title.grid(row=0, column=0, columnspan=2, pady=(0, 5), sticky="w")
        subtitle.grid(row=1, column=0, columnspan=2, pady=(0, 15), sticky="w")

        # Login section
        login_frame = ttk.LabelFrame(self, text="Login")
        login_frame.grid(row=2, column=0, padx=(0, 10), sticky="nsew")

        ttk.Label(login_frame, text="Username:").grid(row=0, column=0, sticky="e", padx=5, pady=5)
        ttk.Label(login_frame, text="Password:").grid(row=1, column=0, sticky="e", padx=5, pady=5)

        self.login_username = ttk.Entry(login_frame, width=20)
        self.login_password = ttk.Entry(login_frame, width=20, show="*")

        self.login_username.grid(row=0, column=1, sticky="w", padx=5, pady=5)
        self.login_password.grid(row=1, column=1, sticky="w", padx=5, pady=5)

        login_button = ttk.Button(login_frame, text="Login", command=self._on_login)
        login_button.grid(row=2, column=0, columnspan=2, pady=10)

        # Register section
        register_frame = ttk.LabelFrame(self, text="Register")
        register_frame.grid(row=2, column=1, sticky="nsew")

        ttk.Label(register_frame, text="New username:").grid(row=0, column=0, sticky="e", padx=5, pady=5)
        ttk.Label(register_frame, text="New password:").grid(row=1, column=0, sticky="e", padx=5, pady=5)

        self.register_username = ttk.Entry(register_frame, width=20)
        self.register_password = ttk.Entry(register_frame, width=20, show="*")

        self.register_username.grid(row=0, column=1, sticky="w", padx=5, pady=5)
        self.register_password.grid(row=1, column=1, sticky="w", padx=5, pady=5)

        register_button = ttk.Button(register_frame, text="Register", command=self._on_register)
        register_button.grid(row=2, column=0, columnspan=2, pady=10)

        # Make columns expand
        self.columnconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)
        login_frame.columnconfigure(1, weight=1)
        register_frame.columnconfigure(1, weight=1)

    def _on_login(self) -> None:
        username = self.login_username.get().strip()
        password = self.login_password.get().strip()
        if self.controller:
            self.controller.login_user(username, password)

    def _on_register(self) -> None:
        username = self.register_username.get().strip()
        password = self.register_password.get().strip()
        if self.controller:
            self.controller.register_user(username, password)
