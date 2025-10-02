import argparse
import datetime
import logging
import os
import signal
import sys
import time
from contextlib import closing
from pathlib import Path
from typing import Any, Dict, List, Optional, Set

import brotli
import celery
import msgpack
from clp_package_utils.general import CONTAINER_INPUT_LOGS_ROOT_DIR
from clp_py_utils.clp_config import (
    CLPConfig,
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_TASKS_TABLE_NAME,
    StorageEngine,
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.clp_metadata_db_utils import (
    add_dataset,
    fetch_existing_datasets,
    get_tags_table_name,
)
from clp_py_utils.compression import validate_path_and_get_info
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.s3_utils import s3_get_object_metadata
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.compress.celery_compress import compress
from job_orchestration.scheduler.compress.partition import PathsToCompressBuffer
from job_orchestration.scheduler.constants import (
    CompressionJobStatus,
    CompressionTaskStatus,
    SchedulerType,
)
from job_orchestration.scheduler.job_config import (
    ClpIoConfig,
    FsInputConfig,
    InputType,
    S3InputConfig,
)
from job_orchestration.scheduler.scheduler_data import (
    CompressionJob,
    CompressionTaskResult,
)
from job_orchestration.scheduler.utils import kill_hanging_jobs
from pydantic import ValidationError

# Setup logging
logger = get_logger("compression_scheduler")

scheduled_jobs = {}

received_sigterm = False


def sigterm_handler(signal_number, frame):
    global received_sigterm
    received_sigterm = True
    logger.info("Received SIGTERM.")


def fetch_new_jobs(db_cursor):
    db_cursor.execute(
        f"""
        SELECT id, clp_config, creation_time
        FROM {COMPRESSION_JOBS_TABLE_NAME}
        WHERE status='{CompressionJobStatus.PENDING}'
        """
    )
    return db_cursor.fetchall()


def update_compression_task_metadata(db_cursor, task_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f"{k} = %s" for k in kv.keys()]
    query = f"""
        UPDATE {COMPRESSION_TASKS_TABLE_NAME}
        SET {", ".join(field_set_expressions)}
        WHERE id = %s
    """
    values = list(kv.values()) + [task_id]
    db_cursor.execute(query, values)


def update_compression_job_metadata(db_cursor, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f"{k} = %s" for k in kv.keys()] + ["update_time = CURRENT_TIMESTAMP()"]
    query = f"""
        UPDATE {COMPRESSION_JOBS_TABLE_NAME}
        SET {", ".join(field_set_expressions)}
        WHERE id = %s
    """
    values = list(kv.values()) + [job_id]
    db_cursor.execute(query, values)


def _process_fs_input_paths(
    fs_input_conf: FsInputConfig, paths_to_compress_buffer: PathsToCompressBuffer
) -> List[str]:
    """
    Iterates through all paths in `fs_input_conf`, validates them, and adds metadata for each valid
    path to `paths_to_compress_buffer` as long as an invalid path hasn't yet been encountered.
    :param fs_input_conf:
    :param paths_to_compress_buffer:
    :return: List of error messages about invalid paths.
    """

    paths_ok = True
    invalid_path_messages: List[str] = []

    for path_idx, path in enumerate(fs_input_conf.paths_to_compress, start=1):
        path = Path(path)

        try:
            file, empty_directory = validate_path_and_get_info(CONTAINER_INPUT_LOGS_ROOT_DIR, path)
        except ValueError as ex:
            paths_ok = False
            error_msg = str(ex)
            logger.error(error_msg)
            invalid_path_messages.append(error_msg)
            continue

        if paths_ok:
            if file:
                paths_to_compress_buffer.add_file(file)
            elif empty_directory:
                paths_to_compress_buffer.add_empty_directory(empty_directory)

        if path.is_dir():
            for internal_path in path.rglob("*"):
                try:
                    file, empty_directory = validate_path_and_get_info(
                        CONTAINER_INPUT_LOGS_ROOT_DIR, internal_path
                    )
                except ValueError as ex:
                    paths_ok = False
                    error_msg = str(ex)
                    logger.error(error_msg)
                    invalid_path_messages.append(error_msg)
                    continue

                if paths_ok:
                    if file:
                        paths_to_compress_buffer.add_file(file)
                    elif empty_directory:
                        paths_to_compress_buffer.add_empty_directory(empty_directory)

    return invalid_path_messages


def _process_s3_input(
    s3_input_config: S3InputConfig,
    paths_to_compress_buffer: PathsToCompressBuffer,
) -> None:
    """
    Iterates through all objects under the <bucket>/<key_prefix> specified by s3_input_config,
    and adds their metadata to paths_to_compress_buffer.
    :param s3_input_config:
    :param paths_to_compress_buffer:
    :raises: RuntimeError if input URL doesn't resolve to any objects.
    :raises: Propagates `s3_get_object_metadata`'s exceptions.
    """

    object_metadata_list = s3_get_object_metadata(s3_input_config)
    if len(object_metadata_list) == 0:
        raise RuntimeError("Input URL doesn't resolve to any object")

    for object_metadata in object_metadata_list:
        paths_to_compress_buffer.add_file(object_metadata)


def _write_failed_path_log(
    invalid_path_messages: List[str], logs_directory: Path, job_id: Any
) -> Optional[Path]:
    """
    Writes the error messages in `invalid_path_messages` to a log file,
    `{logs_directory}/user/failed_paths_{job_id}.txt`. The directory will be created if it doesn't
    already exist.
    :param invalid_path_messages:
    :param logs_directory:
    :param job_id:
    :return: Path to the written log file or `None` if error is encountered.
    """

    user_logs_dir = Path(logs_directory) / "user"
    try:
        user_logs_dir.mkdir(parents=True, exist_ok=True)
    except Exception:
        return None

    log_path = user_logs_dir / f"failed_paths_{job_id}.txt"
    try:
        with log_path.open("w", encoding="utf-8") as f:
            timestamp = datetime.datetime.now().isoformat(timespec="seconds")
            f.write(f"Failed input paths log.\nGenerated at {timestamp}.\n\n")
            for msg in invalid_path_messages:
                f.write(f"{msg.rstrip()}\n")
    except Exception:
        return None

    return log_path


def search_and_schedule_new_tasks(
    clp_config: CLPConfig,
    db_conn,
    db_cursor,
    clp_metadata_db_connection_config: Dict[str, Any],
):
    """
    For all jobs with PENDING status, splits the job into tasks and schedules them.
    :param clp_config:
    :param db_conn:
    :param db_cursor:
    :param clp_metadata_db_connection_config:
    """
    global scheduled_jobs

    existing_datasets: Set[str] = set()
    if StorageEngine.CLP_S == clp_config.package.storage_engine:
        existing_datasets = fetch_existing_datasets(
            db_cursor, clp_metadata_db_connection_config["table_prefix"]
        )

    logger.debug("Search and schedule new tasks")

    # Poll for new compression jobs
    jobs = fetch_new_jobs(db_cursor)
    db_conn.commit()
    for job_row in jobs:
        job_id = job_row["id"]
        clp_io_config = ClpIoConfig.model_validate(
            msgpack.unpackb(brotli.decompress(job_row["clp_config"]))
        )
        input_config = clp_io_config.input

        table_prefix = clp_metadata_db_connection_config["table_prefix"]
        dataset = input_config.dataset

        if dataset is not None and dataset not in existing_datasets:
            add_dataset(
                db_conn,
                db_cursor,
                table_prefix,
                dataset,
                clp_config.archive_output,
            )

            # NOTE: This assumes we never delete a dataset when compression jobs are being scheduled
            existing_datasets.add(dataset)

        paths_to_compress_buffer = PathsToCompressBuffer(
            maintain_file_ordering=False,
            empty_directories_allowed=True,
            scheduling_job_id=job_id,
            clp_io_config=clp_io_config,
            clp_metadata_db_connection_config=clp_metadata_db_connection_config,
        )

        input_type = input_config.type
        if input_type == InputType.FS.value:
            invalid_path_messages = _process_fs_input_paths(input_config, paths_to_compress_buffer)
            if len(invalid_path_messages) > 0:
                base_msg = "At least one of your input paths could not be processed."

                user_log_path = _write_failed_path_log(
                    invalid_path_messages, clp_config.logs_directory, job_id
                )
                if user_log_path is None:
                    error_msg = base_msg + (
                        f" Check the compression scheduler logs in {clp_config.logs_directory} for"
                        " more details."
                    )
                else:
                    error_msg = base_msg + f" Check {user_log_path} for more details."

                update_compression_job_metadata(
                    db_cursor,
                    job_id,
                    {
                        "status": CompressionJobStatus.FAILED,
                        "status_msg": error_msg,
                    },
                )
                db_conn.commit()
                continue
        elif input_type == InputType.S3.value:
            try:
                _process_s3_input(input_config, paths_to_compress_buffer)
            except Exception as err:
                logger.exception("Failed to process S3 input")
                update_compression_job_metadata(
                    db_cursor,
                    job_id,
                    {
                        "status": CompressionJobStatus.FAILED,
                        "status_msg": f"S3 Failure: {err}",
                    },
                )
                db_conn.commit()
                continue
        else:
            logger.error(f"Unsupported input type {input_type}")
            update_compression_job_metadata(
                db_cursor,
                job_id,
                {
                    "status": CompressionJobStatus.FAILED,
                    "status_msg": f"Unsupported input type: {input_type}",
                },
            )
            db_conn.commit()
            continue

        paths_to_compress_buffer.flush()
        tasks = paths_to_compress_buffer.get_tasks()
        partition_info = paths_to_compress_buffer.get_partition_info()

        # Update job metadata
        start_time = datetime.datetime.now()
        update_compression_job_metadata(
            db_cursor,
            job_id,
            {
                "num_tasks": paths_to_compress_buffer.num_tasks,
                "status": CompressionJobStatus.RUNNING,
                "start_time": start_time,
            },
        )
        db_conn.commit()

        tag_ids = None
        if clp_io_config.output.tags:
            tags_table_name = get_tags_table_name(table_prefix, dataset)
            db_cursor.executemany(
                f"INSERT IGNORE INTO {tags_table_name} (tag_name) VALUES (%s)",
                [(tag,) for tag in clp_io_config.output.tags],
            )
            db_conn.commit()
            db_cursor.execute(
                f"SELECT tag_id FROM {tags_table_name} WHERE tag_name IN (%s)"
                % ", ".join(["%s"] * len(clp_io_config.output.tags)),
                clp_io_config.output.tags,
            )
            tag_ids = [tags["tag_id"] for tags in db_cursor.fetchall()]
            db_conn.commit()

        task_instances = []
        for task_idx, task in enumerate(tasks):
            db_cursor.execute(
                f"""
                INSERT INTO {COMPRESSION_TASKS_TABLE_NAME}
                (job_id, partition_original_size, clp_paths_to_compress)
                VALUES({str(job_id)}, {partition_info[task_idx]["partition_original_size"]}, %s)
                """,
                (partition_info[task_idx]["clp_paths_to_compress"],),
            )
            db_conn.commit()
            task["task_id"] = db_cursor.lastrowid
            task["tag_ids"] = tag_ids
            task_instances.append(compress.s(**task))
        tasks_group = celery.group(task_instances)

        job = CompressionJob(
            id=job_id, start_time=start_time, async_task_result=tasks_group.apply_async()
        )
        db_cursor.execute(
            f"""
            UPDATE {COMPRESSION_TASKS_TABLE_NAME}
            SET status='{CompressionTaskStatus.RUNNING}' WHERE job_id={job_id}
            """
        )
        db_conn.commit()

        scheduled_jobs[job_id] = job


def get_results_or_timeout(result):
    try:
        return result.get(timeout=0.1)
    except celery.exceptions.TimeoutError:
        return None


def poll_running_jobs(db_conn, db_cursor):
    """
    Poll for running jobs and update their status.
    """
    global scheduled_jobs

    logger.debug("Poll running jobs")
    jobs_to_delete = []
    for job_id, job in scheduled_jobs.items():
        job_success = True
        duration = 0.0
        error_message = ""

        try:
            returned_results = get_results_or_timeout(job.async_task_result)
            if returned_results is None:
                continue

            duration = (datetime.datetime.now() - job.start_time).total_seconds()
            # Check for finished jobs
            for task_result in returned_results:
                task_result = CompressionTaskResult.model_validate(task_result)
                if task_result.status == CompressionTaskStatus.SUCCEEDED:
                    logger.info(
                        f"Compression task job-{job_id}-task-{task_result.task_id} completed in"
                        f" {task_result.duration} second(s)."
                    )
                else:
                    job_success = False
                    error_message += f"task {task_result.task_id}: {task_result.error_message}\n"
                    logger.error(
                        f"Compression task job-{job_id}-task-{task_result.task_id} failed with"
                        f" error: {task_result.error_message}."
                    )

        except Exception as e:
            logger.error(f"Error while getting results for job {job_id}: {e}")
            job_success = False

        if job_success:
            logger.info(f"Job {job_id} succeeded.")
            update_compression_job_metadata(
                db_cursor,
                job_id,
                dict(
                    status=CompressionJobStatus.SUCCEEDED,
                    duration=duration,
                ),
            )
        else:
            logger.error(f"Job {job_id} failed. See worker logs or status_msg for details.")
            update_compression_job_metadata(
                db_cursor,
                job_id,
                dict(
                    status=CompressionJobStatus.FAILED,
                    status_msg=error_message,
                ),
            )
        db_conn.commit()

        jobs_to_delete.append(job_id)

    for job_id in jobs_to_delete:
        del scheduled_jobs[job_id]

    if received_sigterm and 0 == len(scheduled_jobs):
        logger.info("Recieved SIGTERM and there're no more running jobs. Exiting.")
        sys.exit(0)


def main(argv):
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    args = args_parser.parse_args(argv[1:])

    # Setup logging
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / "compression_scheduler.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    set_logging_level(logger, os.getenv("CLP_LOGGING_LEVEL"))

    # Register the SIGTERM handler
    signal.signal(signal.SIGTERM, sigterm_handler)

    # Load configuration
    config_path = Path(args.config)
    try:
        clp_config = CLPConfig.model_validate(read_yaml_config_file(config_path))
        clp_config.database.load_credentials_from_env()
    except (ValidationError, ValueError) as err:
        logger.error(err)
        return -1
    except Exception:
        logger.exception(f"Failed to initialize {COMPRESSION_SCHEDULER_COMPONENT_NAME}.")
        # read_yaml_config_file already logs the parsing error inside
        return -1

    logger.info(f"Starting {COMPRESSION_SCHEDULER_COMPONENT_NAME}")
    sql_adapter = SQL_Adapter(clp_config.database)

    try:
        killed_jobs = kill_hanging_jobs(sql_adapter, SchedulerType.COMPRESSION)
        if killed_jobs is not None:
            logger.info(f"Killed {len(killed_jobs)} hanging compression jobs.")
    except Exception:
        logger.exception("Failed to kill hanging compression jobs.")
        return -1

    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        clp_metadata_db_connection_config = (
            sql_adapter.database_config.get_clp_connection_params_and_type(True)
        )

        # Start Job Processing Loop
        while True:
            try:
                if not received_sigterm:
                    search_and_schedule_new_tasks(
                        clp_config,
                        db_conn,
                        db_cursor,
                        clp_metadata_db_connection_config,
                    )
                poll_running_jobs(db_conn, db_cursor)
                time.sleep(clp_config.compression_scheduler.jobs_poll_delay)
            except KeyboardInterrupt:
                logger.info("Forcefully shutting down")
                return -1
            except Exception:
                logger.exception(f"Error in scheduling.")
                return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
