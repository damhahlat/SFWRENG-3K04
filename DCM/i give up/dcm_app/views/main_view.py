import tkinter as tk
from tkinter import ttk

from .base_frame import BaseFrame
from .modes_tab import ModesTab
from .parameters_tab import ParametersTab
from .egram_tab import EgramTab
from .device_info_tab import DeviceInfoTab


class MainView(BaseFrame):
    """
    Main DCM window with toolbar, notebook tabs, and status bar.
    """

    def __init__(self, parent, controller=None, **kwargs):
        super().__init__(parent, controller=controller, **kwargs)
        self._status_user = None
        self._status_mode = None
        self._status_connection = None

        self._port_combo = None
        self._connect_button = None

        self.parameters_tab: ParametersTab | None = None
        self.modes_tab: ModesTab | None = None
        self.egram_tab: EgramTab | None = None

        self._build_ui()

    def _build_ui(self) -> None:
        # Menubar
        self._build_menubar()

        # Toolbar (serial connection)
        toolbar = ttk.Frame(self)
        toolbar.pack(side="top", fill="x")

        ttk.Label(toolbar, text="Serial port:").pack(side="left", padx=(0, 5))

        self._port_combo = ttk.Combobox(toolbar, state="readonly", width=20)
        self._port_combo.pack(side="left", padx=(0, 5))

        refresh_button = ttk.Button(toolbar, text="Refresh", command=self._on_refresh_ports)
        refresh_button.pack(side="left", padx=(0, 5))

        self._connect_button = ttk.Button(toolbar, text="Connect", command=self._on_connect_or_disconnect)
        self._connect_button.pack(side="left", padx=(0, 5))

        # Notebook
        notebook = ttk.Notebook(self)
        notebook.pack(side="top", fill="both", expand=True, pady=10)

        self.modes_tab = ModesTab(notebook, controller=self.controller)
        self.parameters_tab = ParametersTab(notebook, controller=self.controller)
        self.egram_tab = EgramTab(notebook, controller=self.controller)
        device_info_tab = DeviceInfoTab(notebook, controller=self.controller)

        notebook.add(self.modes_tab, text="Modes")
        notebook.add(self.parameters_tab, text="Parameters")
        notebook.add(self.egram_tab, text="Egrams")
        notebook.add(device_info_tab, text="Device Info")

        # Status bar
        status_bar = ttk.Frame(self, relief="sunken")
        status_bar.pack(side="bottom", fill="x")

        self._status_user = ttk.Label(status_bar, text="User: -")
        self._status_user.pack(side="left", padx=5)

        self._status_mode = ttk.Label(status_bar, text="Mode: -")
        self._status_mode.pack(side="left", padx=5)

        self._status_connection = ttk.Label(status_bar, text="DCM–Device: Disconnected")
        self._status_connection.pack(side="right", padx=5)

        # Initial port list
        self._refresh_ports()

    def _build_menubar(self) -> None:
        root = self.winfo_toplevel()
        menubar = tk.Menu(root)

        file_menu = tk.Menu(menubar, tearoff=0)
        file_menu.add_command(label="Logout", command=self._on_logout)
        file_menu.add_separator()
        file_menu.add_command(label="Quit", command=root.quit)
        menubar.add_cascade(label="File", menu=file_menu)

        device_menu = tk.Menu(menubar, tearoff=0)
        device_menu.add_command(label="Connect", command=self._on_connect_menu)
        device_menu.add_command(label="Disconnect", command=self._on_disconnect_menu)
        menubar.add_cascade(label="Device", menu=device_menu)

        help_menu = tk.Menu(menubar, tearoff=0)
        help_menu.add_command(label="About", command=self._on_about)
        menubar.add_cascade(label="Help", menu=help_menu)

        root.config(menu=menubar)

    # ===== Menubar actions =====

    def _on_logout(self) -> None:
        if self.controller:
            self.controller.logout()

    def _on_connect_menu(self) -> None:
        self._on_connect_or_disconnect()

    def _on_disconnect_menu(self) -> None:
        if self.controller:
            self.controller.disconnect_device()

    def _on_about(self) -> None:
        from tkinter import messagebox

        messagebox.showinfo(
            "About",
            "Pacemaker DCM\n"
            "Software version: 1.0\n"
            "Model: MockModel-001\n"
            "DCM Serial: DCM-0001\n"
            "Institution: McMaster University",
        )

    # ===== Toolbar helpers =====

    def _refresh_ports(self) -> None:
        if not self.controller:
            return
        ports = self.controller.list_serial_ports()
        self._port_combo["values"] = ports
        if ports:
            self._port_combo.current(0)

    def _on_refresh_ports(self) -> None:
        self._refresh_ports()

    def _on_connect_or_disconnect(self) -> None:
        if not self.controller:
            return

        if self._connect_button["text"] == "Connect":
            port = self._port_combo.get()
            self.controller.connect_device(port)
        else:
            self.controller.disconnect_device()

    # ===== Status updates =====

    def update_user_label(self, username: str | None) -> None:
        if self._status_user is not None:
            self._status_user.config(text=f"User: {username or '-'}")

    def update_mode_label(self, mode: str) -> None:
        if self._status_mode is not None:
            self._status_mode.config(text=f"Mode: {mode or '-'}")

    def update_connection_status(self, connected: bool, streaming: bool = False) -> None:
        if self._status_connection is not None:
            if not connected:
                text = "DCM–Device: Disconnected"
            else:
                text = "DCM–Device: Connected"
                if streaming:
                    text += " (Telemetry active)"
            self._status_connection.config(text=text)

        if self._connect_button is not None:
            self._connect_button.config(text="Disconnect" if connected else "Connect")

    # ===== Parameters bridge =====

    def set_parameters(self, params) -> None:
        """
        Called by controller when it loads stored parameters.
        """
        if self.parameters_tab is not None:
            self.parameters_tab.set_parameters(params)

    def on_mode_changed(self, mode: str) -> None:
        if self.parameters_tab is not None:
            self.parameters_tab.update_for_mode(mode)
