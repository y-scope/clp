import argparse
import datetime
import logging
import os
import pathlib
import sys
import threading
import time
import typing
from contextlib import closing

import zstandard
from pydantic import ValidationError

from clp_py_utils.clp_config import (
    CLPConfig,
    ResultsCache,
)
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.search_task import search
from job_orchestration.scheduler.constants import \
    QueueName, \
    JobStatus, \
    TaskUpdateType, \
    TaskStatus
from job_orchestration.scheduler.results_consumer import ReconnectingResultsConsumer
from job_orchestration.scheduler.scheduler_data import \
    SearchJob, \
    SearchTask, \
    TaskUpdate, \
    TaskFailureUpdate, \
    CompressionTaskSuccessUpdate

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


def fetch_new_search_task_metadata(db_cursor) -> list:
    db_cursor.execute(f"""
        SELECT search_jobs.id as job_id,
        search_jobs.status as job_status,
        search_jobs.num_tasks,
        search_jobs.num_tasks_completed,
        search_jobs.search_config,
        search_tasks.id as task_id,
        search_tasks.status as task_status,
        search_tasks.archive_id
        FROM search_jobs INNER JOIN search_tasks
        ON search_jobs.id=search_tasks.job_id
        WHERE search_tasks.status='{TaskStatus.SUBMITTED}'
    """)
    return db_cursor.fetchall()


def update_task_metadata(db_cursor, task_type: str, task_id, kv: typing.Dict[str, typing.Any]):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f'UPDATE {task_type}_tasks SET {", ".join(field_set_expressions)} WHERE id={task_id};'
    db_cursor.execute(query)


def update_search_task_metadata(db_cursor, task_id, kv: typing.Dict[str, typing.Any]):
    update_task_metadata(db_cursor, 'search', task_id, kv)


