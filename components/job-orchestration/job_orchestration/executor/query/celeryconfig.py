import os

from job_orchestration.executor.utils import load_clp_config_from_config_path_env_var
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
result_expires = 7200

_clp_config = load_clp_config_from_config_path_env_var()
task_soft_time_limit = _clp_config.query_worker.task_soft_time_limit
task_time_limit = _clp_config.query_worker.task_time_limit

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
