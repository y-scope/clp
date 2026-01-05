import argparse
import datetime
import logging
import os
import signal
import sys
import time
from contextlib import closing
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import brotli
import msgpack
from clp_package_utils.general import CONTAINER_INPUT_LOGS_ROOT_DIR
from clp_py_utils.clp_config import (
    ClpConfig,
    ClpDbUserType,
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_TASKS_TABLE_NAME,
    OrchestrationType,
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
from clp_py_utils.sql_adapter import SqlAdapter
from pydantic import ValidationError

from job_orchestration.scheduler.compress.partition import PathsToCompressBuffer
from job_orchestration.scheduler.compress.task_manager.celery_task_manager import CeleryTaskManager
from job_orchestration.scheduler.compress.task_manager.spider_task_manager import SpiderTaskManager
from job_orchestration.scheduler.compress.task_manager.task_manager import TaskManager
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
)
from job_orchestration.scheduler.utils import kill_hanging_jobs


@dataclass
class DbContext:
    """Database context holding a connection, and a cursor created from the connection."""

    connection: Any
    cursor: Any


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


def update_compression_job_metadata(db_context: DbContext, job_id: int, kv: dict[str, str]) -> None:
    """
    Updates compression job metadata in the database.

    :param db_context:
    :param job_id:
    :param kv:
    """
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f"{k} = %s" for k in kv] + ["update_time = CURRENT_TIMESTAMP()"]
    query = f"""
        UPDATE {COMPRESSION_JOBS_TABLE_NAME}
        SET {", ".join(field_set_expressions)}
        WHERE id = %s
    """
    values = [*list(kv.values()), job_id]
    db_context.cursor.execute(query, values)
    db_context.connection.commit()


def _process_fs_input_paths(
    fs_input_conf: FsInputConfig, paths_to_compress_buffer: PathsToCompressBuffer
) -> list[str]:
    """
    Iterates through all paths in `fs_input_conf`, validates them, and adds metadata for each valid
    path to `paths_to_compress_buffer` as long as an invalid path hasn't yet been encountered.

    :param fs_input_conf:
    :param paths_to_compress_buffer:
    :return: List of error messages about invalid paths.
    """
    paths_ok = True
    invalid_path_messages: list[str] = []

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


def _write_user_failure_log(
    title: str,
    content: list[str],
    logs_directory: Path,
    job_id: int,
    filename_suffix: str,
) -> Path | None:
    """
    Writes a user-oriented failure log to
    `{logs_directory}/user/job_{job_id}_{filename_suffix}.txt`. The `{logs_directory}/user`
    directory will be created if it does not already exist.

    :param title:
    :param content:
    :param logs_directory:
    :param job_id:
    :param filename_suffix:
    :return: Path to the written log file relative to `logs_directory`, or `None` on error.
    """
    relative_log_path = Path("user") / f"job_{job_id}_{filename_suffix}.txt"
    user_logs_dir = logs_directory / relative_log_path.parent
    try:
        user_logs_dir.mkdir(parents=True, exist_ok=True)
    except Exception as e:
        logger.error("Failed to create user logs directory: '%s' - %s", user_logs_dir, e)
        return None

    log_path = logs_directory / relative_log_path
    try:
        with log_path.open("w", encoding="utf-8") as f:
            timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")
            f.write(f"{title}\nGenerated at {timestamp}.\n\n")
            for item in content:
                f.write(f"{item.rstrip()}\n")
    except Exception as e:
        logger.error("Failed to write compression failure user log: '%s' - %s", log_path, e)
        return None

    return relative_log_path


