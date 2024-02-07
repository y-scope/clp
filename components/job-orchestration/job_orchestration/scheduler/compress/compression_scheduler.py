import argparse
import datetime
import logging
import os
import sys
import time
from contextlib import closing
from pathlib import Path

import celery
import msgpack
import zstandard
from clp_package_utils.general import CONTAINER_INPUT_LOGS_ROOT_DIR
from clp_py_utils.clp_config import (
    CLPConfig,
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_TASKS_TABLE_NAME,
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.compression import validate_path_and_get_info
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.compress.fs_compression_task import compress
from job_orchestration.scheduler.compress.partition import PathsToCompressBuffer
from job_orchestration.scheduler.constants import CompressionJobStatus, CompressionTaskStatus
from job_orchestration.scheduler.job_config import ClpIoConfig
from job_orchestration.scheduler.scheduler_data import (
    CompressionJob,
    CompressionTaskFailureResult,
    CompressionTaskSuccessResult,
)
from pydantic import ValidationError

# Setup logging
logger = get_logger("compression_scheduler")

scheduled_jobs = {}


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

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f"""
    UPDATE {COMPRESSION_TASKS_TABLE_NAME}
    SET {", ".join(field_set_expressions)}
    WHERE id={task_id}
    """
    db_cursor.execute(query)


def update_compression_job_metadata(db_cursor, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f"""
    UPDATE {COMPRESSION_JOBS_TABLE_NAME}
    SET {", ".join(field_set_expressions)}
    WHERE id={job_id}
    """
    db_cursor.execute(query)


def search_and_schedule_new_tasks(db_conn, db_cursor, clp_metadata_db_connection_config):
    """
    For all jobs with PENDING status, split the job into tasks and schedule them.
    """
    global scheduled_jobs

    logger.debug("Search and schedule new tasks")

    zstd_dctx = zstandard.ZstdDecompressor()
    zstd_cctx = zstandard.ZstdCompressor(level=3)

    # Poll for new compression jobs
    jobs = fetch_new_jobs(db_cursor)
    db_conn.commit()
    for job_row in jobs:
        job_id = job_row["id"]
        clp_io_config = ClpIoConfig.parse_obj(
            msgpack.unpackb(zstd_dctx.decompress(job_row["clp_config"]))
        )

        paths_to_compress_buffer = PathsToCompressBuffer(
            maintain_file_ordering=False,
            empty_directories_allowed=True,
            scheduling_job_id=job_id,
            zstd_cctx=zstd_cctx,
            clp_io_config=clp_io_config,
            clp_metadata_db_connection_config=clp_metadata_db_connection_config,
        )

        with open(Path(clp_io_config.input.list_path).resolve(), "r") as f:
            for path_idx, path in enumerate(f, start=1):
                stripped_path = path.strip()
                if "" == stripped_path:
                    # Skip empty paths
                    continue
                path = Path(stripped_path)

                try:
                    file, empty_directory = validate_path_and_get_info(
                        CONTAINER_INPUT_LOGS_ROOT_DIR, path
                    )
                except ValueError as ex:
                    logger.error(str(ex))
                    continue

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
                            logger.error(str(ex))
                            continue

                        if file:
                            paths_to_compress_buffer.add_file(file)
                        elif empty_directory:
                            paths_to_compress_buffer.add_empty_directory(empty_directory)

                if path_idx % 10000 == 0:
                    db_conn.commit()

        paths_to_compress_buffer.flush()
        tasks = paths_to_compress_buffer.get_tasks()
        partition_info = paths_to_compress_buffer.get_partition_info()

        if len(tasks) == 0:
            logger.warning(f"No tasks were created for job {job_id}")
            update_compression_job_metadata(
                db_cursor,
                job_id,
                {
                    "status": CompressionJobStatus.FAILED,
                    "status_msg": "invalid input path",
                },
            )
            db_conn.commit()
            continue

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
            task["task_id"] = db_cursor.lastrowid
            task_instances.append(compress.s(**task))
        db_conn.commit()
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
        num_tasks_completed = 0
        uncompressed_size = 0
        compressed_size = 0
        error_message = ""

        try:
            returned_results = get_results_or_timeout(job.async_task_result)
            if returned_results is None:
                continue

            duration = (datetime.datetime.now() - job.start_time).total_seconds()
            # Check for finished jobs
            for task_result in returned_results:
                if not task_result["status"] == CompressionTaskStatus.SUCCEEDED:
                    task_result = CompressionTaskFailureResult.parse_obj(task_result)
                    job_success = False
                    error_message += f"task {task_result.task_id}: {task_result.error_message}\n"
                    update_compression_task_metadata(
                        db_cursor,
                        task_result.task_id,
                        dict(
                            start_time=task_result.start_time,
                            status=task_result.status,
                            duration=task_result.duration,
                        ),
                    )
                    logger.error(
                        f"Compression task job-{job_id}-task-{task_result.task_id} failed with"
                        f" error: {task_result.error_message}."
                    )
                else:
                    task_result = CompressionTaskSuccessResult.parse_obj(task_result)
                    num_tasks_completed += 1
                    uncompressed_size += task_result.total_uncompressed_size
                    compressed_size += task_result.total_compressed_size
                    update_compression_task_metadata(
                        db_cursor,
                        task_result.task_id,
                        dict(
                            start_time=task_result.start_time,
                            status=task_result.status,
                            partition_uncompressed_size=task_result.total_uncompressed_size,
                            partition_compressed_size=task_result.total_compressed_size,
                            duration=task_result.duration,
                        ),
                    )
                    logger.info(
                        f"Compression task job-{job_id}-task-{task_result.task_id} completed in"
                        f" {task_result.duration} second(s)."
                    )
        except Exception as e:
            logger.error(f"Error while getting results for job {job_id}: {e}")
            job_success = False

        db_conn.commit()

        if job_success:
            logger.info(f"Job {job_id} succeeded.")
            update_compression_job_metadata(
                db_cursor,
                job_id,
                dict(
                    status=CompressionJobStatus.SUCCEEDED,
                    duration=duration,
                    uncompressed_size=uncompressed_size,
                    compressed_size=compressed_size,
                    num_tasks_completed=num_tasks_completed,
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
                    num_tasks_completed=num_tasks_completed,
                ),
            )
        db_conn.commit()

        jobs_to_delete.append(job_id)

    for job_id in jobs_to_delete:
        del scheduled_jobs[job_id]


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

    # Load configuration
    config_path = Path(args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
        return -1
    except Exception as ex:
        logger.error(ex)
        # read_yaml_config_file already logs the parsing error inside
        return -1

    logger.info("Starting compression scheduler")
    sql_adapter = SQL_Adapter(clp_config.database)

    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        # Start Job Processing Loop
        while True:
            try:
                search_and_schedule_new_tasks(
                    db_conn,
                    db_cursor,
                    sql_adapter.database_config.get_clp_connection_params_and_type(True),
                )
                poll_running_jobs(db_conn, db_cursor)
                time.sleep(clp_config.compression_scheduler.jobs_poll_delay)
            except KeyboardInterrupt:
                logger.info("Gracefully shutting down")
                return -1
            except Exception:
                logger.exception(f"Error in scheduling.")
                return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
