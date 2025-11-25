import json
from pathlib import Path
from typing import Optional

from ..models.parameters import ProgrammableParameters

# Stores parameters per user and per mode in the JSON files under params/
# Written in conjunction with generative AI due to complexity of code and inexperience with JSON storage in python
class ParameterStorage:

    def __init__(self, base_dir: str | Path = "params") -> None:
        self.base_dir = Path(base_dir)
        self.base_dir.mkdir(parents=True, exist_ok=True)

    def _path_for_user(self, username: str) -> Path:
        safe_name = username.replace("/", "_")
        return self.base_dir / f"{safe_name}.json"

    def save_parameters(self, username: str, params: ProgrammableParameters) -> None:
        """
        Save parameters for the given user and the mode inside params.mode.
        Only that mode entry is updated.
        """
        path = self._path_for_user(username)

        data = {"modes": {}}
        if path.exists():
            try:
                data = json.loads(path.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                data = {"modes": {}}

        if "modes" not in data or not isinstance(data["modes"], dict):
            data["modes"] = {}

        data["modes"][params.mode] = params.__dict__
        path.write_text(json.dumps(data, indent=2), encoding="utf-8")

    def _normalize_params_dict(self, params_dict: dict) -> dict:
        """
        Make old JSON files compatible with the current ProgrammableParameters
        definition.

        - Drop obsolete activity_threshold if present.
        - Provide defaults for new fields if missing.
        """
        params_dict = dict(params_dict)  # shallow copy

        # Old field, no longer used
        params_dict.pop("activity_threshold", None)

        params_dict.setdefault("hysteresis_time_ms", 0.0)
        params_dict.setdefault("av_delay_ms", 0.0)

        return params_dict

    def load_parameters(
        self,
        username: str,
        mode: Optional[str] = None,
    ) -> Optional[ProgrammableParameters]:
        """
        Load parameters for a given user and mode.

        If mode is None and a modes dict exists, returns the first mode found.
        Returns None if no file or no matching mode exists.
        """
        path = self._path_for_user(username)
        if not path.exists():
            return None

        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            return None

        # New format with per-mode dict
        if "modes" in data and isinstance(data["modes"], dict):
            modes_dict = data["modes"]

            if mode is None:
                if not modes_dict:
                    return None
                mode_key = next(iter(modes_dict.keys()))
                params_dict = self._normalize_params_dict(modes_dict[mode_key])
                return ProgrammableParameters(**params_dict)

            if mode in modes_dict:
                params_dict = self._normalize_params_dict(modes_dict[mode])
                params_dict["mode"] = mode
                return ProgrammableParameters(**params_dict)
            return None

        # Backward compatibility: old files with a single dict
        if isinstance(data, dict):
            params_dict = self._normalize_params_dict(data)
            if mode is not None:
                params_dict["mode"] = mode
            return ProgrammableParameters(**params_dict)

        return None