def search_and_schedule_new_tasks(
    clp_config: ClpConfig,
    clp_metadata_db_connection_config: dict[str, Any],
    task_manager: TaskManager,
    db_context: DbContext,
) -> None:
    """
    Splits all the jobs with PENDING into tasks and schedules them in batches.

    :param clp_config:
    :param clp_metadata_db_connection_config:
    :param task_manager:
    :param db_context:
    """
    existing_datasets: set[str] = set()
    if StorageEngine.CLP_S == clp_config.package.storage_engine:
        existing_datasets = fetch_existing_datasets(
            db_context.cursor, clp_metadata_db_connection_config["table_prefix"]
        )

    logger.debug("Search and schedule new tasks")

    # Poll for new compression jobs
    jobs = fetch_new_jobs(db_context.cursor)
    # TODO: revisit why we need to commit here. To end long transactions?
    db_context.connection.commit()
    for job_row in jobs:
        job_id = job_row["id"]
        clp_io_config = ClpIoConfig.model_validate(
            msgpack.unpackb(brotli.decompress(job_row["clp_config"]))
        )
        input_config = clp_io_config.input
        table_prefix = clp_metadata_db_connection_config["table_prefix"]
        dataset = input_config.dataset

        _ensure_dataset_exists(
            clp_config,
            db_context,
            table_prefix,
            dataset,
            existing_datasets,
        )

        # Prepare paths buffer
        tag_ids = _get_tag_ids_for_job(db_context, clp_io_config, table_prefix, dataset)
        paths_to_compress_buffer = PathsToCompressBuffer(
            maintain_file_ordering=False,
            empty_directories_allowed=True,
            scheduling_job_id=job_id,
            clp_io_config=clp_io_config,
            clp_metadata_db_connection_config=clp_metadata_db_connection_config,
            tag_ids=tag_ids,
        )

        # Process input paths
        input_type = input_config.type
        if input_type == InputType.FS.value:
            invalid_path_messages = _process_fs_input_paths(input_config, paths_to_compress_buffer)
            if len(invalid_path_messages) > 0:
                user_log_relative_path = _write_user_failure_log(
                    title="Failed input paths log.",
                    content=invalid_path_messages,
                    logs_directory=clp_config.logs_directory,
                    job_id=job_id,
                    filename_suffix="failed_paths",
                )
                if user_log_relative_path is None:
                    err_msg = "Failed to write user log for invalid input paths."
                    raise RuntimeError(err_msg)

                error_msg = (
                    "At least one of your input paths could not be processed."
                    f" See the error log at '{user_log_relative_path}' inside your configured logs"
                    " directory (`logs_directory`) for more details."
                )

                update_compression_job_metadata(
                    db_context,
                    job_id,
                    {
                        "status": CompressionJobStatus.FAILED,
                        "status_msg": error_msg,
                    },
                )
                return
        elif input_type == InputType.S3.value:
            try:
                _process_s3_input(input_config, paths_to_compress_buffer)
            except Exception as err:
                logger.exception("Failed to process S3 input")
                update_compression_job_metadata(
                    db_context,
                    job_id,
                    {
                        "status": CompressionJobStatus.FAILED,
                        "status_msg": f"S3 Failure: {err}",
                    },
                )
                return
        else:
            logger.error(f"Unsupported input type {input_type}")
            update_compression_job_metadata(
                db_context,
                job_id,
                {
                    "status": CompressionJobStatus.FAILED,
                    "status_msg": f"Unsupported input type: {input_type}",
                },
            )
            return
        paths_to_compress_buffer.flush()

        _batch_and_submit_tasks(
            clp_config,
            task_manager,
            db_context,
            job_id,
            paths_to_compress_buffer,
        )


def poll_running_jobs(
    clp_config: ClpConfig, task_manager: TaskManager, db_context: DbContext
) -> None:
    """
    Polls for running jobs, update their status, and dispatch the next batch if needed.

    :param clp_config:
    :param task_manager:
    :param db_context:
    """
    logs_directory = clp_config.logs_directory
    max_concurrent_tasks_per_job = clp_config.compression_scheduler.max_concurrent_tasks_per_job

    logger.debug("Poll running jobs")
    jobs_to_delete = []
    for job_id, job in scheduled_jobs.items():
        job_success = True
        duration = 0.0
        error_messages: list[str] = []
        num_tasks_in_batch = 0

        try:
            returned_results = job.result_handle.get_result()
            if returned_results is None:
                continue

            duration = (
                datetime.datetime.now(datetime.timezone.utc) - job.start_time
            ).total_seconds()
            # Check for finished jobs
            num_tasks_in_batch = len(returned_results)
            for task_result in returned_results:
                if task_result.status == CompressionTaskStatus.SUCCEEDED:
                    logger.info(
                        f"Compression task job-{job_id}-task-{task_result.task_id} completed in"
                        f" {task_result.duration} second(s)."
                    )
                else:
                    job_success = False
                    error_messages.append(
                        f"task {task_result.task_id}: {task_result.error_message}"
                    )
                    logger.error(
                        f"Compression task job-{job_id}-task-{task_result.task_id} failed with"
                        f" error: {task_result.error_message}."
                    )

        except Exception:
            logger.exception("Error while getting results for job %s", job_id)
            job_success = False

        if not job_success:
            _handle_failed_compression_job(logs_directory, db_context, job_id, error_messages)
            jobs_to_delete.append(job_id)
            continue

        job.num_tasks_completed += num_tasks_in_batch

        if len(job.remaining_tasks) > 0:
            _dispatch_next_task_batch(task_manager, db_context, job, max_concurrent_tasks_per_job)
        else:
            # All tasks completed successfully
            _complete_compression_job(db_context, job_id, job.num_tasks_total, duration)
            jobs_to_delete.append(job_id)

    for job_id in jobs_to_delete:
        del scheduled_jobs[job_id]

    if received_sigterm and 0 == len(scheduled_jobs):
        logger.info("Received SIGTERM and there're no more running jobs. Exiting.")
        sys.exit(0)


