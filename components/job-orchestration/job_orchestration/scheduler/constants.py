from __future__ import annotations

from enum import auto, IntEnum

TASK_QUEUE_LOWEST_PRIORITY = 1
TASK_QUEUE_HIGHEST_PRIORITY = 3


class QueueName:
    COMPRESSION = "compression"
    SEARCH = "search"


class CompressionJobStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()


class CompressionJobCompletionStatus(IntEnum):
    SUCCEEDED = 0
    FAILED = auto()


class CompressionTaskStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()


# When adding new states always add them to the end of this enum
# and make necessary changes in the UI, Search Scheduler, and Reducer
class SearchJobStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()
    CANCELLING = auto()
    CANCELLED = auto()

    @staticmethod
    def from_str(label: str) -> SearchJobStatus:
        return SearchJobStatus[label.upper()]

    def __str__(self) -> str:
        return str(self.value)

    def to_str(self) -> str:
        return str(self.name)