def update_job_metadata(db_cursor, job_type: str, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f'UPDATE {job_type}_jobs SET {", ".join(field_set_expressions)} WHERE id={job_id};'
    db_cursor.execute(query)


def update_search_job_metadata(db_cursor, job_id, kv):
    update_job_metadata(db_cursor, 'search', job_id, kv)


def increment_job_metadata(db_cursor, job_type: str, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to increment")
        raise ValueError

    field_set_expressions = [f'{k}={k}+{v}' for k, v in kv.items()]
    query = f'UPDATE {job_type}_jobs SET {", ".join(field_set_expressions)} WHERE id={job_id};'
    db_cursor.execute(query)


def increment_search_job_metadata(db_cursor, job_id, kv):
    increment_job_metadata(db_cursor, 'search', job_id, kv)


def schedule_search_task(job: SearchJob, task: SearchTask, results_cache: ResultsCache,
                         dctx: zstandard.ZstdDecompressor):
    args = (job.id, task.id, job.get_search_config_json_str(dctx), task.archive_id,
            results_cache.get_uri())
    return search.apply_async(args, task_id=str(task.id), queue=QueueName.SEARCH, priority=task.priority)


def search_and_schedule_new_tasks(db_conn, db_cursor, results_cache: ResultsCache):
    """
    For all task with SUBMITTED status, push them to task queue to be processed, if finished, update them
    """
    global scheduled_jobs
    global id_to_search_job
    global jobs_lock

    logger.debug('Search and schedule new tasks')

    dctx = zstandard.ZstdDecompressor()

    # Poll for new search tasks
    for task_row in fetch_new_search_task_metadata(db_cursor):
        search_task = SearchTask(
            id=task_row['task_id'],
            status=task_row['task_status'],
            job_id=task_row['job_id'],
            archive_id=task_row['archive_id'],
        )
        job_id = search_task.job_id

        with jobs_lock:
            now = datetime.datetime.utcnow()

            try:
                search_job = id_to_search_job[job_id]
            except KeyError:
                # New job
                search_job = SearchJob(
                    id=task_row['job_id'],
                    status=task_row['job_status'],
                    start_time=now,
                    num_tasks=task_row['num_tasks'],
                    num_tasks_completed=task_row['num_tasks_completed'],
                    search_config=task_row['search_config'],
                )
                update_search_job_metadata(db_cursor, job_id,
                                           dict(start_time=now.strftime('%Y-%m-%d %H:%M:%S')))
                id_to_search_job[search_job.id] = search_job

            celery_task = schedule_search_task(search_job, search_task, results_cache, dctx)

            update_search_task_metadata(db_cursor, search_task.id, dict(
                status=TaskStatus.SCHEDULED,
                scheduled_time=now.strftime('%Y-%m-%d %H:%M:%S')
            ))
            db_conn.commit()

            search_task.instance = celery_task
            search_task.status = TaskStatus.SCHEDULED
            search_job.tasks[search_task.id] = search_task

    # Commit one final time to force MySQL/MariaDB to drop its cache of results
    # for SELECT statements
    # FIXME: Why is this necessary with the latest MySQL/MariaDB?
    db_conn.commit()


def update_completed_jobs(db_cursor, job_type: str):
    # Update completed jobs if there are any
    db_cursor.execute(f"""
        UPDATE {job_type}_jobs 
        SET status='{JobStatus.SUCCEEDED}', duration=TIMESTAMPDIFF(SECOND, start_time, CURRENT_TIMESTAMP())
        WHERE status='{JobStatus.SCHEDULED}' AND num_tasks=num_tasks_completed
    """)


def handle_search_task_update(db_conn, db_cursor, task_update: typing.Union[TaskUpdate, TaskFailureUpdate]):
    global id_to_search_job

    try:
        job = id_to_search_job[task_update.job_id]
        task = job.tasks[task_update.task_id]
    except KeyError:
        logger.warning(f"Discarding update for untracked task: {task_update.json()}")
        return

    now = datetime.datetime.utcnow()
    task_duration = 0
    if task.start_time:
        task_duration = max(int((now - task.start_time).total_seconds()), 1)

    if TaskStatus.IN_PROGRESS == task_update.status:
        update_search_task_metadata(db_cursor, task_update.task_id, dict(
            status=task_update.status,
            start_time=now.strftime('%Y-%m-%d %H:%M:%S')
        ))
    elif TaskStatus.SUCCEEDED == task_update.status:
        if TaskStatus.IN_PROGRESS != task.status:
            logger.warning(f"Discarding task update that's impossible for tracked task: {task_update.json()}")
            return

        logger.info(f"Search task job-{task_update.job_id}-task-{task_update.task_id} completed in {task_duration} "
                    f"second(s).")

        update_search_task_metadata(db_cursor, task_update.task_id, dict(
            status=task_update.status,
            duration=task_duration
        ))
        increment_search_job_metadata(db_cursor, task_update.job_id, dict(
            num_tasks_completed=1
        ))
    elif TaskStatus.FAILED == task_update.status:
        logger.error(f"Search task job-{task_update.job_id}-task-{task_update.task_id} failed with error: "
                     f"{task_update.error_message}.")
        update_search_task_metadata(db_cursor, task_update.task_id, dict(
            status=task_update.status,
            duration=task_duration
        ))
        update_search_job_metadata(db_cursor, job.id, dict(
            status=task_update.status,
            status_msg=task_update.error_message
        ))
    else:
        raise NotImplementedError

    db_conn.commit()

    task.status = task_update.status
    if TaskStatus.IN_PROGRESS == task_update.status:
        task.start_time = now
    elif TaskStatus.SUCCEEDED == task_update.status:
        job.num_tasks_completed += 1
    elif TaskStatus.FAILED == task_update.status:
        job.status = task_update.status
    else:
        raise NotImplementedError


def task_results_consumer(sql_adapter: SQL_Adapter, celery_broker_url):
    def callback(ch, method, properties, body):
        global scheduled_jobs
        global id_to_search_job
        global jobs_lock
        global logger

        try:
            # Validate message body
            task_update = TaskUpdate.parse_raw(body)
            if TaskStatus.FAILED == task_update.status:
                task_update = TaskFailureUpdate.parse_raw(body)
            elif TaskUpdateType.COMPRESSION == task_update.type and \
                    TaskStatus.SUCCEEDED == task_update.status:
                task_update = CompressionTaskSuccessUpdate.parse_raw(body)
        except ValidationError as err:
            logger.error(err)
            exit(-1)

        with closing(sql_adapter.create_connection(True)) as db_conn, \
                closing(db_conn.cursor(dictionary=True)) as db_cursor, jobs_lock:
            logger.debug(f"Task update received: "
                         f"job_id={task_update.job_id} "
                         f"task_id={task_update.task_id} "
                         f"status={task_update.status}")

            try:
                handle_search_task_update(db_conn, db_cursor, task_update)

                # Only send out the ACK if data successfully persisted to the database
                ch.basic_ack(method.delivery_tag)
            except Exception as error:
                # Transaction failure, rollback, don't send ACK and simply reprocess the msg again
                logger.error(f'Database update failed: {error}.')
                db_conn.rollback()

    consumer = ReconnectingResultsConsumer(celery_broker_url, callback)
    consumer_thread = threading.Thread(target=consumer.run)
    consumer_thread.start()
    return consumer


def main(argv):
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')
    args = args_parser.parse_args(argv[1:])

    celery_broker_url = os.getenv('BROKER_URL')

    # Load configuration
    config_path = pathlib.Path(args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
    except Exception as ex:
        logger.error(ex)
        # read_yaml_config_file already logs the parsing error inside
        pass
    else:
        # Collect new jobs from the database
        logger.info('Starting CLP job scheduler')
        sql_adapter = SQL_Adapter(clp_config.database)

        results_consumer = task_results_consumer(sql_adapter, celery_broker_url)

        while True:
            try:
                # Start Job Processing Loop
                with closing(sql_adapter.create_connection(True)) as db_conn, \
                        closing(db_conn.cursor(dictionary=True)) as db_cursor:
                    search_and_schedule_new_tasks(db_conn, db_cursor, clp_config.results_cache)
                    update_completed_jobs(db_cursor, 'search')
                    db_conn.commit()
            except:
                logger.exception("Error in scheduling.")
            finally:
                try:
                    time.sleep(clp_config.scheduler.jobs_poll_delay)
                except KeyboardInterrupt:
                    logger.info('Gracefully shutting down')
                    break

        if results_consumer:
            try:
                results_consumer._consumer.stop()
            except RuntimeError as err:
                if 'IOLoop is not reentrant and is already running' != str(err):
                    logger.error(err)
                    raise RuntimeError
                else:
                    # Normal graceful shutdown path
                    pass
            logger.info('Scheduler stopped')


if '__main__' == __name__:
    main(sys.argv)
