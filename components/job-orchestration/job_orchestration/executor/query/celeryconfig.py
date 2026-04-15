import os

from job_orchestration.scheduler.constants import SchedulerType

imports = (
    "job_orchestration.executor.query.fs_search_task",
    "job_orchestration.executor.query.extract_stream_task",
)

task_routes = {
    "job_orchestration.executor.query.fs_search_task.search": SchedulerType.QUERY,
    "job_orchestration.executor.query.extract_stream_task.extract_stream": SchedulerType.QUERY,
}
task_create_missing_queues = True

broker_url = os.getenv("BROKER_URL")
result_backend = os.getenv("RESULT_BACKEND")

result_persistent = True

# Differentiate between tasks that have started v.s. tasks still in queue
task_track_started = True

# Allow workers to prefetch up to 4 tasks so the next task is ready to execute immediately
# when the current one finishes, reducing idle time between tasks.
worker_prefetch_multiplier = 4

# Acknowledge tasks only after they complete (not when received), so that if a worker crashes
# mid-task, the task is re-delivered to another worker instead of being lost.
task_acks_late = True

# Reject tasks back to the queue when a worker is shut down, rather than losing them.
task_reject_on_worker_lost = True

accept_content = [
    "application/json",  # json
    "application/x-python-serialize",  # pickle
]

result_accept_content = [
    "application/json",  # json
    "application/x-python-serialize",  # pickle
]

# TODO: Choose a different serialization format for tasks and results. Sticking with json is
# probably not a good idea.
result_serializer = "json"
