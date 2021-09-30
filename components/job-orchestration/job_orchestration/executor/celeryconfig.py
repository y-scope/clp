import os
result_persistent = True
worker_prefetch_multiplier = 1
task_queue_max_priority = 3
imports = 'job_orchestration.executor.compression.task'
task_routes = {
    'job_orchestration.executor.compression.task.compress': 'compression'
}
task_create_missing_queues = True

broker_url = os.getenv('BROKER_URL')
result_backend = os.getenv('RESULT_BACKEND')