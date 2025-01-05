from __future__ import annotations

from enum import auto, IntEnum

TASK_QUEUE_LOWEST_PRIORITY = 1
TASK_QUEUE_HIGHEST_PRIORITY = 3


class QueueName:
    COMPRESSION = "compression"
    QUERY = "query"


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
# and make necessary changes in the UI, Query Scheduler, and Reducer
class QueryJobStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()
    CANCELLING = auto()
    CANCELLED = auto()

    @staticmethod
    def from_str(label: str) -> QueryJobStatus:
        return QueryJobStatus[label.upper()]

    def __str__(self) -> str:
        return str(self.value)

    def to_str(self) -> str:
        return str(self.name)


class QueryTaskStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()
    CANCELLED = auto()

    @staticmethod
    def from_str(label: str) -> QueryTaskStatus:
        return QueryTaskStatus[label.upper()]

    def __str__(self) -> str:
        return str(self.value)

    def to_str(self) -> str:
        return str(self.name)


class QueryJobType(IntEnum):
    SEARCH_OR_AGGREGATION = 0
    EXTRACT_IR = auto()
    EXTRACT_JSON = auto()

    def __str__(self) -> str:
        return str(self.value)

    def to_str(self) -> str:
        return str(self.name)
