from tkinter import ttk

from .base_frame import BaseFrame


class DeviceInfoTab(BaseFrame):
    """
    Tab showing static device info (for now).
    """

    def __init__(self, parent, controller=None, **kwargs):
        super().__init__(parent, controller=controller, **kwargs)
        self._build_ui()

    def _build_ui(self) -> None:
        frame = ttk.Frame(self)
        frame.pack(fill="both", expand=True, padx=5, pady=5)

        labels = [
            "Device model:",
            "Device serial:",
            "Battery status:",
            "Last interrogation:",
        ]

        values = [
            "MockModel-001",
            "SN-000000",
            "BOL",
            "Not available",
        ]

        for i, (label_text, value_text) in enumerate(zip(labels, values)):
            ttk.Label(frame, text=label_text).grid(row=i, column=0, sticky="e", padx=5, pady=3)
            ttk.Label(frame, text=value_text).grid(row=i, column=1, sticky="w", padx=5, pady=3)
