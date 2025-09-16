from __future__ import annotations

from enum import auto, IntEnum

TASK_QUEUE_LOWEST_PRIORITY = 1
TASK_QUEUE_HIGHEST_PRIORITY = 3


class SchedulerType:
    COMPRESSION = "compression"
    QUERY = "query"


class StatusIntEnum(IntEnum):
    """
    Delegates __str__ to int.__str__, matching the behavior of IntEnum in Python 3.11+.
    TODO: Remove this when our minimum supported Python version is 3.11+.
    """

    def __str__(self) -> str:
        return str(self.value)

    def to_str(self) -> str:
        return self.name


class CompressionJobStatus(StatusIntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()
    KILLED = auto()


class CompressionJobCompletionStatus(StatusIntEnum):
    SUCCEEDED = 0
    FAILED = auto()


class CompressionTaskStatus(StatusIntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()
    KILLED = auto()


# When adding new states always add them to the end of this enum
# and make necessary changes in the UI, Query Scheduler, and Reducer
class QueryJobStatus(StatusIntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()
    CANCELLING = auto()
    CANCELLED = auto()
    KILLED = auto()

    @staticmethod
    def from_str(label: str) -> QueryJobStatus:
        return QueryJobStatus[label.upper()]


class QueryTaskStatus(StatusIntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCEEDED = auto()
    FAILED = auto()
    CANCELLED = auto()
    KILLED = auto()

    @staticmethod
    def from_str(label: str) -> QueryTaskStatus:
        return QueryTaskStatus[label.upper()]


class QueryJobType(StatusIntEnum):
    SEARCH_OR_AGGREGATION = 0
    EXTRACT_IR = auto()
    EXTRACT_JSON = auto()
