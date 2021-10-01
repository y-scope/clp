import enum


class JobCompletionStatus(enum.IntEnum):
    SUCCEEDED = 0
    FAILED = 1
    SUCCEEDED_WITH_ERRORS = 2
