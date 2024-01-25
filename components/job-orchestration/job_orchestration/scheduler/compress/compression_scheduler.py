import argparse
import celery
import datetime
import logging
import os
import pathlib
import sys
import time
from contextlib import closing

import msgpack
import zstandard
from pydantic import ValidationError

from clp_package_utils.general import CONTAINER_INPUT_LOGS_ROOT_DIR
from clp_py_utils.clp_config import (
    CLPConfig,
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_TASKS_TABLE_NAME
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.compression import validate_path_and_get_info
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.compress.fs_compression_task import compress
from job_orchestration.scheduler.constants import \
    CompressionJobStatus, \
    CompressionTaskStatus
from job_orchestration.scheduler.compress.partition import PathsToCompressBuffer
from job_orchestration.scheduler.scheduler_data import CompressionJob

# Setup logging
logger = get_logger("compression-job-handler")

scheduled_jobs = {}


def fetch_new_jobs(db_cursor):
    db_cursor.execute(f"""
        SELECT id, clp_config, creation_time
        FROM {COMPRESSION_JOBS_TABLE_NAME}
        WHERE status='{CompressionJobStatus.SCHEDULING}'
    """)
    return db_cursor.fetchall()


def update_compression_task_metadata(db_cursor, task_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f'UPDATE {COMPRESSION_TASKS_TABLE_NAME} SET {", ".join(field_set_expressions)} WHERE id={task_id};'
    db_cursor.execute(query)


def update_compression_job_metadata(db_cursor, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f'UPDATE {COMPRESSION_JOBS_TABLE_NAME} SET {", ".join(field_set_expressions)} WHERE id={job_id};'
    db_cursor.execute(query)


def search_and_schedule_new_tasks(db_conn, db_cursor, database_connection_params):
    """
    For all jobs with SUBMITTED status, split the job into tasks and schedule them.
    """
    global scheduled_jobs

    logger.debug('Search and schedule new tasks')

    zstd_dctx = zstandard.ZstdDecompressor()
    zstd_cctx = zstandard.ZstdCompressor(level=3)

    # Poll for new compression jobs
    for job_row in fetch_new_jobs(db_cursor):
        db_conn.commit()
        job_id = job_row['id']
        clp_io_config = msgpack.unpackb(zstd_dctx.decompress(job_row['clp_config']))

        paths_to_compress_buffer = PathsToCompressBuffer(
            scheduler_db_cursor=db_cursor,
            maintain_file_ordering=False,
            empty_directories_allowed=True,
            scheduling_job_id=job_id,
            zstd_cctx=zstd_cctx,
            clp_io_config=clp_io_config,
            database_connection_params=database_connection_params
        )

        with open(pathlib.Path(clp_io_config['input']['list_path']).resolve(), 'r') as f:
            for path_idx, path in enumerate(f, start=1):
                stripped_path = path.strip()
                if '' == stripped_path:
                    # Skip empty paths
                    continue
                path = pathlib.Path(stripped_path)

                try:
                    file, empty_directory = validate_path_and_get_info(CONTAINER_INPUT_LOGS_ROOT_DIR, path)
                except ValueError as ex:
                    logger.error(str(ex))
                    continue

                if file:
                    paths_to_compress_buffer.add_file(file)
                elif empty_directory:
                    paths_to_compress_buffer.add_empty_directory(empty_directory)

                if path.is_dir():
                    for internal_path in path.rglob('*'):
                        try:
                            file, empty_directory = validate_path_and_get_info(
                                CONTAINER_INPUT_LOGS_ROOT_DIR, internal_path)
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
        if len(tasks) == 0:
            logger.warning(f'No tasks were created for job {job_id}')
            update_compression_job_metadata(db_cursor, job_id, {
                'status': CompressionJobStatus.FAILED,
                'status_msg': 'invalid input path',
            })
            db_conn.commit()
            continue

        # Update job metadata
        start_time = datetime.datetime.now()
        update_compression_job_metadata(db_cursor, job_id, {
            'num_tasks': paths_to_compress_buffer.num_tasks,
            'status': CompressionJobStatus.SCHEDULED,
            'start_time': start_time
        })
        db_conn.commit()

        task_instances = []
        for task in tasks:
            task_instances.append(compress.s(**task))
        tasks_group = celery.group(task_instances)

        job = CompressionJob(
            id=job_id,
            start_time=start_time,
            tasks=tasks_group.apply_async()
        )
        db_cursor.execute(f"""
            UPDATE {COMPRESSION_TASKS_TABLE_NAME}
            SET status='{CompressionTaskStatus.SCHEDULED}', start_time='{datetime.datetime.now()}'
            WHERE job_id={job_id}
        """)
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

    logger.debug('Poll running jobs')
    jobs_to_delete = []
    for job_id, job in scheduled_jobs.items():
        job_success = True
        num_tasks_completed = 0
        uncompressed_size = 0
        compressed_size = 0
        error_message = ""

        try:
            returned_results = get_results_or_timeout(job.tasks)
            if returned_results is not None:
                duration = (datetime.datetime.now() - job.start_time).total_seconds()
                # Check for finished jobs
                for task_result in returned_results:
                    if not task_result['status'] == CompressionTaskStatus.SUCCEEDED:
                        job_success = False
                        error_message += f"task {task_result['task_id']}: {task_result['error_message']}\n"
                        update_compression_task_metadata(db_cursor, task_result['task_id'], dict(
                            status=task_result['status'],
                            duration=task_result['duration'],
                        ))
                        db_conn.commit()
                        logger.error(f"Compression task job-{job_id}-task-{task_result['task_id']} failed with error: "
                                     f"{task_result['error_message']}.")
                    else:
                        num_tasks_completed += 1
                        uncompressed_size += task_result['total_uncompressed_size']
                        compressed_size += task_result['total_compressed_size']
                        update_compression_task_metadata(db_cursor, task_result['task_id'], dict(
                            status=task_result['status'],
                            partition_uncompressed_size=task_result['total_uncompressed_size'],
                            partition_compressed_size=task_result['total_compressed_size'],
                            duration=task_result['duration'],
                        ))
                        db_conn.commit()
                        logger.info(f"Compression task job-{job_id}-task-{task_result['task_id']} completed in "
                                    f"{task_result['duration']} second(s).")
            else:
                # If results not ready check next job
                continue
        except Exception as e:
            logger.error(f"Error while getting results for job {job_id}: {e}")
            job_success = False

        if job_success:
            logger.info(f"Job {job_id} succeeded.")
            update_compression_job_metadata(db_cursor, job_id, dict(
                status=CompressionJobStatus.SUCCEEDED,
                duration=duration,
                uncompressed_size=uncompressed_size,
                compressed_size=compressed_size,
                num_tasks_completed=num_tasks_completed
            ))
        else:
            logger.error(f"Job {job_id} failed. See worker logs or status_msg for details.")
            update_compression_job_metadata(db_cursor, job_id, dict(
                status=CompressionJobStatus.FAILED,
                status_msg=error_message,
                num_tasks_completed=num_tasks_completed
            ))
        db_conn.commit()

        # delete job
        jobs_to_delete.append(job_id)

    for job_id in jobs_to_delete:
        del scheduled_jobs[job_id]


def main(argv):
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')
    args = args_parser.parse_args(argv[1:])

    # Setup logging
    log_file = pathlib.Path(os.getenv("CLP_LOGS_DIR")) / "compression_scheduler.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    set_logging_level(logger, os.getenv("CLP_LOGGING_LEVEL"))

    # Load configuration
    config_path = pathlib.Path(args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
    except Exception as ex:
        logger.error(ex)
        # read_yaml_config_file already logs the parsing error inside
        return -1

    logger.info('Starting CLP compression job scheduler')
    sql_adapter = SQL_Adapter(clp_config.database)

    while True:
        try:
            # Start Job Processing Loop
            with closing(sql_adapter.create_connection(True)) as db_conn, \
                    closing(db_conn.cursor(dictionary=True)) as db_cursor:
                search_and_schedule_new_tasks(db_conn, db_cursor, sql_adapter.database_config.
                                              get_clp_connection_params_and_type(True))
                poll_running_jobs(db_conn, db_cursor)
        except:
            logger.exception("Error in scheduling.")
        finally:
            try:
                time.sleep(clp_config.compression_scheduler.jobs_poll_delay)
            except KeyboardInterrupt:
                logger.info('Gracefully shutting down')
                break


if '__main__' == __name__:
    main(sys.argv)
