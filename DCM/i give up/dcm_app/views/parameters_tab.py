from __future__ import annotations

import tkinter as tk
from tkinter import ttk

from .base_frame import BaseFrame
from ..models.parameters import ProgrammableParameters
from ..services.validation import RATE_ADAPTIVE_MODES, validate_parameters


class ParametersTab(BaseFrame):
    """
    Tab for entering programmable parameters.
    """

    def __init__(self, parent, controller=None, **kwargs):
        super().__init__(parent, controller=controller, **kwargs)

        self.entries: dict[str, ttk.Entry] = {}
        self.vars: dict[str, tk.StringVar] = {}

        # Rate-adaptive keys (no Activity Threshold now)
        self._rate_adaptive_keys = [
            "msr",
            "react_time",
            "recov_time",
            "resp_factor",
        ]

        self.message_label: ttk.Label | None = None
        self.info_label: ttk.Label | None = None

        self._build_ui()

    def _build_ui(self) -> None:
        # Basic rate parameters
        rate_frame = ttk.LabelFrame(self, text="Basic Rate Parameters")
        rate_frame.pack(fill="x", padx=5, pady=5)

        self._add_labeled_entry(rate_frame, "Lower Rate Limit (bpm)", "lrl", row=0)
        self._add_labeled_entry(rate_frame, "Upper Rate Limit (bpm)", "url", row=1)

        # Chamber parameters
        chamber_frame = ttk.LabelFrame(self, text="Chamber Pacing and Refractory Periods")
        chamber_frame.pack(fill="x", padx=5, pady=5)

        self._add_labeled_entry(chamber_frame, "Atrial Amplitude (V)", "aa", row=0)
        self._add_labeled_entry(chamber_frame, "Atrial Pulse Width (ms)", "apw", row=1)
        self._add_labeled_entry(chamber_frame, "Ventricular Amplitude (V)", "va", row=2)
        self._add_labeled_entry(chamber_frame, "Ventricular Pulse Width (ms)", "vpw", row=3)
        self._add_labeled_entry(chamber_frame, "ARP (ms)", "arp", row=4)
        self._add_labeled_entry(chamber_frame, "VRP (ms)", "vrp", row=5)
        self._add_labeled_entry(chamber_frame, "Hysteresis Time (ms)", "hyst", row=6)
        self._add_labeled_entry(chamber_frame, "AV Delay (ms)", "avd", row=7)

        # Rate-adaptive parameters (only active in *R modes)
        ra_frame = ttk.LabelFrame(self, text="Rate-Adaptive Parameters (AOOR/VOOR/AAIR/VVIR)")
        ra_frame.pack(fill="x", padx=5, pady=5)

        self._add_labeled_entry(ra_frame, "Maximum Sensor Rate (bpm)", "msr", row=0)
        self._add_labeled_entry(ra_frame, "Reaction Time (s)", "react_time", row=1)
        self._add_labeled_entry(ra_frame, "Recovery Time (min)", "recov_time", row=2)
        self._add_labeled_entry(ra_frame, "Response Factor", "resp_factor", row=3)

        # Buttons and messages as before
        button_frame = ttk.Frame(self)
        button_frame.pack(fill="x", padx=5, pady=10)

        save_button = ttk.Button(button_frame, text="Save Mode Profile", command=self._on_save_local)
        save_button.pack(side="left", padx=5)

        revert_button = ttk.Button(button_frame, text="Revert to Saved Profile", command=self._on_revert_saved)
        revert_button.pack(side="left", padx=5)

        apply_button = ttk.Button(
            button_frame,
            text="Apply to Device & Save Profile",
            command=self._on_apply_device,
        )
        apply_button.pack(side="right", padx=5)

        reset_button = ttk.Button(button_frame, text="Reset to Defaults", command=self._on_reset_defaults)
        reset_button.pack(side="right", padx=5)

        self.message_label = ttk.Label(self, text="", foreground="red")
        self.message_label.pack(fill="x", padx=5, pady=(0, 2))

        self.info_label = ttk.Label(
            self,
            text="Note: Profiles are saved per user and per mode. "
                 "Switching modes reloads that mode's saved profile.",
            foreground="gray",
        )
        self.info_label.pack(fill="x", padx=5, pady=(0, 5))

        for var in self.vars.values():
            var.trace_add("write", self._on_live_change)

    def _add_labeled_entry(self, parent, label_text, key, row) -> None:
        ttk.Label(parent, text=label_text).grid(row=row, column=0, sticky="e", padx=5, pady=2)
        var = tk.StringVar()
        entry = ttk.Entry(parent, width=10, textvariable=var)
        entry.grid(row=row, column=1, sticky="w", padx=5, pady=2)
        self.entries[key] = entry        # type: ignore[assignment]
        self.vars[key] = var             # type: ignore[assignment]

    # ----- Button handlers (unchanged except for the different params) -----

    def _on_save_local(self) -> None:
        params = self._collect_parameters(strict=True)
        if params is None:
            self.message_label.config(text="Invalid input. Please enter numeric values.")
            return
        if self.controller:
            ok, msg = self.controller.save_parameters_locally(params)
            self.message_label.config(text=msg if msg else "")
        else:
            self.message_label.config(text="No controller available.")

    def _on_apply_device(self) -> None:
        params = self._collect_parameters(strict=True)
        if params is None:
            self.message_label.config(text="Invalid input. Please enter numeric values.")
            return
        if self.controller:
            ok, msg = self.controller.validate_and_save_parameters(params)
            self.message_label.config(text=msg if msg else "")
        else:
            self.message_label.config(text="No controller available.")

    def _on_reset_defaults(self) -> None:
        mode = "AOO"
        if hasattr(self.controller, "current_parameters"):
            mode = self.controller.current_parameters.mode  # type: ignore[attr-defined]
        params = ProgrammableParameters(mode=mode)
        self.set_parameters(params)
        self.message_label.config(text="Reset to default values.")

    def _on_revert_saved(self) -> None:
        if not self.controller:
            self.message_label.config(text="No controller available.")
            return
        params = self.controller.reload_current_mode_parameters()
        if params is None:
            self.message_label.config(text="No saved profile found for this mode.")
        else:
            self.message_label.config(text="Reverted to saved profile for this mode.")

    # ----- Collect / set -----

    def _collect_parameters(self, strict: bool = True) -> ProgrammableParameters | None:
        try:
            lrl = float(self.vars["lrl"].get())
            url = float(self.vars["url"].get())
            aa = float(self.vars["aa"].get())
            apw = float(self.vars["apw"].get())
            va = float(self.vars["va"].get())
            vpw = float(self.vars["vpw"].get())
            arp = float(self.vars["arp"].get())
            vrp = float(self.vars["vrp"].get())
            hyst = float(self.vars["hyst"].get() or 0.0)
            avd = float(self.vars["avd"].get() or 0.0)

            msr = float(self.vars["msr"].get() or 0.0)
            react_time = float(self.vars["react_time"].get() or 0.0)
            recov_time = float(self.vars["recov_time"].get() or 0.0)
            resp_factor = float(self.vars["resp_factor"].get() or 0.0)
        except ValueError:
            if strict:
                return None
            return None

        mode = "AOO"
        if hasattr(self.controller, "current_parameters"):
            mode = self.controller.current_parameters.mode  # type: ignore[attr-defined]

        params = ProgrammableParameters(
            mode=mode,
            lower_rate_limit=lrl,
            upper_rate_limit=url,
            atrial_amplitude=aa,
            atrial_pulse_width=apw,
            ventricular_amplitude=va,
            ventricular_pulse_width=vpw,
            arp=arp,
            vrp=vrp,
            hysteresis_time_ms=hyst,
            av_delay_ms=avd,
            max_sensor_rate=msr,
            reaction_time=react_time,
            recovery_time=recov_time,
            response_factor=resp_factor,
        )
        return params

    def set_parameters(self, params: ProgrammableParameters) -> None:
        self.vars["lrl"].set(str(params.lower_rate_limit))
        self.vars["url"].set(str(params.upper_rate_limit))

        self.vars["aa"].set(str(params.atrial_amplitude))
        self.vars["apw"].set(str(params.atrial_pulse_width))
        self.vars["va"].set(str(params.ventricular_amplitude))
        self.vars["vpw"].set(str(params.ventricular_pulse_width))

        self.vars["arp"].set(str(params.arp))
        self.vars["vrp"].set(str(params.vrp))
        self.vars["hyst"].set(str(params.hysteresis_time_ms))
        self.vars["avd"].set(str(params.av_delay_ms))

        self.vars["msr"].set(str(params.max_sensor_rate))
        self.vars["react_time"].set(str(params.reaction_time))
        self.vars["recov_time"].set(str(params.recovery_time))
        self.vars["resp_factor"].set(str(params.response_factor))

        self.update_for_mode(params.mode)

    # ----- Mode awareness & validation -----

    def update_for_mode(self, mode: str) -> None:
        ra = mode in RATE_ADAPTIVE_MODES
        state = "normal" if ra else "disabled"
        for key in self._rate_adaptive_keys:
            entry = self.entries.get(key)
            if entry is not None:
                entry.configure(state=state)

    def _on_live_change(self, *_) -> None:
        self._run_live_validation()

    def _run_live_validation(self) -> None:
        if self.message_label is None:
            return
        params = self._collect_parameters(strict=False)
        if params is None:
            self.message_label.config(text="")
            return
        ok, msg = validate_parameters(params)
        self.message_label.config(text="" if ok else msg)
