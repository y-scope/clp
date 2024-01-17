from __future__ import annotations

from enum import IntEnum, auto

# When adding new states always add them to the end of this enum
# and make necessary changes in the UI, Search Scheduler, and Reducer
class JobStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCESS = auto()
    FAILED = auto()
    CANCELLING = auto()
    CANCELLED = auto()
    NO_MATCHING_ARCHIVE = auto()

    def __str__(self) -> str:
        return str(self.value).lower()

    def to_str(self) -> str:
        return str(self.name).lower()

    @staticmethod
    def from_str(label: str) -> JobStatus:
        return JobStatus[label.upper()]
