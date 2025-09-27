import os

from job_orchestration.scheduler.constants import SchedulerType, TASK_QUEUE_HIGHEST_PRIORITY

# Worker settings
# Force workers to consume only one task at a time
worker_prefetch_multiplier = 1
imports = [
    "job_orchestration.executor.compress.celery_compress",
]

# Queue settings
task_queue_max_priority = TASK_QUEUE_HIGHEST_PRIORITY
task_routes = {
    "job_orchestration.executor.compress.celery_compress.compress": SchedulerType.COMPRESSION,
}
task_create_missing_queues = True

# Results backend settings
result_persistent = True

broker_url = os.getenv("BROKER_URL")
result_backend = os.getenv("RESULT_BACKEND")
