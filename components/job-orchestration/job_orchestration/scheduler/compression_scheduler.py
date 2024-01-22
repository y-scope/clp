import argparse
import datetime
import logging
import pathlib
import sys
import threading
import time
import typing
from contextlib import closing

import zstandard
from pydantic import ValidationError

from clp_package_utils.general import CONTAINER_INPUT_LOGS_ROOT_DIR
from clp_py_utils.clp_config import (
    CLPConfig,
    Database,
    ResultsCache,
)
from clp_py_utils.compression import validate_path_and_get_info
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.compression_task import compress
from job_orchestration.executor.search_task import search
from job_orchestration.scheduler.constants import \
    QueueName, \
    JobStatus, \
    TaskUpdateType, \
    TaskStatus
from job_orchestration.scheduler.partition import PathsToCompressBuffer
from job_orchestration.scheduler.scheduler_data import \
    CompressionJob, \
    SearchJob, \
    CompressionTask, \
    SearchTask, \
    TaskUpdate, \
    TaskFailureUpdate, \
    CompressionTaskSuccessUpdate

from celery import group

# Setup logging
# Create logger
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.INFO)
console_handler.setFormatter(logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s"))
logger = logging.getLogger(__file__)
logger.addHandler(console_handler)
logger.setLevel(logging.DEBUG)

scheduled_jobs = {}


def fetch_new_jobs(db_cursor):
    db_cursor.execute(f"""
        SELECT id, clp_config
        FROM compression_jobs
        WHERE status='{JobStatus.SCHEDULING}'
    """)
    return db_cursor.fetchall()


def update_compression_task_metadata(db_cursor, task_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f'UPDATE compression_tasks SET {", ".join(field_set_expressions)} WHERE id={task_id};'
    db_cursor.execute(query)


def update_compression_job_metadata(db_cursor, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f'UPDATE compression_jobs SET {", ".join(field_set_expressions)} WHERE id={job_id};'
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
        job = CompressionJob(
            id=job_row['id'],
            clp_config=job_row['clp_config']
        )
        db_conn.commit()
        job_id = job.id
        clp_io_config = job.get_clp_config(zstd_dctx)

        paths_to_compress_buffer = PathsToCompressBuffer(
            scheduler_db_cursor=db_cursor,
            maintain_file_ordering=False,
            empty_directories_allowed=True,
            scheduling_job_id=job_id,
            zstd_cctx=zstd_cctx,
            clp_io_config=clp_io_config,
            database_connection_params=database_connection_params
        )

        with open(pathlib.Path(clp_io_config.input.list_path).resolve(), 'r') as f:
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

        update_compression_job_metadata(db_cursor, job_id, {
            'num_tasks': paths_to_compress_buffer.num_tasks,
            'status': JobStatus.SCHEDULED,
            'start_time': datetime.datetime.now()
        })
        db_conn.commit()

        tasks = paths_to_compress_buffer.get_tasks()
        if len(tasks) == 0:
            logger.warning(f'No tasks were created for job {job_id}')
            continue

        job.num_tasks = len(tasks)

        tasks_group = group(tasks)
        job.tasks = tasks_group.apply_async()
        db_cursor.execute(f"""
            UPDATE compression_tasks
            SET status='{TaskStatus.SCHEDULED}', scheduled_time='{datetime.datetime.now()}'
            WHERE job_id={job_id}
        """)
        db_conn.commit()
        job.start_time = datetime.datetime.now()
        scheduled_jobs[job_id] = job


def try_getting_results(result):
    if not result.ready():
        return None
    return result.get()


def poll_running_jobs(db_conn, db_cursor):
    """
    Poll for running jobs and update their status.
    """
    global scheduled_jobs

    logger.debug('Poll running jobs')

    for job_id, job in scheduled_jobs.items():
        job_success = False
        num_tasks_completed = 0
        uncompressed_size = 0
        compressed_size = 0
        error_message = ""

        task_results = []
        try:
            returned_results = try_getting_results(job.tasks)
            if returned_results is not None:
                job_end_time = float(time.time())
                job.duration = job_end_time - job.start_time
                # Check for finished jobs
                for task_result in returned_results:
                    task_results.append(task_result)
                    if not task_result.status == TaskStatus.SUCCESS:
                        job_success = False
                        error_message += f"task {task_result.task_id}: {task_result.error_message}\n"
                    else:
                        num_tasks_completed += 1
                        uncompressed_size += task_result.total_uncompressed_size
                        compressed_size += task_result.total_compressed_size
                        update_compression_task_metadata(db_cursor, task_result.task_id, dict(
                            status=task_result.status,
                            partition_uncompressed_size=task_result.total_uncompressed_size,
                            partition_compressed_size=task_result.total_compressed_size,
                            duration=task_result.duration,
                        ))
                        db_conn.commit()
            else:
                # If results not ready check next job
                continue
        except Exception as e:
            job_success = False

        logger.info(f"Finished job `{job_id}`.")
        if job_success:
            logger.info(f"Job {job_id} succeeded.")
            update_compression_job_metadata(db_cursor, job_id, dict(
                status=JobStatus.SUCCESS,
                duration=job.duration,
                uncompressed_size=uncompressed_size,
                compressed_size=compressed_size,
                num_tasks_completed=num_tasks_completed
            ))
        else:
            logger.error(f"Job {job_id} failed. See worker logs or status_msg for details.")
            update_compression_job_metadata(db_cursor, task_result.job_id, dict(
                status=JobStatus.FAILED,
                status_msg=error_message,
                num_tasks_completed=num_tasks_completed
            ))
        db_conn.commit()

        # delete job
        del scheduled_jobs[job_id]


def main(argv):
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')
    args = args_parser.parse_args(argv[1:])

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
                time.sleep(clp_config.scheduler.jobs_poll_delay)
            except KeyboardInterrupt:
                logger.info('Gracefully shutting down')
                break


if '__main__' == __name__:
    main(sys.argv)
