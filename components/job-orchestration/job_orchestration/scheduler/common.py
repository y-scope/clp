from __future__ import annotations

from enum import IntEnum, auto


class JobStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    DONE = auto()
    SUCCESS = auto()
    SUCCESS_WITH_ERRORS = auto()
    NO_MATCHING_ARCHIVE = auto()
    FAILED = auto()
    CANCELLING = auto()
    CANCELLED = auto()
    PENDING_REDUCER = auto()
    REDUCER_READY = auto()
    PENDING_REDUCER_DONE = auto()

    def __str__(self) -> str:
        return str(self.value).lower()

    def to_str(self) -> str:
        return str(self.name).lower()

    @staticmethod
    def from_str(label: str) -> JobStatus:
        return JobStatus[label.upper()]
