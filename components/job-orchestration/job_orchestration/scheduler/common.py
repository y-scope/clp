from __future__ import annotations

from enum import IntEnum, auto


class JobStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    DONE = auto()
    SUCCESS = auto()
    SUCCESS_WITH_ERRORS = auto()
    FAILED = auto()
    CANCELLING = auto()
    CANCELLED = auto()

    def __str__(self) -> str:
        return str(self.value).lower()

    @staticmethod
    def from_str(label: str) -> JobStatus:
        return JobStatus[label.upper()]