def main(argv) -> int | None:
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    args = args_parser.parse_args(argv[1:])

    # Set logging level from environment
    set_logging_level(logger, os.getenv("CLP_LOGGING_LEVEL"))

    # Register the SIGTERM handler
    signal.signal(signal.SIGTERM, sigterm_handler)

    # Load configuration
    config_path = Path(args.config)
    try:
        clp_config = ClpConfig.model_validate(read_yaml_config_file(config_path))
        clp_config.database.load_credentials_from_env()
    except (ValidationError, ValueError):
        logger.exception("Failed to load configuration")
        return -1
    except Exception:
        logger.exception(f"Failed to initialize {COMPRESSION_SCHEDULER_COMPONENT_NAME}.")
        # read_yaml_config_file already logs the parsing error inside
        return -1

    logger.info(f"Starting {COMPRESSION_SCHEDULER_COMPONENT_NAME}")
    sql_adapter = SqlAdapter(clp_config.database)

    task_manager: CeleryTaskManager | SpiderTaskManager
    if clp_config.compression_scheduler.type == OrchestrationType.CELERY:
        task_manager = CeleryTaskManager()
    elif clp_config.compression_scheduler.type == OrchestrationType.SPIDER:
        clp_config.database.load_credentials_from_env(ClpDbUserType.SPIDER)
        task_manager = SpiderTaskManager(
            clp_config.database.get_container_url(ClpDbUserType.SPIDER)
        )
    else:
        logger.error(
            f"Unsupported compression scheduler type: {clp_config.compression_scheduler.type}"
        )
        return -1

    try:
        killed_jobs = kill_hanging_jobs(sql_adapter, SchedulerType.COMPRESSION)
        if killed_jobs is not None:
            logger.info(f"Killed {len(killed_jobs)} hanging compression jobs.")
    except Exception:
        logger.exception("Failed to kill hanging compression jobs.")
        return -1

    with (
        closing(sql_adapter.create_connection(True)) as db_conn,
        closing(db_conn.cursor(dictionary=True)) as db_cursor,
    ):
        db_context = DbContext(connection=db_conn, cursor=db_cursor)
        clp_metadata_db_connection_config = (
            sql_adapter.database_config.get_clp_connection_params_and_type(True)
        )

        # Start Job Processing Loop
        while True:
            try:
                if not received_sigterm:
                    search_and_schedule_new_tasks(
                        clp_config,
                        clp_metadata_db_connection_config,
                        task_manager,
                        db_context,
                    )
                poll_running_jobs(
                    clp_config,
                    task_manager,
                    db_context,
                )
                time.sleep(clp_config.compression_scheduler.jobs_poll_delay)
            except KeyboardInterrupt:
                logger.info("Forcefully shutting down")
                return -1
            except Exception:
                logger.exception("Error in scheduling.")
                return -1


def _batch_tasks(
    tasks: list[dict[str, Any]],
    partition_info: list[dict[str, Any]],
    max_concurrent_tasks_per_job: int,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]], list[dict[str, Any]], list[dict[str, Any]]]:
    """
    Batches tasks into submission and remaining groups.

    :param tasks:
    :param partition_info:
    :param max_concurrent_tasks_per_job:
    :return: Tuple of (tasks_to_submit, remaining_tasks, partition_info_to_submit,
    remaining_partition_info).
    """
    # Skip partitioning if max_concurrent_tasks_per_job is 0.
    if max_concurrent_tasks_per_job > 0 and len(tasks) > max_concurrent_tasks_per_job:
        tasks_to_submit = tasks[:max_concurrent_tasks_per_job]
        remaining_tasks = tasks[max_concurrent_tasks_per_job:]
        partition_info_to_submit = partition_info[:max_concurrent_tasks_per_job]
        remaining_partition_info = partition_info[max_concurrent_tasks_per_job:]
    else:
        tasks_to_submit = tasks
        remaining_tasks = []
        partition_info_to_submit = partition_info
        remaining_partition_info = []
    return tasks_to_submit, remaining_tasks, partition_info_to_submit, remaining_partition_info


