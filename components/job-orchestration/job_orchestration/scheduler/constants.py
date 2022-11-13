TASK_QUEUE_LOWEST_PRIORITY = 1
TASK_QUEUE_HIGHEST_PRIORITY = 3


class QueueName:
    COMPRESSION = "compression"
    SEARCH = "search"


class JobStatus:
    SCHEDULING = 'SCHEDULING'
    SCHEDULED = 'SCHEDULED'
    SUCCEEDED = 'SUCCEEDED'
    FAILED = 'FAILED'


class TaskUpdateType:
    COMPRESSION = 'COMPRESSION'
    SEARCH = 'SEARCH'


class TaskStatus:
    SUBMITTED = 'SUBMITTED'
    SCHEDULED = 'SCHEDULED'
    IN_PROGRESS = 'IN_PROGRESS'
    SUCCEEDED = 'SUCCEEDED'
    FAILED = 'FAILED'
