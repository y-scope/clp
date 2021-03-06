import os

# Worker settings
# Force workers to consume only one task at a time
worker_prefetch_multiplier = 1
imports = ['job_orchestration.executor.compression.task']

# Queue settings
task_queue_max_priority = 3
task_routes = {'job_orchestration.executor.compression.task.compress': 'compression'}
task_create_missing_queues = True

# Results backend settings
result_persistent = True

broker_url = os.getenv('BROKER_URL')
result_backend = os.getenv('RESULT_BACKEND')
