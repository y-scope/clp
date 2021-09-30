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

from job_orchestration.executor.compression.task import compress
from job_orchestration.scheduler.results_consumer import ReconnectingResultsConsumer
from job_orchestration.scheduler.scheduler_data \
    import Job, Task, TaskUpdate, TaskCompletionUpdate, TaskFailureUpdate
from clp_py_utils.clp_config import CLPConfig, Database
from clp_py_utils.sql_adapter import SQL_Adapter

# Setup logging
# Create logger
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.INFO)
console_handler.setFormatter(
    logging.Formatter('%(asctime)s [%(levelname)s] [%(name)s] %(message)s'))
log = logging.getLogger('scheduler')
log.addHandler(console_handler)
log.setLevel(logging.DEBUG)

scheduled_jobs = {}
jobs_lock = threading.Lock()

from clp_py_utils.core import read_yaml_config_file


def fetch_new_task_metadata(db_cursor) -> list:
    db_cursor.execute(
        """
            SELECT compression_jobs.job_id,
                compression_jobs.job_status,
                compression_jobs.num_tasks,
                compression_jobs.num_tasks_completed,
                compression_jobs.clp_config,
                compression_tasks.task_id,
                compression_tasks.task_status,
                compression_tasks.clp_paths_to_compress
            FROM compression_jobs INNER JOIN compression_tasks
            ON compression_jobs.job_id=compression_tasks.job_id
            WHERE compression_tasks.task_status='SUBMITTED';
        """
    )
    return db_cursor.fetchall()


def update_task_metadata(db_cursor, task_id, kv: typing.Dict[str, typing.Any]):
    if not len(kv):
        log.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{str(k)}="{str(v)}"' for k, v in kv.items()]
    query = f'UPDATE compression_tasks SET {", ".join(field_set_expressions)} ' \
            f'WHERE task_id={str(task_id)};'
    db_cursor.execute(query)


def update_job_metadata(db_cursor, job_id, kv):
    if not len(kv):
        log.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{str(k)}="{str(v)}"' for k, v in kv.items()]
    query = f'UPDATE compression_jobs SET {", ".join(field_set_expressions)} ' \
            f'WHERE job_id={str(job_id)};'
    db_cursor.execute(query)


def increment_job_metadata(db_cursor, job_id, kv):
    if not len(kv):
        log.error("Must specify at least one field to increment")
        raise ValueError

    field_set_expressions = [f'{str(k)}={str(k)}+{str(v)}' for k, v in kv.items()]
    query = f'UPDATE compression_jobs SET {", ".join(field_set_expressions)} ' \
            f'WHERE job_id={str(job_id)};'
    db_cursor.execute(query)


def schedule_task(job: Job, task: Task, database_config: Database, dctx: zstandard.ZstdDecompressor = None):
    return compress.apply_async(
        (job.job_id, task.task_id,
         job.get_clp_config_json(dctx),
         task.get_clp_paths_to_compress_json(dctx),
         database_config.get_clp_connection_params_and_type()),
        task_id=str(task.task_id), queue='compression', priority=task.priority)


def search_and_schedule_new_tasks(db_conn, db_cursor, database_config: Database):
    """
    For all task with SUBMITTED status, push them to task queue to be processed, if finished, update them
    """
    global scheduled_jobs
    global jobs_lock

    log.debug('Search and schedule new tasks')

    dctx = zstandard.ZstdDecompressor()

    # Fetch new task
    for task_row in fetch_new_task_metadata(db_cursor):
        log.debug(f"Found task with job_id={task_row['job_id']} task_id={task_row['task_id']}")

        # Only Add database credentials to ephemeral task specification passed to workers
        task = Task.parse_obj(task_row)
        job_id: int = task_row['job_id']

        with jobs_lock:
            now = datetime.datetime.utcnow()

            try:
                job = scheduled_jobs[job_id]
            except KeyError:
                # Identified a new job identified
                job = Job(job_start_time=now, **task_row)
                update_job_metadata(db_cursor, job_id, dict(
                    job_start_time=now.strftime('%Y-%m-%d %H:%M:%S')
                ))

            # Schedule task, update ephemeral metadata in scheduler and commit to database
            celery_task_instance = schedule_task(job, task, database_config, dctx)

            update_task_metadata(db_cursor, task.task_id, dict(
                task_status='SCHEDULED',
                task_scheduled_time=now.strftime('%Y-%m-%d %H:%M:%S')
            ))
            db_conn.commit()

            # After database commit is successful, update internal metadata
            task.instance = celery_task_instance
            task.task_status = 'SCHEDULED'
            job.tasks[task.task_id] = task

            # Optimization: if job has finished scheduling while we are scheduling task,
            # Then we'll update the job's status and num_tasks count
            try:
                if 'SCHEDULED' == task_row['job_status']:
                    job.num_tasks = task_row['num_tasks']
                    job.job_status = task_row['job_status']
            except KeyError:
                pass

            scheduled_jobs[job_id] = job
            db_conn.commit()


def update_completed_jobs(db_conn, db_cursor):
    # Update completed jobs if there are any
    db_cursor.execute(
        """
            UPDATE compression_jobs 
            SET job_status="COMPLETED", job_duration=TIMESTAMPDIFF(SECOND,job_start_time, CURRENT_TIMESTAMP())
            WHERE job_status="SCHEDULED" AND num_tasks=num_tasks_completed;
        """
    )
    db_conn.commit()