def _batch_and_submit_tasks(
    clp_config: ClpConfig,
    task_manager: TaskManager,
    db_context: DbContext,
    job_id: int,
    paths_to_compress_buffer: PathsToCompressBuffer,
) -> None:
    """
    Batches tasks from paths_to_compress_buffer and submits them to the task manager.

    :param clp_config:
    :param task_manager:
    :param db_context:
    :param job_id:
    :param paths_to_compress_buffer:
    """
    start_time = datetime.datetime.now(datetime.timezone.utc)
    update_compression_job_metadata(
        db_context,
        job_id,
        {
            "num_tasks": paths_to_compress_buffer.num_tasks,
            "status": CompressionJobStatus.RUNNING,
            "start_time": start_time,
        },
    )

    # Compute the first batch of tasks and submit them to the task manager
    max_concurrent_tasks_per_job = clp_config.compression_scheduler.max_concurrent_tasks_per_job
    tasks = paths_to_compress_buffer.get_tasks()
    partition_info = paths_to_compress_buffer.get_partition_info()
    tasks_to_submit, remaining_tasks, partition_info_to_submit, remaining_partition_info = (
        _batch_tasks(tasks, partition_info, max_concurrent_tasks_per_job)
    )
    _insert_tasks_to_db(db_context, job_id, tasks_to_submit, partition_info_to_submit)
    result_handle = task_manager.submit(tasks_to_submit)

    job = CompressionJob(
        id=job_id,
        start_time=start_time,
        result_handle=result_handle,
        num_tasks_total=paths_to_compress_buffer.num_tasks,
        num_tasks_completed=0,
        remaining_tasks=remaining_tasks,
        remaining_partition_info=remaining_partition_info,
    )
    scheduled_jobs[job_id] = job

    _update_tasks_status_to_running(db_context, tasks_to_submit)
    logger.info(
        "Dispatched job %s with %s tasks (%s remaining).",
        job_id,
        len(tasks_to_submit),
        len(remaining_tasks),
    )


def _complete_compression_job(
    db_context: DbContext, job_id: int, num_tasks_total: int, duration: float
) -> None:
    """
    Marks a compression job as successfully completed.

    :param db_context:
    :param job_id:
    :param num_tasks_total:
    :param duration:
    """
    logger.info("Job %s succeeded (%s tasks completed).", job_id, num_tasks_total)
    update_compression_job_metadata(
        db_context,
        job_id,
        {
            "status": CompressionJobStatus.SUCCEEDED,
            "duration": duration,
        },
    )


def _dispatch_next_task_batch(
    task_manager: TaskManager,
    db_context: DbContext,
    job: CompressionJob,
    max_concurrent_tasks_per_job: int,
) -> None:
    """
    Dispatches the next batch of tasks for a compression job.

    :param task_manager:
    :param db_context:
    :param job:
    :param max_concurrent_tasks_per_job:
    """
    job_id = job.id
    logger.info(
        "Job %s batch completed. Dispatching next batch (%s/%s tasks completed).",
        job_id,
        job.num_tasks_completed,
        job.num_tasks_total,
    )

    # Prepare the next batch of tasks
    tasks_to_submit, job.remaining_tasks, partition_info_to_submit, job.remaining_partition_info = (
        _batch_tasks(
            job.remaining_tasks, job.remaining_partition_info, max_concurrent_tasks_per_job
        )
    )

    # Insert tasks into the database and submit them
    _insert_tasks_to_db(db_context, job_id, tasks_to_submit, partition_info_to_submit)
    job.result_handle = task_manager.submit(tasks_to_submit)
    _update_tasks_status_to_running(db_context, tasks_to_submit)
    logger.info(
        "Dispatched next batch for job %s with %s tasks (%s remaining).",
        job_id,
        len(tasks_to_submit),
        len(job.remaining_tasks),
    )


