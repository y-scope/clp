import os

from job_orchestration.scheduler.constants import QueueName, TASK_QUEUE_HIGHEST_PRIORITY

imports = ("job_orchestration.executor.search.fs_search_method")

task_routes = {
    'job_orchestration.executor.search.fs_search_method.search': QueueName.SEARCH
}

broker_url = os.getenv('BROKER_URL')
result_backend = os.getenv('RESULT_BACKEND')

result_persistent = True

# Differentiate between tasks that have started v.s. tasks still in queue
task_track_started = True

# The JSON serializer must be present. Otherwise, many of the Celery CLI will
# fail (e.g. inspect worker online status).
accept_content = [
    "application/json",  # json
    "application/x-python-serialize",  # pickle
    # "application/x-yaml",  # yaml
]

result_accept_content = [
    "application/json",
    "application/x-python-serialize",
]

# TODO: Find out how to precisely specify the serialization format for both the
# task (args + kwargs) and the task return value (instead of using json/pickle
# for everything). See also:
# https://stackoverflow.com/questions/69531560/how-do-you-configure-celery-to-use-serializer-pickle
# https://docs.celeryq.dev/en/stable/internals/protocol.html#task-messages
result_serializer = "json"
