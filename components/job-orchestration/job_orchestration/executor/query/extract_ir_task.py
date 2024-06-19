import datetime
import os
import signal
import subprocess
import sys
from contextlib import closing
from pathlib import Path
from typing import Any, Dict

from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import Database, QUERY_TASKS_TABLE_NAME, StorageEngine
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.query.celery import app
from job_orchestration.scheduler.job_config import ExtractIrConfig
from job_orchestration.scheduler.scheduler_data import QueryTaskResult, QueryTaskStatus
from .utils import update_query_task_metadata

# Setup logging
logger = get_task_logger(__name__)

@app.task(bind=True)
def extract_ir(
    self: Task,
    job_id: str,
    task_id: int,
    job_config_obj: dict,
    archive_id: str,
    clp_metadata_db_conn_params: dict,
    results_cache_uri: str,
) -> Dict[str, Any]:
    clp_home = Path(os.getenv("CLP_HOME"))
    archive_directory = Path(os.getenv("CLP_ARCHIVE_OUTPUT_DIR"))
    clp_logs_dir = Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = str(os.getenv("CLP_LOGGING_LEVEL"))
    clp_storage_engine = str(os.getenv("CLP_STORAGE_ENGINE"))

    # Setup logging to file
    worker_logs_dir = clp_logs_dir / job_id
    worker_logs_dir.mkdir(exist_ok=True, parents=True)
    set_logging_level(logger, clp_logging_level)
    clo_log_path = worker_logs_dir / f"{task_id}-clo.log"
    clo_log_file = open(clo_log_path, "w")

    logger.info(f"Started extract IR task for job {job_id}")

    extract_ir_config = ExtractIrConfig.parse_obj(job_config_obj)
    sql_adapter = SQL_Adapter(Database.parse_obj(clp_metadata_db_conn_params))

    start_time = datetime.datetime.now()
    search_status = QueryTaskStatus.RUNNING
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        update_query_task_metadata(
            db_cursor, task_id, dict(status=search_status, start_time=start_time)
        )
        db_conn.commit()

    logger.info(f'Running Placeholder task for job {job_id}')
    logger.info(f'Arguments: split_id: {extract_ir_config.file_split_id}, msg_ix: {extract_ir_config.msg_ix}')

    # Mark job succeed
    search_status = QueryTaskStatus.SUCCEEDED

    # Close log files
    clo_log_file.close()
    duration = (datetime.datetime.now() - start_time).total_seconds()

    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        update_query_task_metadata(
            db_cursor, task_id, dict(status=search_status, start_time=start_time, duration=duration)
        )
        db_conn.commit()

    extract_ir_task_result = QueryTaskResult(
        status=search_status,
        task_id=task_id,
        duration=duration,
    )

    if QueryTaskStatus.FAILED == search_status:
        extract_ir_task_result.error_log_path = str(clo_log_path)
    logger.info(f'Finished Placeholder task for job {job_id}')
    return extract_ir_task_result.dict()