def _ensure_dataset_exists(
    clp_config: ClpConfig,
    db_context: DbContext,
    table_prefix: str,
    dataset: str,
    existing_datasets: set[str],
) -> None:
    """
    Ensures that the specified dataset exists in the metadata database.

    :param db_context:
    :param table_prefix:
    :param dataset:
    :param clp_config:
    :param existing_datasets:
    """
    if dataset is not None and dataset not in existing_datasets:
        add_dataset(
            db_context.connection,
            db_context.cursor,
            table_prefix,
            dataset,
            clp_config.archive_output,
        )
        existing_datasets.add(dataset)


def _get_tag_ids_for_job(
    db_context: DbContext, clp_io_config: ClpIoConfig, table_prefix: str, dataset: str
) -> list[int]:
    """
    Gets tag IDs for a compression job.

    :param db_context:
    :param clp_io_config:
    :param table_prefix:
    :param dataset:
    :return: List of tag IDs.
    """
    tag_ids = []
    if clp_io_config.output.tags:
        tags_table_name = get_tags_table_name(table_prefix, dataset)
        db_context.cursor.executemany(
            f"INSERT IGNORE INTO {tags_table_name} (tag_name) VALUES (%s)",
            [(tag,) for tag in clp_io_config.output.tags],
        )
        db_context.connection.commit()
        db_context.cursor.execute(
            f"SELECT tag_id FROM {tags_table_name} WHERE tag_name IN (%s)"
            % ", ".join(["%s"] * len(clp_io_config.output.tags)),
            clp_io_config.output.tags,
        )
        tag_ids = [tags["tag_id"] for tags in db_context.cursor.fetchall()]
        db_context.connection.commit()
    return tag_ids


def _handle_failed_compression_job(
    logs_directory: Path, db_context: DbContext, job_id: int, error_messages: list[str]
) -> None:
    """
    Handles a failed compression job by writing error logs and updating metadata.

    :param logs_directory:
    :param db_context:
    :param job_id:
    :param error_messages:
    """
    logger.error("Job %s failed. See worker logs or status_msg for details.", job_id)

    error_log_relative_path = _write_user_failure_log(
        title="Compression task errors.",
        content=error_messages,
        logs_directory=logs_directory,
        job_id=job_id,
        filename_suffix="task_errors",
    )
    if error_log_relative_path is None:
        err_msg = "Failed to write user log for failed compression job."
        raise RuntimeError(err_msg)

    error_msg = (
        "One or more compression tasks failed."
        f" See the error log at '{error_log_relative_path}' inside your configured logs"
        " directory (`logs_directory`) for more details."
    )

    update_compression_job_metadata(
        db_context,
        job_id,
        {
            "status": CompressionJobStatus.FAILED,
            "status_msg": error_msg,
        },
    )


def _insert_tasks_to_db(
    db_context: DbContext,
    job_id: int,
    tasks_to_submit: list[dict[str, Any]],
    partition_info_to_submit: list[dict[str, Any]],
) -> None:
    """
    Inserts tasks into the database and assign task IDs.

    :param db_context:
    :param job_id:
    :param tasks_to_submit:
    :param partition_info_to_submit:
    """
    for task_idx, task in enumerate(tasks_to_submit):
        db_context.cursor.execute(
            f"INSERT INTO {COMPRESSION_TASKS_TABLE_NAME}"  # noqa: S608
            " (job_id, partition_original_size, clp_paths_to_compress)"
            " VALUES (%s, %s, %s)",
            (
                job_id,
                partition_info_to_submit[task_idx]["partition_original_size"],
                partition_info_to_submit[task_idx]["clp_paths_to_compress"],
            ),
        )
        db_context.connection.commit()
        task["task_id"] = db_context.cursor.lastrowid


def _update_tasks_status_to_running(
    db_context: DbContext, tasks_to_submit: list[dict[str, Any]]
) -> None:
    """
    Updates the status of submitted tasks to RUNNING.

    :param db_context:
    :param tasks_to_submit:
    """
    if len(tasks_to_submit) > 0:
        task_ids = [task["task_id"] for task in tasks_to_submit]
        task_id_placeholder = ", ".join(["%s"] * len(task_ids))
        db_context.cursor.execute(
            f"UPDATE {COMPRESSION_TASKS_TABLE_NAME}"  # noqa: S608
            f" SET status = %s WHERE id IN ({task_id_placeholder})",
            (CompressionTaskStatus.RUNNING, *task_ids),
        )
        db_context.connection.commit()


if "__main__" == __name__:
    sys.exit(main(sys.argv))
