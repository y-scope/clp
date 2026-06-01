from celery import Celery, signals
from celery.utils.log import get_task_logger

from job_orchestration.executor.query import celeryconfig
from job_orchestration.executor.telemetry_utils import init_worker_telemetry

logger = get_task_logger(__name__)

app = Celery("query")
app.config_from_object(celeryconfig)

@signals.worker_process_init.connect
def worker_process_init_handler(**kwargs):
    init_worker_telemetry("query-worker", logger)

@signals.worker_shutdown.connect
def worker_shutdown_handler(signal=None, sender=None, **kwargs):
    logger.info("Shutdown signal received.")
    from clp_py_utils.telemetry import shutdown_telemetry
    shutdown_telemetry()

if "__main__" == __name__:
    app.start()
