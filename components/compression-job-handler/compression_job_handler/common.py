from __future__ import annotations

from enum import IntEnum, auto


class JobStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    DONE = auto()
    SUCCESS = auto()
    SUCCESS_WITH_ERRORS = auto()
    FAILED = auto()
    FAILED_TO_SUBMIT = auto()
    NO_INPUTS_TO_COMPRESS = auto()
    CANCELLING = auto()
    CANCELLED = auto()

    def __str__(self) -> str:
        return str(self.name).lower()

    @staticmethod
    def from_str(label: str) -> JobStatus:
        return JobStatus[label.upper()]