from __future__ import annotations

import tkinter as tk
from tkinter import ttk

from .base_frame import BaseFrame
from ..models.egram import EgramPoint


class EgramTab(BaseFrame):
    """
    Tab for displaying egrams.

    Uses controller.start_egram_stream / stop_egram_stream to get data.
    """

    def __init__(self, parent, controller=None, **kwargs):
        super().__init__(parent, controller=controller, **kwargs)
        self.canvas: tk.Canvas | None = None

        self.atrial_var = tk.BooleanVar(value=True)
        self.ventricular_var = tk.BooleanVar(value=True)

        self.points: list[EgramPoint] = []
        self.max_points = 200

        self._build_ui()
        self._schedule_redraw()

    def _build_ui(self) -> None:
        controls = ttk.Frame(self)
        controls.pack(side="top", fill="x", pady=5)

        ttk.Checkbutton(controls, text="Atrial", variable=self.atrial_var).pack(side="left", padx=5)
        ttk.Checkbutton(controls, text="Ventricular", variable=self.ventricular_var).pack(side="left", padx=5)

        start_button = ttk.Button(controls, text="Start Egram", command=self._on_start)
        start_button.pack(side="right", padx=5)

        stop_button = ttk.Button(controls, text="Stop Egram", command=self._on_stop)
        stop_button.pack(side="right", padx=5)

        self.canvas = tk.Canvas(self, background="white", height=300)
        self.canvas.pack(side="top", fill="both", expand=True, pady=5)

    # ===== Start / stop =====

    def _on_start(self) -> None:
        if self.controller:
            self.points.clear()
            self.controller.start_egram_stream(self.handle_egram_point)

    def _on_stop(self) -> None:
        if self.controller:
            self.controller.stop_egram_stream()
        self.points.clear()
        self._draw_placeholder()

    def handle_egram_point(self, point: EgramPoint) -> None:
        """
        Called by the controller (on the Tk main thread) whenever new data arrives.
        """
        self.points.append(point)
        if len(self.points) > self.max_points:
            self.points = self.points[-self.max_points :]

    # ===== Drawing =====

    def _schedule_redraw(self) -> None:
        self._redraw()
        self.after(50, self._schedule_redraw)

    def _draw_placeholder(self) -> None:
        if self.canvas is None:
            return
        self.canvas.delete("all")
        w = int(self.canvas.winfo_width() or 600)
        h = int(self.canvas.winfo_height() or 300)
        self.canvas.create_text(
            w // 2,
            h // 2,
            text="Egram plot will appear here.",
            fill="gray",
        )

    def _redraw(self) -> None:
        if self.canvas is None:
            return

        self.canvas.delete("all")

        if not self.points:
            self._draw_placeholder()
            return

        w = int(self.canvas.winfo_width() or 600)
        h = int(self.canvas.winfo_height() or 300)

        # Time axis: map index to x position
        n = len(self.points)
        if n < 2:
            self._draw_placeholder()
            return

        x_step = w / max(1, self.max_points - 1)

        # We'll draw atrial in upper half, ventricular in lower half.
        mid = h / 2
        amp = (h / 2) * 0.8

        # Get value ranges to roughly normalize (assuming values around [-1, 1])
        atrial_vals = [p.atrial_value for p in self.points]
        vent_vals = [p.ventricular_value for p in self.points]

        def normalize(val_list):
            vmin = min(val_list)
            vmax = max(val_list)
            if vmax - vmin < 1e-6:
                return [0.0 for _ in val_list]
            return [(v - vmin) / (vmax - vmin) * 2 - 1 for v in val_list]

        norm_atrial = normalize(atrial_vals)
        norm_vent = normalize(vent_vals)

        if self.atrial_var.get():
            atrial_points = []
            for i, v in enumerate(norm_atrial):
                x = i * x_step
                y = mid - amp * 0.5 + (-v) * amp * 0.4
                atrial_points.extend([x, y])
            if len(atrial_points) >= 4:
                self.canvas.create_line(*atrial_points)

        if self.ventricular_var.get():
            vent_points = []
            for i, v in enumerate(norm_vent):
                x = i * x_step
                y = mid + amp * 0.5 + (-v) * amp * 0.4
                vent_points.extend([x, y])
            if len(vent_points) >= 4:
                self.canvas.create_line(*vent_points)
