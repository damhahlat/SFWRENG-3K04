from dataclasses import dataclass

# Just stores the user's username and password
@dataclass
class User:
    username: str
    password: str
