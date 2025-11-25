from dataclasses import dataclass
from collections import deque


@dataclass
class EgramPoint:
    timestamp_ms: int
    atrial_value: float
    ventricular_value: float

# Just a simple buffer to store the egram data.
class EgramBuffer:

    def __init__(self, maxlen: int = 1000) -> None:
        self._buffer = deque(maxlen=maxlen)

    def add_point(self, point: EgramPoint) -> None:
        self._buffer.append(point)

    def get_points(self) -> list[EgramPoint]:
        return list(self._buffer)
