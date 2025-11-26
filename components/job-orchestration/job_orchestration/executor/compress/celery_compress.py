from celery import signals
from celery.app.task import Task
from celery.utils.log import get_task_logger

from job_orchestration.executor.compress.celery import app
from job_orchestration.executor.compress.compression_task import compression_entry_point

# Setup logging
logger = get_task_logger(__name__)


@signals.worker_shutdown.connect
def worker_shutdown_handler(signal=None, sender=None, **kwargs):
    logger.info("Shutdown signal received.")


@app.task(bind=True)
def compress(
    self: Task,
    job_id: int,
    task_id: int,
    tag_ids: list[int],
    clp_io_config_json: str,
    paths_to_compress_json: str,
    clp_metadata_db_connection_config,
):
    return compression_entry_point(
        job_id,
        task_id,
        tag_ids,
        clp_io_config_json,
        paths_to_compress_json,
        clp_metadata_db_connection_config,
        logger,
    )
