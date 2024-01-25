from __future__ import annotations

from enum import IntEnum, auto

TASK_QUEUE_LOWEST_PRIORITY = 1
TASK_QUEUE_HIGHEST_PRIORITY = 3


class QueueName:
    COMPRESSION = "compression"
    SEARCH = "search"


class CompressionJobStatus:
    SCHEDULING = 'SCHEDULING'
    SCHEDULED = 'SCHEDULED'
    SUCCEEDED = 'SUCCEEDED'
    FAILED = 'FAILED'


class CompressionJobCompletionStatus(IntEnum):
    SUCCEEDED = 0
    FAILED = 1


class CompressionTaskUpdateType:
    COMPRESSION = 'COMPRESSION'
    SEARCH = 'SEARCH'


class CompressionTaskStatus:
    SUBMITTED = 'SUBMITTED'
    SCHEDULED = 'SCHEDULED'
    IN_PROGRESS = 'IN_PROGRESS'
    SUCCEEDED = 'SUCCEEDED'
    FAILED = 'FAILED'


# When adding new states always add them to the end of this enum
# and make necessary changes in the UI, Search Scheduler, and Reducer
class SearchJobStatus(IntEnum):
    PENDING = 0
    RUNNING = auto()
    SUCCESS = auto()
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
