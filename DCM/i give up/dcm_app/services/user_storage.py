import json
from pathlib import Path
from typing import Tuple

from ..models.user import User

# JSON user storage and enforces a max of 10
class UserStorage:

    MAX_USERS = 10

    def __init__(self, path: str | Path = "users.json") -> None:
        self.path = Path(path)
        self._data = {"users": []}
        self._load()

    def _load(self) -> None:
        if self.path.exists():
            try:
                self._data = json.loads(self.path.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                self._data = {"users": []}
        else:
            self._data = {"users": []}

    def _save(self) -> None:
        self.path.write_text(json.dumps(self._data, indent=2), encoding="utf-8")

    def _find_user(self, username: str) -> dict | None:
        for u in self._data.get("users", []):
            if u.get("username") == username:
                return u
        return None

    def register_user(self, username: str, password: str) -> Tuple[bool, str]:
        if self._find_user(username) is not None:
            return False, "Username already exists."

        if len(self._data.get("users", [])) >= self.MAX_USERS:
            return False, "Maximum number of users reached."

        user_dict = {"username": username, "password": password}
        self._data["users"].append(user_dict)
        self._save()
        return True, "User registered."

    def validate_credentials(self, username: str, password: str) -> bool:
        user = self._find_user(username)
        if user is None:
            return False
        return user.get("password") == password
