from __future__ import annotations

from typing import Tuple

from ..models.parameters import ProgrammableParameters

# Modes that use rate adaptive
RATE_ADAPTIVE_MODES = {"AOOR", "VOOR", "AAIR", "VVIR"}

# Check the ranges and ensure that it works. Returns a tuple of a bool and a string with the error message.
# The tuple return was a recommendation by generative AI.
def _check_range(name: str, value: float, lo: float, hi: float, step: float | None = None) -> Tuple[bool, str]:
    if value < lo or value > hi:
        return False, f"{name} must be between {lo} and {hi}."
    if step is not None:
        # allow for small floating roundoff
        k = round((value - lo) / step)
        if abs(lo + k * step - value) > 1e-6:
            return False, f"{name} must change in steps of {step}."
    return True, ""

# Just validates the parameters individually and return a boolean, signifying if it's valid, and a string with the error message.
def validate_parameters(params: ProgrammableParameters) -> Tuple[bool, str]:

    # LRL / URL basic
    ok, msg = _check_range("Lower Rate Limit", params.lower_rate_limit, 30, 175, 5)
    if not ok:
        return ok, msg

    ok, msg = _check_range("Upper Rate Limit", params.upper_rate_limit, 50, 175, 5)
    if not ok:
        return ok, msg

    if params.upper_rate_limit <= params.lower_rate_limit:
        return False, "Upper Rate Limit must be greater than Lower Rate Limit."

    # Amplitudes
    ok, msg = _check_range("Atrial Amplitude", params.atrial_amplitude, 0.5, 7.0, 0.5)
    if not ok:
        return ok, msg
    ok, msg = _check_range("Ventricular Amplitude", params.ventricular_amplitude, 0.5, 7.0, 0.5)
    if not ok:
        return ok, msg

    # Pulse widths
    ok, msg = _check_range("Atrial Pulse Width", params.atrial_pulse_width, 0.05, 1.9, 0.05)
    if not ok:
        return ok, msg
    ok, msg = _check_range("Ventricular Pulse Width", params.ventricular_pulse_width, 0.05, 1.9, 0.05)
    if not ok:
        return ok, msg

    # Refractory periods
    ok, msg = _check_range("ARP", params.arp, 150, 500, 10)
    if not ok:
        return ok, msg
    ok, msg = _check_range("VRP", params.vrp, 150, 500, 10)
    if not ok:
        return ok, msg

    # AV delay & Hysteresis – spec is usually 70–300 ms, I’ll allow a wide range 0–500 with 10ms steps
    ok, msg = _check_range("AV Delay", params.av_delay_ms, 0, 500, 10)
    if not ok:
        return ok, msg

    ok, msg = _check_range("Hysteresis Time", params.hysteresis_time_ms, 0, 500, 10)
    if not ok:
        return ok, msg

    # Rate adaptive
    if params.mode in RATE_ADAPTIVE_MODES:
        ok, msg = _check_range("Maximum Sensor Rate", params.max_sensor_rate, 50, 175, 5)
        if not ok:
            return ok, msg

        ok, msg = _check_range("Reaction Time", params.reaction_time, 10, 50, 10)
        if not ok:
            return ok, msg

        ok, msg = _check_range("Recovery Time", params.recovery_time, 2, 16, 1)
        if not ok:
            return ok, msg

        ok, msg = _check_range("Response Factor", params.response_factor, 1, 16, 1)
        if not ok:
            return ok, msg
    else:
        pass

    return True, ""
