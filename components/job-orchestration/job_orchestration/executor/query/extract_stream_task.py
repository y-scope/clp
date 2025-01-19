import datetime
import json
import os
from pathlib import Path
from typing import Any, Dict, List, Optional

from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import Database, S3Config, StorageEngine, StorageType, WorkerConfig
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.s3_utils import s3_put
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.query.celery import app
from job_orchestration.executor.query.utils import (
    report_task_failure,
    run_query_task,
)
from job_orchestration.executor.utils import load_worker_config
from job_orchestration.scheduler.job_config import ExtractIrJobConfig, ExtractJsonJobConfig
from job_orchestration.scheduler.scheduler_data import QueryTaskStatus

# Setup logging
logger = get_task_logger(__name__)


def make_command(
    clp_home: Path,
    worker_config: WorkerConfig,
    archive_id: str,
    job_config: dict,
    results_cache_uri: str,
    print_stream_stats: bool,
) -> Optional[List[str]]:
    storage_engine = worker_config.package.storage_engine
    archives_dir = worker_config.archive_output.get_directory()
    stream_output_dir = worker_config.stream_output.get_directory()
    stream_collection_name = worker_config.stream_collection_name

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
        if print_stream_stats:
            command.append("--print-ir-stats")
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
            command.append("--target-ordered-chunk-size")
            command.append(str(extract_json_config.target_chunk_size))
        if print_stream_stats:
            command.append("--print-ordered-chunk-stats")
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

    # Load configuration
    clp_config_path = Path(os.getenv("CLP_CONFIG_PATH"))
    worker_config = load_worker_config(clp_config_path, logger)
    if worker_config is None:
        return report_task_failure(
            sql_adapter=sql_adapter,
            task_id=task_id,
            start_time=start_time,
        )

    if worker_config.archive_output.storage.type == StorageType.S3:
        logger.error(f"Stream extraction is not supported for the S3 storage type")
        return report_task_failure(
            sql_adapter=sql_adapter,
            task_id=task_id,
            start_time=start_time,
        )

    # Make task_command
    clp_home = Path(os.getenv("CLP_HOME"))

    # Get S3 config
    s3_config: S3Config
    enable_s3_upload = False
    storage_config = worker_config.stream_output.storage
    if StorageType.S3 == storage_config.type:
        s3_config = storage_config.s3_config
        enable_s3_upload = True

    task_command = make_command(
        clp_home=clp_home,
        worker_config=worker_config,
        archive_id=archive_id,
        job_config=job_config,
        results_cache_uri=results_cache_uri,
        print_stream_stats=enable_s3_upload,
    )
    if not task_command:
        logger.error(f"Error creating {task_name} command")
        return report_task_failure(
            sql_adapter=sql_adapter,
            task_id=task_id,
            start_time=start_time,
        )

    task_results, task_stdout_str = run_query_task(
        sql_adapter=sql_adapter,
        logger=logger,
        clp_logs_dir=clp_logs_dir,
        task_command=task_command,
        env_vars=None,
        task_name=task_name,
        job_id=job_id,
        task_id=task_id,
        start_time=start_time,
    )

    if enable_s3_upload and QueryTaskStatus.SUCCEEDED == task_results.status:
        logger.info(f"Uploading streams to S3...")

        upload_error = False
        for line in task_stdout_str.splitlines():
            try:
                stream_stats = json.loads(line)
            except json.decoder.JSONDecodeError:
                logger.exception(f"`{line}` cannot be decoded as JSON")
                upload_error = True
                continue

            stream_path_str = stream_stats.get("path")
            if stream_path_str is None:
                logger.error(f"`path` is not a valid key in `{line}`")
                upload_error = True
                continue

            stream_path = Path(stream_path_str)

            # If we've had a single upload error, we don't want to try uploading any other streams
            # since that may unnecessarily slow down the task and generate a lot of extraneous
            # output.
            if not upload_error:
                stream_name = stream_path.name
                logger.info(f"Uploading stream {stream_name} to S3...")

                try:
                    s3_put(s3_config, stream_path, stream_name)
                    logger.info(f"Finished uploading stream {stream_name} to S3.")
                except Exception as err:
                    logger.error(f"Failed to upload stream {stream_name}: {err}")
                    upload_error = True

            stream_path.unlink()

        if upload_error:
            task_results.status = QueryTaskStatus.FAILED
            task_results.error_log_path = str(os.getenv("CLP_WORKER_LOG_PATH"))
        else:
            logger.info(f"Finished uploading streams.")

    return task_results.dict()
