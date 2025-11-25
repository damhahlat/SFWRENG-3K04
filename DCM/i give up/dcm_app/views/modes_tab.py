import tkinter as tk
from tkinter import ttk

from .base_frame import BaseFrame


class ModesTab(BaseFrame):
    """
    Tab for selecting pacemaker mode.
    """

    MODES = [
        "AOO", "VOO", "AAI", "VVI",
        "AOOR", "VOOR", "AAIR", "VVIR",
    ]

    def __init__(self, parent, controller=None, **kwargs):
        super().__init__(parent, controller=controller, **kwargs)
        self.mode_var = None
        self.description_label = None
        self._build_ui()

    def _build_ui(self) -> None:
        # Use tk.StringVar (not ttk.StringVar)
        self.mode_var = tk.StringVar(value="AOO")

        group = ttk.LabelFrame(self, text="Select Mode")
        group.pack(fill="both", expand=True, padx=5, pady=5)

        # Layout modes in two columns
        col_count = 2
        for idx, mode in enumerate(self.MODES):
            row = idx // col_count
            col = idx % col_count
            rb = ttk.Radiobutton(
                group,
                text=mode,
                value=mode,
                variable=self.mode_var,
                command=self._on_mode_changed,
            )
            rb.grid(row=row, column=col, sticky="w", padx=5, pady=5)

        self.description_label = ttk.Label(
            self,
            text="Mode description will appear here.",
            wraplength=400,
            justify="left",
        )
        self.description_label.pack(fill="x", padx=5, pady=5)

        apply_button = ttk.Button(self, text="Apply Mode", command=self._on_apply_mode)
        apply_button.pack(pady=(0, 5))

        # Initialise description
        self._on_mode_changed()

    def _on_mode_changed(self) -> None:
        mode = self.mode_var.get()
        desc = self._mode_description(mode)
        if self.description_label:
            self.description_label.config(text=desc)

    def _on_apply_mode(self) -> None:
        mode = self.mode_var.get()
        if self.controller:
            self.controller.set_mode(mode)

    @staticmethod
    def _mode_description(mode: str) -> str:
        # Very simple placeholders
        descriptions = {
            "AOO": "AOO: Atrial pacing, no sensing, no response.",
            "VOO": "VOO: Ventricular pacing, no sensing, no response.",
            "AAI": "AAI: Atrial pacing, atrial sensing, inhibits pacing.",
            "VVI": "VVI: Ventricular pacing, ventricular sensing, inhibits pacing.",
            "AOOR": "AOOR: Rate-responsive AOO (uses sensor-based rate).",
            "VOOR": "VOOR: Rate-responsive VOO (uses sensor-based rate).",
            "AAIR": "AAIR: Rate-responsive AAI (uses sensor-based rate).",
            "VVIR": "VVIR: Rate-responsive VVI (uses sensor-based rate).",
        }
        return descriptions.get(mode, "No description available.")
