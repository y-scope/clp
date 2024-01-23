import os

from job_orchestration.scheduler.constants import QueueName

imports = ("job_orchestration.executor.search.fs_search_task")

task_routes = {
    'job_orchestration.executor.search.fs_search_task.search': QueueName.SEARCH,
}

broker_url = os.getenv('BROKER_URL')
result_backend = os.getenv('RESULT_BACKEND')

result_persistent = True

# Differentiate between tasks that have started v.s. tasks still in queue
task_track_started = True

accept_content = [
    "application/json",  # json
    "application/x-python-serialize",  # pickle
]

result_accept_content = [
    "application/json", # json
    "application/x-python-serialize", # pickle
]

# TODO: Find out how to precisely specify the serialization format for both the
# task (args + kwargs) and the task return value (instead of using json/pickle
# for everything). See also:
# https://stackoverflow.com/questions/69531560/how-do-you-configure-celery-to-use-serializer-pickle
# https://docs.celeryq.dev/en/stable/internals/protocol.html#task-messages
result_serializer = "json"
