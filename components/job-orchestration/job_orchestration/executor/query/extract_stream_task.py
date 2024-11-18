import datetime
import os
from pathlib import Path
from typing import Any, Dict, List, Optional

from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import Database, StorageEngine
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.query.celery import app
from job_orchestration.executor.query.utils import (
    report_command_creation_failure,
    run_query_task,
)
from job_orchestration.scheduler.job_config import ExtractIrJobConfig, ExtractJsonJobConfig
from job_orchestration.scheduler.scheduler_data import QueryTaskStatus

# Setup logging
logger = get_task_logger(__name__)


def make_command(
    storage_engine: str,
    clp_home: Path,
    archives_dir: Path,
    archive_id: str,
    stream_output_dir: Path,
    job_config: dict,
    results_cache_uri: str,
    stream_collection_name: str,
) -> Optional[List[str]]:
    if StorageEngine.CLP == storage_engine:
        logger.info("Starting IR extraction")
        extract_ir_config = ExtractIrJobConfig.parse_obj(job_config)
        if not extract_ir_config.file_split_id:
            logger.error("file_split_id not supplied")
            return None
        command = [
            str(clp_home / "bin" / "clo"),
            "i",
            str(archives_dir / archive_id),
            extract_ir_config.file_split_id,
            str(stream_output_dir),
            results_cache_uri,
            stream_collection_name,
        ]
        if extract_ir_config.target_uncompressed_size is not None:
            command.append("--target-size")
            command.append(str(extract_ir_config.target_uncompressed_size))
    elif StorageEngine.CLP_S == storage_engine:
        logger.info("Starting JSON extraction")
        extract_json_config = ExtractJsonJobConfig.parse_obj(job_config)
        command = [
            str(clp_home / "bin" / "clp-s"),
            "x",
            str(archives_dir),
            str(stream_output_dir),
            "--ordered",
            "--archive-id",
            archive_id,
            "--mongodb-uri",
            results_cache_uri,
            "--mongodb-collection",
            stream_collection_name,
        ]
        if extract_json_config.target_chunk_size is not None:
            command.append("--ordered-chunk-size")
            command.append(str(extract_json_config.target_chunk_size))
    else:
        logger.error(f"Unsupported storage engine {storage_engine}")
        return None

    return command


@app.task(bind=True)
def extract_stream(
    self: Task,
    job_id: str,
    task_id: int,
    job_config: dict,
    archive_id: str,
    clp_metadata_db_conn_params: dict,
    results_cache_uri: str,
) -> Dict[str, Any]:
    task_name = "Stream Extraction"

    # Setup logging to file
    clp_logs_dir = Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = os.getenv("CLP_LOGGING_LEVEL")
    set_logging_level(logger, clp_logging_level)

    logger.info(f"Started {task_name} task for job {job_id}")

    start_time = datetime.datetime.now()
    task_status: QueryTaskStatus
    sql_adapter = SQL_Adapter(Database.parse_obj(clp_metadata_db_conn_params))

    # Make task_command
    clp_home = Path(os.getenv("CLP_HOME"))
    archive_directory = Path(os.getenv("CLP_ARCHIVE_OUTPUT_DIR"))
    clp_storage_engine = os.getenv("CLP_STORAGE_ENGINE")
    stream_output_dir = Path(os.getenv("CLP_STREAM_OUTPUT_DIR"))
    stream_collection_name = os.getenv("CLP_STREAM_COLLECTION_NAME")

    task_command = make_command(
        storage_engine=clp_storage_engine,
        clp_home=clp_home,
        archives_dir=archive_directory,
        archive_id=archive_id,
        stream_output_dir=stream_output_dir,
        job_config=job_config,
        results_cache_uri=results_cache_uri,
        stream_collection_name=stream_collection_name,
    )
    if not task_command:
        return report_command_creation_failure(
            sql_adapter=sql_adapter,
            logger=logger,
            task_name=task_name,
            task_id=task_id,
            start_time=start_time,
        )

    return run_query_task(
        sql_adapter=sql_adapter,
        logger=logger,
        clp_logs_dir=clp_logs_dir,
        task_command=task_command,
        task_name=task_name,
        job_id=job_id,
        task_id=task_id,
        start_time=start_time,
    )
