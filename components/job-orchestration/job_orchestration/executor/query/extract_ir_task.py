import datetime
import os
import subprocess
from pathlib import Path
from typing import Any, Dict

from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import Database, StorageEngine
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.query.celery import app
from job_orchestration.scheduler.job_config import ExtractIrJobConfig
from job_orchestration.scheduler.scheduler_data import QueryTaskResult, QueryTaskStatus

from .utils import get_logger_file_path, generate_final_task_results, update_query_task_metadata

# Setup logging
logger = get_task_logger(__name__)


def make_command(
    storage_engine: str,
    clp_home: Path,
    archives_dir: Path,
    ir_output_dir: Path,
    archive_id: str,
    extract_ir_config: ExtractIrJobConfig,
    results_cache_uri: str,
    results_collection: str,
):
    if StorageEngine.CLP == storage_engine:
        if not extract_ir_config.file_split_id:
            raise ValueError(f"file_split_id not supplied")
        command = [
            str(clp_home / "bin" / "clo"),
            "i",
            str(archives_dir / archive_id),
            extract_ir_config.file_split_id,
            str(ir_output_dir),
            results_cache_uri,
            results_collection,
        ]
        if extract_ir_config.target_size is not None:
            command.append("--target-size")
            command.append(extract_ir_config.target_size)
    else:
        raise ValueError(f"Unsupported storage engine {storage_engine}")

    return command


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
    ir_directory = Path(os.getenv("CLP_IR_OUTPUT_DIR"))
    clp_logs_dir = Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = str(os.getenv("CLP_LOGGING_LEVEL"))
    clp_storage_engine = str(os.getenv("CLP_STORAGE_ENGINE"))

    ir_collection = str(os.getenv("CLP_IR_COLLECTION"))

    # Setup logging to file
    set_logging_level(logger, clp_logging_level)
    clo_log_path = get_logger_file_path(clp_logs_dir, job_id, task_id)
    clo_log_file = open(clo_log_path, "w")

    logger.info(f"Started IR extraction task for job {job_id}")

    extract_ir_config = ExtractIrJobConfig.parse_obj(job_config_obj)
    sql_adapter = SQL_Adapter(Database.parse_obj(clp_metadata_db_conn_params))

    start_time = datetime.datetime.now()
    task_status: QueryTaskStatus
    try:
        task_command = make_command(
            storage_engine=clp_storage_engine,
            clp_home=clp_home,
            archives_dir=archive_directory,
            ir_output_dir=ir_directory,
            archive_id=archive_id,
            extract_ir_config=extract_ir_config,
            results_cache_uri=results_cache_uri,
            results_collection=ir_collection,
        )
    except ValueError as e:
        error_message = f"Error creating IR extraction command: {e}"
        logger.error(error_message)
        clo_log_file.write(error_message)

        task_status = QueryTaskStatus.FAILED
        update_query_task_metadata(
            sql_adapter,
            task_id,
            dict(status=task_status, duration=0, start_time=start_time),
        )

        clo_log_file.close()
        return QueryTaskResult(
            task_id=task_id,
            status=task_status,
            duration=0,
            error_log_path=str(clo_log_path),
        ).dict()

    task_status = QueryTaskStatus.RUNNING
    update_query_task_metadata(sql_adapter, task_id, dict(status=task_status, start_time=start_time))

    logger.info(f'Running: {" ".join(task_command)}')
    extract_proc = subprocess.Popen(
        task_command,
        preexec_fn=os.setpgrp,
        close_fds=True,
        stdout=clo_log_file,
        stderr=clo_log_file,
    )

    logger.info("Waiting for IR extraction to finish")
    # communicate is equivalent to wait in this case, but avoids deadlocks if we switch to piping
    # stdout/stderr in the future.
    extract_proc.communicate()
    return_code = extract_proc.returncode
    if 0 != return_code:
        task_status = QueryTaskStatus.FAILED
        logger.error(f"IR extraction task {task_id} failed for job {job_id} - return_code={return_code}")
    else:
        task_status = QueryTaskStatus.SUCCEEDED
        logger.info(f"IR extraction task {task_id} completed for job {job_id}")

    clo_log_file.close()
    duration = (datetime.datetime.now() - start_time).total_seconds()

    update_query_task_metadata(
        sql_adapter, task_id, dict(status=task_status, start_time=start_time, duration=duration)
    )

    return generate_final_task_results(task_id, task_status, duration, clo_log_path)