def task_results_consumer(sql_adapter: SQL_Adapter, celery_broker_url):
    global scheduled_jobs
    global jobs_lock

    def callback(ch, method, properties, body):
        global scheduled_jobs
        global jobs_lock
        global log

        try:
            # Validate message body
            task_update = TaskUpdate.parse_raw(body)
            if 'COMPLETED' == task_update.status:
                task_update = TaskCompletionUpdate.parse_raw(body)
            elif 'FAILED' == task_update.status:
                task_update = TaskFailureUpdate.parse_raw(body)
        except ValidationError as err:
            log.error(err)
            exit(-1)

        with closing(sql_adapter.create_connection()) as db_conn, \
                closing(db_conn.cursor(dictionary=True)) as db_cursor, jobs_lock:
            log.debug(f'Task update received: '
                      f'job_id={task_update.job_id} '
                      f'task_id={task_update.task_id} '
                      f'status={task_update.status}')

            # Retrieve scheduler state
            try:
                job = scheduled_jobs[task_update.job_id]
                task = job.tasks[task_update.task_id]
            except KeyError:
                # Scheduler detected response from task which it does not keep track of
                # It could be that previous scheduler crashed.
                # The only thing we can do is to log, and discard the message
                # to prevent infinite loop
                log.warning(f'Discarding untracked task update: {task_update.json()}')
                ch.basic_ack(method.delivery_tag)
                return

            # Process task update and update database
            try:
                # Scheduler is aware of the task
                now = datetime.datetime.utcnow()

                if 'COMPRESSING' == task_update.status:
                    # Update sent by worker when task began in the database
                    update_task_metadata(db_cursor, task_update.task_id, dict(
                        task_status=task_update.status,
                        task_start_time=now.strftime('%Y-%m-%d %H:%M:%S')
                    ))
                elif 'COMPLETED' == task_update.status:
                    # Update sent by worker when task finishes
                    if 'COMPRESSING' != task.task_status:
                        log.warning(f'Discarding untracked task update: {task_update.json()}')
                        ch.basic_ack(method.delivery_tag)
                        raise NotImplementedError

                    task_duration = max(int((now - task.task_start_time).total_seconds()), 1)

                    log.info(f'Task job-{task_update.job_id}-task-{task_update.task_id} '
                             f'completed in {str(task_duration)} second.')

                    update_task_metadata(db_cursor, task_update.task_id, dict(
                        task_status=task_update.status,
                        partition_uncompressed_size=task_update.total_uncompressed_size,
                        partition_compressed_size=task_update.total_compressed_size,
                        task_duration=int(task_duration)
                    ))
                    increment_job_metadata(db_cursor, task_update.job_id, dict(
                        job_uncompressed_size=task_update.total_uncompressed_size,
                        job_compressed_size=task_update.total_compressed_size,
                        num_tasks_completed=1
                    ))
                elif 'FAILED' == task_update.status:
                    log.warning(f'Marking job_id={task_update.job_id} as failed.')
                    log.warning(str(task_update.error_message))
                    update_task_metadata(db_cursor, task_update.task_id, dict(
                        task_status=task_update.status,
                        task_duration=int((now - task.task_start_time).total_seconds())
                    ))
                    update_job_metadata(db_cursor, job.job_id, dict(
                        job_status=task_update.status,
                        job_status_msg=task_update.error_message
                    ))
                else:
                    raise NotImplementedError

                db_conn.commit()

                # Only update scheduler metadata only after transaction finishes
                # If update fails, rollback and avoid updating scheduler state
                job.tasks[task_update.task_id].task_status = task_update.status
                if 'COMPRESSING' == task_update.status:
                    job.tasks[task_update.task_id].task_start_time = now
                elif 'COMPLETED' == task_update.status:
                    job.num_tasks_completed += 1
                elif 'FAILED' == task_update.status:
                    # TODO: how to handle failure scheduler state update besides simply recording acknowledgement?
                    job.job_status = task_update.status
                    pass
                else:
                    raise NotImplementedError

                # Only send out the ACK if data successfully persisted to the database
                ch.basic_ack(method.delivery_tag)

            except Exception as error:
                # Transaction failure, rollback, don't send ACK and simply reprocess the msg again
                log.error(f'Database update failed: {error}.')
                db_conn.rollback()

    consumer = ReconnectingResultsConsumer(celery_broker_url, callback)
    consumer_thread = threading.Thread(target=consumer.run)
    consumer_thread.start()
    return consumer


def main(argv):
    global scheduled_jobs
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')
    args = args_parser.parse_args(argv[1:])

    celery_broker_url = os.getenv('BROKER_URL')

    # Load configuration
    config_path = pathlib.Path(args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        log.error(err)
    except Exception as ex:
        log.error(ex)
        # read_yaml_config_file already logs the parsing error inside
        pass
    else:
        # Collect new jobs from the database
        log.info("Starting CLP job scheduler")
        sql_adapter = SQL_Adapter(clp_config.database)

        results_consumer = task_results_consumer(sql_adapter, celery_broker_url)

        while True:
            try:
                # Start Job Processing Loop
                with closing(sql_adapter.create_connection()) as db_conn, \
                        closing(db_conn.cursor(dictionary=True)) as db_cursor:
                    search_and_schedule_new_tasks(db_conn, db_cursor, sql_adapter.database_config)
                    update_completed_jobs(db_conn, db_cursor)
            except Exception as ex:
                log.error("Error in scheduling: ")
                log.error(ex)
            finally:
                try:
                    time.sleep(clp_config.scheduler.jobs_poll_delay)
                except KeyboardInterrupt:
                    log.info("Gracefully shutting down")
                    break

        if results_consumer:
            try:
                results_consumer._consumer.stop()
            except RuntimeError as err:
                if 'IOLoop is not reentrant and is already running' != str(err):
                    log.error(err)
                    raise RuntimeError
                else:
                    # Normal graceful shutdown path
                    pass
            log.info('Scheduler stopped')


if '__main__' == __name__:
    main(sys.argv)
