from typing import Optional


class Result:
    def __init__(self, success: bool, error: Optional[str] = None):
        self.success = success
        self.error = error

    def __repr__(self):
        return f"Result(success={self.success}, error={self.error})"
