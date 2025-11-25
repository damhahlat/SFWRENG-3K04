from __future__ import annotations

import math
import struct
import threading
import time
from typing import Callable, List, Optional

from ..models.parameters import ProgrammableParameters
from ..models.egram import EgramPoint

# Because pyserial is acting funny sometimes
try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None


# First byte is status (always 1), second is a header byte (0)
SET_HEADER_BYTE_1 = 0x01
SET_HEADER_BYTE_2 = 0x00

# 1 indexed mode encoding due to simulink
MODE_TO_BYTE = {
    "AOO": 1,
    "VOO": 2,
    "AAI": 3,
    "VVI": 4,
    "AOOR": 5,
    "VOOR": 6,
    "AAIR": 7,
    "VVIR": 8,
    "DDDR": 9,
}

BYTE_TO_MODE = {v: k for k, v in MODE_TO_BYTE.items()}

# Class that encapsulates everything for serial management
class SerialManager:

    def __init__(self) -> None:
        self._connected = False
        self._port: Optional[str] = None
        self._ser = None

        self._mock_mode = serial is None

        self._last_parameters: Optional[ProgrammableParameters] = None

        # Serial monitor
        self._monitor_thread: Optional[threading.Thread] = None
        self._monitor_running = False
        self._monitor_paused = False

        # Egram
        self._egram_thread: Optional[threading.Thread] = None
        self._egram_running = False
        self._egram_callback: Optional[Callable[[EgramPoint], None]] = None

    # Helper stuff
    def is_connected(self) -> bool:
        return self._connected

    # For testing purposes primarily
    def uses_mock_mode(self) -> bool:
        return self._mock_mode

    def list_ports(self) -> List[str]:
        if self._mock_mode:
            return ["MockDevice-1"]
        return [p.device for p in serial.tools.list_ports.comports()]

    # Connection and disconnection
    def connect(self, port: str, baudrate: int = 115200) -> bool:
        if self._mock_mode:
            self._connected = True
            self._port = port
            print(f"[SerialManager] Mock connected to {port}")
            return True

        try:
            self._ser = serial.Serial(port, baudrate=baudrate, timeout=0.1)
            self._connected = self._ser.is_open
            self._port = port if self._connected else None

            print(f"[SerialManager] Connected: {self._connected}")

            if self._connected:
                self._start_monitor()

            return self._connected

        except Exception as e:
            print(f"[SerialManager] Connect error: {e}")
            self._connected = False
            self._ser = None
            return False

    def disconnect(self) -> None:
        self.stop_egram_stream()
        self._stop_monitor()

        if not self._mock_mode and self._ser:
            try:
                self._ser.close()
            except Exception:
                pass

        self._connected = False
        self._ser = None
        self._port = None
        print("[SerialManager] Disconnected")

    # Serial monitor, for debugging
    # Written in conjunction with generative AI due to complexity and unfamiliarity
    def _start_monitor(self):
        if self._mock_mode or self._monitor_running:
            return

        self._monitor_running = True
        self._monitor_paused = False

        def loop():
            print("[SerialMonitor] started")
            while self._monitor_running:
                if self._monitor_paused or self._ser is None:
                    time.sleep(0.05)
                    continue

                try:
                    data = self._ser.read(64)
                    if not data:
                        continue

                    print(f"[SerialMonitor] RX {len(data)} bytes: {data.hex(' ')}")
                    print(f"[SerialMonitor] decimal: {list(data)}")

                    if (
                        len(data) == 31
                        and data[0] == SET_HEADER_BYTE_1
                        and data[1] == SET_HEADER_BYTE_2
                    ):
                        self._print_frame_breakdown(data, "[SerialMonitor] decoded")

                except Exception as e:
                    # Handle Windows "device not functioning" / PermissionError cleanly
                    print(f"[SerialMonitor] error: {e}")
                    if isinstance(e, PermissionError) or "device attached to the system is not functioning" in str(e).lower():
                        print("[SerialMonitor] fatal serial error, stopping monitor loop.")
                        break
                    time.sleep(0.1)

            self._monitor_running = False
            print("[SerialMonitor] stopped")

        self._monitor_thread = threading.Thread(target=loop, daemon=True)
        self._monitor_thread.start()

    def _stop_monitor(self):
        self._monitor_running = False
        if self._monitor_thread:
            try:
                self._monitor_thread.join(timeout=0.5)
            except Exception:
                pass
        self._monitor_thread = None

    def _pause_monitor(self):
        self._monitor_paused = True

    def _resume_monitor(self):
        self._monitor_paused = False

    # Builds frame
    def _build_frame(self, p: ProgrammableParameters) -> bytes:
        b = bytearray()

        # Byte 1 = status bit (1), Byte 2 = header (0)
        b.append(SET_HEADER_BYTE_1)
        b.append(SET_HEADER_BYTE_2)

        # Mode and rates
        b.append(MODE_TO_BYTE.get(p.mode, 1)) # default AOO = 1
        b.append(int(p.lower_rate_limit) & 0xFF)
        b.append(int(p.max_sensor_rate) & 0xFF)

        def f32(x): return struct.pack("<f", float(x))
        def u16(x): return struct.pack("<H", int(x) & 0xFFFF)

        # Floats
        b.extend(f32(p.atrial_amplitude))
        b.extend(f32(p.ventricular_amplitude))
        b.extend(f32(p.atrial_pulse_width))
        b.extend(f32(p.ventricular_pulse_width))

        # 16 bit timings
        b.extend(u16(p.vrp))
        b.extend(u16(p.arp))

        # Hysteresis and AV delay
        b.append(int(p.hysteresis_time_ms) & 0xFF)
        b.extend(u16(p.av_delay_ms))

        # Rate adaptive
        b.append(int(p.reaction_time) & 0xFF)
        b.append(int(p.response_factor) & 0xFF)
        b.append(int(p.recovery_time) & 0xFF)

        return bytes(b)

    # Prints the frame
    def _print_frame_breakdown(self, frame: bytes, prefix: str):
        print(prefix, "bytes:", list(frame))

        if len(frame) != 31:
            print(prefix, "WARNING: expected 31 bytes")
            return

        # Put reading the frame in a try except in case there is an error at any point
        try:
            h1, h2 = frame[:2]
            mode_code = frame[2]
            mode = BYTE_TO_MODE.get(mode_code, f"UNKNOWN({mode_code})")

            lrl = frame[3]
            msr = frame[4]

            aa = struct.unpack("<f", frame[5:9])[0]
            va = struct.unpack("<f", frame[9:13])[0]
            apw = struct.unpack("<f", frame[13:17])[0]
            vpw = struct.unpack("<f", frame[17:21])[0]

            vrp = struct.unpack("<H", frame[21:23])[0]
            arp = struct.unpack("<H", frame[23:25])[0]

            hyst = frame[25]
            avd = struct.unpack("<H", frame[26:28])[0]

            react = frame[28]
            resp = frame[29]
            recov = struct.unpack("b", bytes([frame[30]]))[0] if False else frame[30]

            print(
                f"{prefix} decoded:"
                f"\n  status/header = ({h1}, {h2})"
                f"\n  mode = {mode} ({mode_code})"
                f"\n  LRL = {lrl}"
                f"\n  MSR = {msr}"
                f"\n  AAmp = {aa}"
                f"\n  VAmp = {va}"
                f"\n  APW = {apw}"
                f"\n  VPW = {vpw}"
                f"\n  VRP = {vrp}"
                f"\n  ARP = {arp}"
                f"\n  Hysteresis = {hyst}"
                f"\n  AV Delay = {avd}"
                f"\n  Reaction = {react}"
                f"\n  ResponseFactor = {resp}"
                f"\n  Recovery = {recov}"
            )

        except Exception as e:
            print(prefix, "decode error:", e)

    # Send parameters to the pacemaker.
    # Written in conjunction with generative AI due to unfamiliarity with serial communication in Python and Simulink.
    def send_and_verify_parameters(
        self, params: ProgrammableParameters, echo_timeout: float = 1.0
    ) -> bool:
        if not self._connected:
            print("[SerialManager] not connected")
            return False

        tx = self._build_frame(params)
        print("[SerialManager] TX:", tx.hex(" "))
        self._print_frame_breakdown(tx, "[SerialManager] TX breakdown")

        if self._mock_mode:
            print("[SerialManager] mock send ok")
            self._last_parameters = params
            return True

        if self._ser is None:
            print("[SerialManager] Serial object is None")
            return False

        try:
            self._ser.write(tx)
            self._ser.flush()
            return True
        except Exception as e:
            print("[SerialManager] error sending parameters:", e)
            return False

    # Egram. Pacemaker sends uint8 bytes at 0.01 s intervals. Convert every 2 bytes into an egrampoint.
    # Written in conjunction with generative AI due to complexity and unfamiliarity with threading.
    def start_egram_stream(self, callback):

        if not self._connected:
            print("[SerialManager] cannot start egram, not connected")
            return

        # Stop monitor so it does not compete for bytes
        self._stop_monitor()

        self._egram_callback = callback
        self._egram_running = True

        if self._mock_mode:
            th = threading.Thread(target=self._mock_egram_loop, daemon=True)
        else:
            th = threading.Thread(target=self._serial_egram_loop, daemon=True)

        self._egram_thread = th
        th.start()

    def stop_egram_stream(self):
        self._egram_running = False
        self._egram_callback = None
        if self._connected and not self._mock_mode:
            # Resume monitor when not streaming egram
            self._start_monitor()

    def _mock_egram_loop(self):
        t0 = time.time()
        while self._egram_running:
            t_ms = int((time.time() - t0) * 1000.0)
            a = math.sin(t_ms / 1000.0 * 2 * math.pi)
            v = math.sin(t_ms / 1000.0 * 2.3 * math.pi)
            if self._egram_callback:
                self._egram_callback(EgramPoint(t_ms, a, v))
            time.sleep(0.01)

    def _serial_egram_loop(self):
        if self._ser is None:
            return

        t0 = time.time()
        sample_count = 0

        print("[SerialManager] Egram loop started (uint8 atr/vent pairs).")

        try:
            while self._egram_running and self._ser.is_open:
                try:
                    chunk = self._ser.read(2)
                    if len(chunk) < 2:
                        continue

                    atr_u8, vent_u8 = chunk[0], chunk[1]

                    atr_val = float(atr_u8)
                    vent_val = float(vent_u8)

                    t_ms = int((time.time() - t0) * 1000.0)

                    if sample_count < 10:
                        print(f"[Egram] sample {sample_count}: atr={atr_u8}, vent={vent_u8}, t={t_ms} ms")
                    sample_count += 1

                    if self._egram_callback:
                        pt = EgramPoint(
                            timestamp_ms=t_ms,
                            atrial_value=atr_val,
                            ventricular_value=vent_val,
                        )
                        self._egram_callback(pt)

                except Exception as e:
                    print("[SerialManager] egram loop error:", e)
                    # If the device just died / was unplugged, exit loop cleanly
                    if isinstance(e, PermissionError) or "device attached to the system is not functioning" in str(e).lower():
                        print("[SerialManager] fatal serial error in egram loop, stopping.")
                        break
                    time.sleep(0.01)
                    continue

        finally:
            self._egram_running = False
            print("[SerialManager] Egram loop stopped.")
