import os

from job_orchestration.scheduler.constants import QueueName

imports = "job_orchestration.executor.search.fs_search_task"

task_routes = {
    "job_orchestration.executor.search.fs_search_task.search": QueueName.SEARCH,
}
task_create_missing_queues = True

broker_url = os.getenv("BROKER_URL")
result_backend = os.getenv("RESULT_BACKEND")

result_persistent = True

# Differentiate between tasks that have started v.s. tasks still in queue
task_track_started = True

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
