import datetime
import os
import pathlib
from contextlib import closing

from celery import signals
from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import Database, WorkerConfig
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.compress.celery import app
from job_orchestration.executor.compress.compression_task import (
    increment_compression_job_metadata,
    run_clp,
    update_compression_task_metadata,
)
from job_orchestration.scheduler.constants import CompressionTaskStatus
from job_orchestration.scheduler.job_config import ClpIoConfig, PathsToCompress
from job_orchestration.scheduler.scheduler_data import CompressionTaskResult

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
    tag_ids,
    clp_io_config_json: str,
    paths_to_compress_json: str,
    clp_metadata_db_connection_config,
):
    clp_home = pathlib.Path(os.getenv("CLP_HOME"))

    # Set logging level
    logs_dir = pathlib.Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = str(os.getenv("CLP_LOGGING_LEVEL"))
    set_logging_level(logger, clp_logging_level)

    # Load configuration
    try:
        worker_config = WorkerConfig.parse_obj(
            read_yaml_config_file(pathlib.Path(os.getenv("CLP_CONFIG_PATH")))
        )
    except Exception as ex:
        error_msg = "Failed to load worker config"
        logger.exception(error_msg)
        return CompressionTaskResult(
            task_id=task_id,
            status=CompressionTaskStatus.FAILED,
            duration=0,
            error_message=error_msg,
        )

    clp_io_config = ClpIoConfig.parse_raw(clp_io_config_json)
    paths_to_compress = PathsToCompress.parse_raw(paths_to_compress_json)

    sql_adapter = SQL_Adapter(Database.parse_obj(clp_metadata_db_connection_config))

    start_time = datetime.datetime.now()
    logger.info(f"[job_id={job_id} task_id={task_id}] COMPRESSION STARTED.")
    compression_task_status, worker_output = run_clp(
        worker_config,
        clp_io_config,
        clp_home,
        logs_dir,
        job_id,
        task_id,
        tag_ids,
        paths_to_compress,
        sql_adapter,
        clp_metadata_db_connection_config,
        logger,
    )
    duration = (datetime.datetime.now() - start_time).total_seconds()
    logger.info(f"[job_id={job_id} task_id={task_id}] COMPRESSION COMPLETED.")

    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        update_compression_task_metadata(
            db_cursor,
            task_id,
            dict(
                start_time=start_time,
                status=compression_task_status,
                partition_uncompressed_size=worker_output["total_uncompressed_size"],
                partition_compressed_size=worker_output["total_compressed_size"],
                duration=duration,
            ),
        )
        if CompressionTaskStatus.SUCCEEDED == compression_task_status:
            increment_compression_job_metadata(db_cursor, job_id, dict(num_tasks_completed=1))
        db_conn.commit()

    compression_task_result = CompressionTaskResult(
        task_id=task_id,
        status=compression_task_status,
        duration=duration,
    )

    if CompressionTaskStatus.FAILED == compression_task_status:
        compression_task_result.error_message = worker_output["error_message"]

    return compression_task_result.dict()
