from __future__ import annotations

from dataclasses import dataclass, field

# This class stores all programmable pacemaker parameterrs.
# These are stored per user and per mode in JSON, and is used to build the SET_PARAM frame which is sent to the pacemaker
@dataclass
class ProgrammableParameters:

    # Mode selection
    mode: str = "AOO"

    # Basic rate parameters
    lower_rate_limit: float = 60.0 # bpm (LRL)
    upper_rate_limit: float = 120.0 # bpm (URL) – not in frame snippet but kept here
    max_sensor_rate: float = 120.0 # bpm (MSR)

    atrial_amplitude: float = 3.5 # V
    atrial_pulse_width: float = 1.0 # ms
    ventricular_amplitude: float = 3.5 # V
    ventricular_pulse_width: float = 1.0 # ms
    arp: float = 250.0 # ms
    vrp: float = 320.0 # ms

    hysteresis_time_ms: float = 0.0 # ms
    av_delay_ms: float = 0.0 # ms

    # Rate adaptive
    reaction_time: float = 30.0 # s
    recovery_time: float = 5.0 # min
    response_factor: float = 8.0 # unitless

    # Extra bag for anything not explicitly modeled
    extra: dict[str, float] = field(default_factory=dict)
