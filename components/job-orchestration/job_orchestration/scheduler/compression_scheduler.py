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
id_to_search_job: typing.Dict[int, SearchJob] = {}
jobs_lock = threading.Lock()


def fetch_new_jobs(db_cursor):
    db_cursor.execute(f"""
        SELECT id, clp_config
        FROM compression_jobs
        WHERE status='{JobStatus.SUBMITTED}'
    """)
    return db_cursor.fetchall()


def update_task_metadata(db_cursor, task_type: str, task_id, kv: typing.Dict[str, typing.Any]):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f'UPDATE {task_type}_tasks SET {", ".join(field_set_expressions)} WHERE id={task_id};'
    db_cursor.execute(query)


def update_compression_task_metadata(db_cursor, task_id, kv: typing.Dict[str, typing.Any]):
    update_task_metadata(db_cursor, 'compression', task_id, kv)


def update_job_metadata(db_cursor, job_type: str, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f'UPDATE {job_type}_jobs SET {", ".join(field_set_expressions)} WHERE id={job_id};'
    db_cursor.execute(query)


def update_compression_job_metadata(db_cursor, job_id, kv):
    update_job_metadata(db_cursor, 'compression', job_id, kv)


def increment_job_metadata(db_cursor, job_type: str, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to increment")
        raise ValueError

    field_set_expressions = [f'{k}={k}+{v}' for k, v in kv.items()]
    query = f'UPDATE {job_type}_jobs SET {", ".join(field_set_expressions)} WHERE id={job_id};'
    db_cursor.execute(query)


def increment_compression_job_metadata(db_cursor, job_id, kv):
    increment_job_metadata(db_cursor, 'compression', job_id, kv)


def search_and_schedule_new_tasks(db_conn, db_cursor, database_config: Database):
    """
    For all jobs with SUBMITTED status, split the job into tasks and schedule them.
    """
    global scheduled_jobs
    global id_to_search_job
    global jobs_lock

    logger.debug('Search and schedule new tasks')

    zstd_dctx = zstandard.ZstdDecompressor()
    zstd_cctx = zstandard.ZstdCompressor(level=3)

    # Poll for new compression tasks
    for job_row in fetch_new_jobs(db_cursor):
        job = CompressionJob(
            id=job_row['id'],
            status=job_row['status'],
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
            database_connection_params=database_config.get_clp_connection_params_and_type(True)
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

        # Ensure all of the scheduled task and the total number of tasks
        # in the job row has been updated and committed
        db_cursor.execute(f"""
            UPDATE compression_jobs 
            SET num_tasks={paths_to_compress_buffer.num_tasks}, status='{JobStatus.SCHEDULED}' 
            WHERE id={job_id}
        """)
        db_conn.commit()

        tasks = paths_to_compress_buffer.get_tasks()
        if len(tasks) == 0:
            logger.warning(f'No tasks were created for job {job_id}')
            continue

        job.num_tasks = len(tasks)

        tasks_group = group(tasks)
        job.task_results = tasks_group.apply_async()
        job.start_time = datetime.datetime.now()
        scheduled_jobs[job_id] = job


def try_getting_results(result):
    if not result.ready():
        return None
    return result.get()


def poll_running_jobs(db_conn, db_cursor, database_config: Database):
    """
    Poll for running jobs and update their status.
    """
    global scheduled_jobs

    logger.debug('Poll running jobs')

    for job_id, job in scheduled_jobs.items():
        all_tasks_successful = True
        job_success = False
        # V0.5 TODO: ideally we should update database for each successful subtask but
        # I don't have time to do it properly. instead, I do the aggregation here
        job_result: Dict[str, Any] = {
            "total_uncompressed_size": 0,
            "total_compressed_size": 0,
            "num_messages": 0,
            "num_files": 0,
            "begin_ts": sys.maxsize,
            "end_ts": 0
        }

        task_results_list = []
        try:
            returned_results = try_getting_results(job.task_results)
            if returned_results is not None:
                job_completion_ts = float(time.time())
                # Check for finished jobs
                for task_result in returned_results:
                    logger.info(task_result)
                    task_results_list.append(task_result)
                    if not task_result["status"]:
                        all_worker_jobs_successful = False
                        logger.error(f"Worker of {job_id_str} failed. See the worker logs for details.")
                    else:
                        job_success = True
                        update_job_results(job_result, task_result["archive_stat"])
                        db_manager.update_job_progression(job_id_str)
            else:
                # If results not ready check next job
                continue
        except Exception as e:
            all_worker_jobs_successful = False
            logger.error(f"job `{job_id_str}` failed: {e}")

        logger.info(f"Finished job `{job_id_str}`.")

        celery_worker_time = job_completion_ts - active_jobs[job_id_str].celery_submission_ts if job_success else 0
        end_to_end_time = job_completion_ts - active_jobs[job_id_str].submission_ts if job_success else 0
        job_metrics = {
            "submission_ts": active_jobs[job_id_str].submission_ts,
            "scheduler_time": active_jobs[job_id_str].schedule_start_ts - active_jobs[job_id_str].submission_ts,
            "job_setup_time": active_jobs[job_id_str].celery_submission_ts - active_jobs[job_id_str].schedule_start_ts,
            "celery_worker_time": celery_worker_time,
            "end_to_end_time": end_to_end_time
        }
        if not db_manager.update_job_stats(job_id_str, job_result):
            logger.warning(f"stats of job {job_id_str} not updated, job doesn't exist")
        if not db_manager.update_job_metrics(job_id_str, job_metrics):
            logger.warning(f"metrics of job {job_id_str} not updated, job doesn't exist")
        db_manager.insert_tasks_metrics(task_results_list)

        # delete job
        del active_jobs[job_id_str]

        job_completion_time_date = datetime.fromtimestamp(job_completion_ts)

        # FIXME: set job status here and delete
        if not all_worker_jobs_successful:
            set_job_finish_status(db_manager, job_id_str, JobStatus.FAILED, job_completion_ts)
        else:
            set_job_finish_status(db_manager, job_id_str, JobStatus.SUCCESS, job_completion_ts)
        # FIXME: what was this trying to do?
        # elif job_completed_with_errors:
        #    set_job_finish_status(db_manager, job_id_str, JobStatus.SUCCESS_WITH_ERRORS, job_completion_ts)


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
                search_and_schedule_new_tasks(db_conn, db_cursor, sql_adapter.database_config,
                                              clp_config.results_cache)

                poll_running_jobs()
                update_completed_jobs(db_cursor, 'compression')
                db_conn.commit()
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
