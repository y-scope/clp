"""
A scheduler for scheduling query jobs in the CLP package.

NOTE: This scheduler currently only has partial handling for failures of the database. Specifically,
in the event that the database is unreachable, the scheduler will continue running as if reads from
the database return no records and writes to the database always fail. Failed writes are currently
silently ignored, in which case the state of the database won't match the scheduler's internal
state. If the database comes back up, the scheduler will eventually reconnect to it and reads/writes
will function as normal again. However, the mismatched state may lead to unexpected behaviour like
jobs seemingly being stuck in the "RUNNING" state or jobs being repeated, which in turn will create
duplicated search results in the results cache, possibly long after the results had already been
cleared from the cache. Unfortunately, these effects will require manual intervention to clean-up.
TODO Address this limitation.
"""

from __future__ import annotations

import argparse
import asyncio
import contextlib
import datetime
import logging
import os
import pathlib
import sys
from pathlib import Path
from typing import Dict, List, Optional

import celery
import msgpack
import pymongo
from clp_py_utils.clp_config import (
    CLP_METADATA_TABLE_PREFIX,
    CLPConfig,
    QUERY_JOBS_TABLE_NAME,
    QUERY_TASKS_TABLE_NAME,
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.decorators import exception_default_value
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.query.fs_search_task import search
from job_orchestration.scheduler.constants import QueryJobStatus, QueryTaskStatus
from job_orchestration.scheduler.job_config import SearchConfig
from job_orchestration.scheduler.query.reducer_handler import (
    handle_reducer_connection,
    ReducerHandlerMessage,
    ReducerHandlerMessageQueues,
    ReducerHandlerMessageType,
)
from job_orchestration.scheduler.scheduler_data import InternalJobState, QueryTaskResult, SearchJob
from pydantic import ValidationError

# Setup logging
logger = get_logger("search-job-handler")

# Dictionary of active jobs indexed by job id
active_jobs: Dict[str, SearchJob] = {}

reducer_connection_queue: Optional[asyncio.Queue] = None


def cancel_job_except_reducer(job: SearchJob):
    """
    Cancels the job apart from releasing the reducer since that requires an async call.
    NOTE: By keeping this method synchronous, the caller can cancel most of the job atomically,
    making it easier to avoid using locks in concurrent tasks.
    :param job:
    """

    if InternalJobState.RUNNING == job.state:
        job.current_sub_job_async_task_result.revoke(terminate=True)
        try:
            job.current_sub_job_async_task_result.get()
        except Exception:
            pass
    elif InternalJobState.WAITING_FOR_REDUCER == job.state:
        job.reducer_acquisition_task.cancel()


async def release_reducer_for_job(job: SearchJob):
    """
    Releases the reducer assigned to the given job
    :param job:
    """

    if job.reducer_handler_msg_queues is not None:
        # Signal the reducer to cancel the job
        msg = ReducerHandlerMessage(ReducerHandlerMessageType.FAILURE)
        await job.reducer_handler_msg_queues.put_to_handler(msg)


@exception_default_value(default=[])
def fetch_new_search_jobs(db_conn) -> list:
    """
    Fetches search jobs with status=PENDING from the database.
    :param db_conn:
    :return: The pending search jobs on success. An empty list if an exception occurs while
    interacting with the database.
    """
    with contextlib.closing(db_conn.cursor(dictionary=True)) as db_cursor:
        db_cursor.execute(
            f"""
            SELECT {QUERY_JOBS_TABLE_NAME}.id as job_id,
            {QUERY_JOBS_TABLE_NAME}.job_config
            FROM {QUERY_JOBS_TABLE_NAME}
            WHERE {QUERY_JOBS_TABLE_NAME}.status={QueryJobStatus.PENDING}
            """
        )
        return db_cursor.fetchall()


@exception_default_value(default=[])
def fetch_cancelling_search_jobs(db_conn) -> list:
    """
    Fetches search jobs with status=CANCELLING from the database.
    :param db_conn:
    :return: The cancelling search jobs on success. An empty list if an exception occurs while
    interacting with the database.
    """
    with contextlib.closing(db_conn.cursor(dictionary=True)) as db_cursor:
        db_cursor.execute(
            f"""
            SELECT {QUERY_JOBS_TABLE_NAME}.id as job_id
            FROM {QUERY_JOBS_TABLE_NAME}
            WHERE {QUERY_JOBS_TABLE_NAME}.status={QueryJobStatus.CANCELLING}
            """
        )
        return db_cursor.fetchall()


@exception_default_value(default=False)
def set_job_or_task_status(
    db_conn,
    table_name: str,
    job_id: str,
    status: QueryJobStatus | QueryTaskStatus,
    prev_status: Optional[QueryJobStatus | QueryTaskStatus] = None,
    **kwargs,
) -> bool:
    """
    Sets the status of the job or the tasks identified by `job_id` to `status`. If `prev_status` is
    specified, the update is conditional on the job/task's current status matching `prev_status`. If
    `kwargs` are specified, the fields identified by the args are also updated.
    :param db_conn:
    :param table_name:
    :param job_id:
    :param status:
    :param prev_status:
    :param kwargs:
    :return: True on success, False if the update fails or an exception occurs while interacting
    with the database.
    """
    field_set_expressions = [f"status={status}"]
    if QUERY_JOBS_TABLE_NAME == table_name:
        id_col_name = "id"
        field_set_expressions.extend([f'{k}="{v}"' for k, v in kwargs.items()])
    elif QUERY_TASKS_TABLE_NAME == table_name:
        id_col_name = "job_id"
        field_set_expressions.extend([f"{k}={v}" for k, v in kwargs.items()])
    else:
        raise ValueError(f"Unsupported table name {table_name}")
    update = (
        f'UPDATE {table_name} SET {", ".join(field_set_expressions)} WHERE {id_col_name}={job_id}'
    )

    if prev_status is not None:
        update += f" AND status={prev_status}"

    with contextlib.closing(db_conn.cursor()) as cursor:
        cursor.execute(update)
        db_conn.commit()
        rval = cursor.rowcount != 0
    return rval


async def handle_cancelling_search_jobs(db_conn_pool) -> None:
    global active_jobs

    with contextlib.closing(db_conn_pool.connect()) as db_conn:
        cancelling_jobs = fetch_cancelling_search_jobs(db_conn)

        for cancelling_job in cancelling_jobs:
            job_id = str(cancelling_job["job_id"])
            if job_id in active_jobs:
                job = active_jobs.pop(job_id)
                cancel_job_except_reducer(job)
                # Perform any async tasks last so that it's easier to reason about synchronization
                # issues between concurrent tasks
                await release_reducer_for_job(job)
            else:
                continue

            set_job_or_task_status(
                db_conn,
                QUERY_TASKS_TABLE_NAME,
                job_id,
                QueryTaskStatus.CANCELLED,
                QueryTaskStatus.PENDING,
                duration=0,
            )

            set_job_or_task_status(
                db_conn,
                QUERY_TASKS_TABLE_NAME,
                job_id,
                QueryTaskStatus.CANCELLED,
                QueryTaskStatus.RUNNING,
                duration="TIMESTAMPDIFF(MICROSECOND, start_time, NOW())/1000000.0",
            )

            if set_job_or_task_status(
                db_conn,
                QUERY_JOBS_TABLE_NAME,
                job_id,
                QueryJobStatus.CANCELLED,
                QueryJobStatus.CANCELLING,
                duration=(datetime.datetime.now() - job.start_time).total_seconds(),
            ):
                logger.info(f"Cancelled job {job_id}.")
            else:
                logger.error(f"Failed to cancel job {job_id}.")


def insert_search_tasks_into_db(db_conn, job_id, archive_ids: List[str]) -> List[int]:
    task_ids = []
    with contextlib.closing(db_conn.cursor()) as cursor:
        for archive_id in archive_ids:
            cursor.execute(
                f"""
                INSERT INTO {QUERY_TASKS_TABLE_NAME}
                (job_id, archive_id)
                VALUES({job_id}, '{archive_id}')
                """
            )
            task_ids.append(cursor.lastrowid)
    db_conn.commit()
    return task_ids


@exception_default_value(default=[])
def get_archives_for_search(
    db_conn,
    search_config: SearchConfig,
):
    query = f"""SELECT id as archive_id, end_timestamp 
            FROM {CLP_METADATA_TABLE_PREFIX}archives
            """
    filter_clauses = []
    if search_config.end_timestamp is not None:
        filter_clauses.append(f"begin_timestamp <= {search_config.end_timestamp}")
    if search_config.begin_timestamp is not None:
        filter_clauses.append(f"end_timestamp >= {search_config.begin_timestamp}")
    if search_config.tags is not None:
        filter_clauses.append(
            f"id IN (SELECT archive_id FROM {CLP_METADATA_TABLE_PREFIX}archive_tags WHERE "
            f"tag_id IN (SELECT tag_id FROM {CLP_METADATA_TABLE_PREFIX}tags WHERE tag_name IN "
            f"(%s)))" % ", ".join(["%s" for _ in search_config.tags])
        )
    if len(filter_clauses) > 0:
        query += " WHERE " + " AND ".join(filter_clauses)
    query += " ORDER BY end_timestamp DESC"

    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
        if search_config.tags is not None:
            cursor.execute(query, tuple(search_config.tags))
        else:
            cursor.execute(query)
        archives_for_search = list(cursor.fetchall())
    return archives_for_search


def get_task_group_for_job(
    archive_ids: List[str],
    task_ids: List[int],
    job_id: str,
    search_config: SearchConfig,
    clp_metadata_db_conn_params: Dict[str, any],
    results_cache_uri: str,
):
    search_config_obj = search_config.dict()
    return celery.group(
        search.s(
            job_id=job_id,
            archive_id=archive_ids[i],
            task_id=task_ids[i],
            search_config_obj=search_config_obj,
            clp_metadata_db_conn_params=clp_metadata_db_conn_params,
            results_cache_uri=results_cache_uri,
        )
        for i in range(len(archive_ids))
    )


def dispatch_search_job(
    db_conn,
    job: SearchJob,
    archives_for_search: List[Dict[str, any]],
    clp_metadata_db_conn_params: Dict[str, any],
    results_cache_uri: str,
) -> None:
    global active_jobs
    archive_ids = [archive["archive_id"] for archive in archives_for_search]
    task_ids = insert_search_tasks_into_db(db_conn, job.id, archive_ids)

    task_group = get_task_group_for_job(
        archive_ids,
        task_ids,
        job.id,
        job.search_config,
        clp_metadata_db_conn_params,
        results_cache_uri,
    )
    job.current_sub_job_async_task_result = task_group.apply_async()
    job.state = InternalJobState.RUNNING


async def acquire_reducer_for_job(job: SearchJob):
    reducer_host: Optional[str] = None
    reducer_port: Optional[int] = None
    reducer_handler_msg_queues: Optional[ReducerHandlerMessageQueues] = None
    while True:
        reducer_host, reducer_port, reducer_handler_msg_queues = (
            await reducer_connection_queue.get()
        )
        """
        Below, the task can either be cancelled before sending the job config to the reducer or
        before the reducer acknowledges the job. If the task is cancelled before we send the job
        config to the reducer, then we have two options:

        1. Put the reducer's connection info back in the queue for another job to pick up.
        2. Tell the reducer to restart its job handling loop.

        If the task is cancelled after we've sent the job config to the reducer, then we have to
        use option (2), so we use option (2) in both cases.
        """
        try:
            msg = ReducerHandlerMessage(
                ReducerHandlerMessageType.AGGREGATION_CONFIG, job.search_config.aggregation_config
            )
            await reducer_handler_msg_queues.put_to_handler(msg)

            msg = await reducer_handler_msg_queues.get_from_handler()
            if msg.msg_type == ReducerHandlerMessageType.SUCCESS:
                break
            elif msg.msg_type != ReducerHandlerMessageType.FAILURE:
                error_msg = f"Unexpected msg_type: {msg.msg_type.name}"
                raise NotImplementedError(error_msg)
        except asyncio.CancelledError:
            msg = ReducerHandlerMessage(ReducerHandlerMessageType.FAILURE)
            await reducer_handler_msg_queues.put_to_handler(msg)
            raise

    job.reducer_handler_msg_queues = reducer_handler_msg_queues
    job.search_config.aggregation_config.reducer_host = reducer_host
    job.search_config.aggregation_config.reducer_port = reducer_port
    job.state = InternalJobState.WAITING_FOR_DISPATCH
    job.reducer_acquisition_task = None

    logger.info(f"Got reducer for job {job.id} at {reducer_host}:{reducer_port}")


def handle_pending_search_jobs(
    db_conn_pool,
    clp_metadata_db_conn_params: Dict[str, any],
    results_cache_uri: str,
    num_archives_to_search_per_sub_job: int,
) -> List[asyncio.Task]:
    global active_jobs

    reducer_acquisition_tasks = []

    pending_jobs = [
        job for job in active_jobs.values() if InternalJobState.WAITING_FOR_DISPATCH == job.state
    ]

    with contextlib.closing(db_conn_pool.connect()) as db_conn:
        for job in fetch_new_search_jobs(db_conn):
            job_id = str(job["job_id"])

            # Avoid double-dispatch when a job is WAITING_FOR_REDUCER
            if job_id in active_jobs:
                continue

            search_config = SearchConfig.parse_obj(msgpack.unpackb(job["job_config"]))
            archives_for_search = get_archives_for_search(db_conn, search_config)
            if len(archives_for_search) == 0:
                if set_job_or_task_status(
                    db_conn,
                    QUERY_JOBS_TABLE_NAME,
                    job_id,
                    QueryJobStatus.SUCCEEDED,
                    QueryJobStatus.PENDING,
                    start_time=datetime.datetime.now(),
                    num_tasks=0,
                    duration=0,
                ):
                    logger.info(f"No matching archives, skipping job {job['job_id']}.")
                continue

            new_search_job = SearchJob(
                id=job_id,
                search_config=search_config,
                state=InternalJobState.WAITING_FOR_DISPATCH,
                num_archives_to_search=len(archives_for_search),
                num_archives_searched=0,
                remaining_archives_for_search=archives_for_search,
            )

            if search_config.aggregation_config is not None:
                new_search_job.search_config.aggregation_config.job_id = job["job_id"]
                new_search_job.state = InternalJobState.WAITING_FOR_REDUCER
                new_search_job.reducer_acquisition_task = asyncio.create_task(
                    acquire_reducer_for_job(new_search_job)
                )
                reducer_acquisition_tasks.append(new_search_job.reducer_acquisition_task)
            else:
                pending_jobs.append(new_search_job)
            active_jobs[job_id] = new_search_job

        for job in pending_jobs:
            job_id = job.id

            if (
                job.search_config.network_address is None
                and len(job.remaining_archives_for_search) > num_archives_to_search_per_sub_job
            ):
                archives_for_search = job.remaining_archives_for_search[
                    :num_archives_to_search_per_sub_job
                ]
                job.remaining_archives_for_search = job.remaining_archives_for_search[
                    num_archives_to_search_per_sub_job:
                ]
            else:
                archives_for_search = job.remaining_archives_for_search
                job.remaining_archives_for_search = []

            dispatch_search_job(
                db_conn, job, archives_for_search, clp_metadata_db_conn_params, results_cache_uri
            )
            logger.info(
                f"Dispatched job {job_id} with {len(archives_for_search)} archives to search."
            )
            start_time = datetime.datetime.now()
            job.start_time = start_time
            set_job_or_task_status(
                db_conn,
                QUERY_JOBS_TABLE_NAME,
                job_id,
                QueryJobStatus.RUNNING,
                QueryJobStatus.PENDING,
                start_time=start_time,
                num_tasks=job.num_archives_to_search,
            )

    return reducer_acquisition_tasks


def try_getting_task_result(async_task_result):
    if not async_task_result.ready():
        return None
    return async_task_result.get()


def found_max_num_latest_results(
    results_cache_uri: str,
    job_id: str,
    max_num_results: int,
    max_timestamp_in_remaining_archives: int,
) -> bool:
    with pymongo.MongoClient(results_cache_uri) as results_cache_client:
        results_cache_collection = results_cache_client.get_default_database()[job_id]
        results_count = results_cache_collection.count_documents({})
        if results_count < max_num_results:
            return False

        results = list(
            results_cache_collection.find(
                projection=["timestamp"],
                sort=[("timestamp", pymongo.DESCENDING)],
                limit=max_num_results,
            )
            .sort("timestamp", pymongo.ASCENDING)
            .limit(1)
        )
        min_timestamp_in_top_results = 0 if len(results) == 0 else results[0]["timestamp"]
        return max_timestamp_in_remaining_archives <= min_timestamp_in_top_results


async def check_job_status_and_update_db(db_conn_pool, results_cache_uri):
    global active_jobs

    with contextlib.closing(db_conn_pool.connect()) as db_conn:
        for job_id in [
            id for id, job in active_jobs.items() if InternalJobState.RUNNING == job.state
        ]:
            job = active_jobs[job_id]
            is_reducer_job = job.reducer_handler_msg_queues is not None

            try:
                returned_results = try_getting_task_result(job.current_sub_job_async_task_result)
            except Exception as e:
                logger.error(f"Job `{job_id}` failed: {e}.")
                # Clean up
                if is_reducer_job:
                    msg = ReducerHandlerMessage(ReducerHandlerMessageType.FAILURE)
                    await job.reducer_handler_msg_queues.put_to_handler(msg)

                del active_jobs[job_id]
                set_job_or_task_status(
                    db_conn,
                    QUERY_JOBS_TABLE_NAME,
                    job_id,
                    QueryJobStatus.FAILED,
                    QueryJobStatus.RUNNING,
                    duration=(datetime.datetime.now() - job.start_time).total_seconds(),
                )
                continue

            if returned_results is None:
                continue

            new_job_status = QueryJobStatus.RUNNING
            for task_result_obj in returned_results:
                task_result = QueryTaskResult.parse_obj(task_result_obj)
                task_id = task_result.task_id
                task_status = task_result.status
                if not task_status == QueryTaskStatus.SUCCEEDED:
                    new_job_status = QueryJobStatus.FAILED
                    logger.error(
                        f"Search task job-{job_id}-task-{task_id} failed. "
                        f"Check {task_result.error_log_path} for details."
                    )
                else:
                    job.num_archives_searched += 1
                    logger.info(
                        f"Search task job-{job_id}-task-{task_id} succeeded in "
                        f"{task_result.duration} second(s)."
                    )

            if new_job_status != QueryJobStatus.FAILED:
                max_num_results = job.search_config.max_num_results
                # Check if we've searched all archives
                if len(job.remaining_archives_for_search) == 0:
                    new_job_status = QueryJobStatus.SUCCEEDED
                # Check if we've reached max results
                elif False == is_reducer_job and max_num_results > 0:
                    if found_max_num_latest_results(
                        results_cache_uri,
                        job_id,
                        max_num_results,
                        job.remaining_archives_for_search[0]["end_timestamp"],
                    ):
                        new_job_status = QueryJobStatus.SUCCEEDED
            if new_job_status == QueryJobStatus.RUNNING:
                job.current_sub_job_async_task_result = None
                job.state = InternalJobState.WAITING_FOR_DISPATCH
                logger.info(f"Job {job_id} waiting for more archives to search.")
                set_job_or_task_status(
                    db_conn,
                    QUERY_JOBS_TABLE_NAME,
                    job_id,
                    QueryJobStatus.RUNNING,
                    QueryJobStatus.RUNNING,
                    num_tasks_completed=job.num_archives_searched,
                )
                continue

            reducer_failed = False
            if is_reducer_job:
                # Notify reducer that it should have received all results
                msg = ReducerHandlerMessage(ReducerHandlerMessageType.SUCCESS)
                await job.reducer_handler_msg_queues.put_to_handler(msg)

                msg = await job.reducer_handler_msg_queues.get_from_handler()
                if ReducerHandlerMessageType.FAILURE == msg.msg_type:
                    reducer_failed = True
                    new_job_status = QueryJobStatus.FAILED
                elif ReducerHandlerMessageType.SUCCESS != msg.msg_type:
                    error_msg = f"Unexpected msg_type: {msg.msg_type.name}"
                    raise NotImplementedError(error_msg)

            # We set the status regardless of the job's previous status to handle the case where the
            # job is cancelled (status = CANCELLING) while we're in this method.
            if set_job_or_task_status(
                db_conn,
                QUERY_JOBS_TABLE_NAME,
                job_id,
                new_job_status,
                num_tasks_completed=job.num_archives_searched,
                duration=(datetime.datetime.now() - job.start_time).total_seconds(),
            ):
                if new_job_status == QueryJobStatus.SUCCEEDED:
                    logger.info(f"Completed job {job_id}.")
                elif reducer_failed:
                    logger.error(f"Completed job {job_id} with failing reducer.")
                else:
                    logger.info(f"Completed job {job_id} with failing tasks.")
            del active_jobs[job_id]


async def handle_job_updates(db_conn_pool, results_cache_uri: str, jobs_poll_delay: float):
    while True:
        await handle_cancelling_search_jobs(db_conn_pool)
        await check_job_status_and_update_db(db_conn_pool, results_cache_uri)
        await asyncio.sleep(jobs_poll_delay)


async def handle_jobs(
    db_conn_pool,
    clp_metadata_db_conn_params: Dict[str, any],
    results_cache_uri: str,
    jobs_poll_delay: float,
    num_archives_to_search_per_sub_job: int,
) -> None:
    handle_updating_task = asyncio.create_task(
        handle_job_updates(db_conn_pool, results_cache_uri, jobs_poll_delay)
    )

    tasks = [handle_updating_task]
    while True:
        reducer_acquisition_tasks = handle_pending_search_jobs(
            db_conn_pool,
            clp_metadata_db_conn_params,
            results_cache_uri,
            num_archives_to_search_per_sub_job,
        )
        if 0 == len(reducer_acquisition_tasks):
            tasks.append(asyncio.create_task(asyncio.sleep(jobs_poll_delay)))
        else:
            tasks.extend(reducer_acquisition_tasks)

        done, pending = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
        if handle_updating_task in done:
            logger.error("handle_job_updates completed unexpectedly.")
            try:
                handle_updating_task.result()
            except Exception:
                logger.exception("handle_job_updates failed.")
            return
        tasks = list(pending)


async def main(argv: List[str]) -> int:
    global reducer_connection_queue

    args_parser = argparse.ArgumentParser(description="Wait for and run query jobs.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")

    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / "query_scheduler.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    set_logging_level(logger, os.getenv("CLP_LOGGING_LEVEL"))

    # Load configuration
    config_path = pathlib.Path(parsed_args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
        return -1
    except Exception as ex:
        logger.error(ex)
        return -1

    reducer_connection_queue = asyncio.Queue(32)

    sql_adapter = SQL_Adapter(clp_config.database)

    logger.debug(f"Job polling interval {clp_config.query_scheduler.jobs_poll_delay} seconds.")
    try:
        reducer_handler = await asyncio.start_server(
            lambda reader, writer: handle_reducer_connection(
                reader, writer, reducer_connection_queue
            ),
            clp_config.query_scheduler.host,
            clp_config.query_scheduler.port,
        )
        db_conn_pool = sql_adapter.create_connection_pool(
            logger=logger, pool_size=2, disable_localhost_socket_connection=True
        )

        if False == db_conn_pool.alive():
            logger.error(
                f"Failed to connect to archive database "
                f"{clp_config.database.host}:{clp_config.database.port}."
            )
            return -1

        logger.info(
            f"Connected to archive database"
            f" {clp_config.database.host}:{clp_config.database.port}."
        )
        logger.info("Query scheduler started.")
        batch_size = clp_config.query_scheduler.num_archives_to_search_per_sub_job
        job_handler = asyncio.create_task(
            handle_jobs(
                db_conn_pool=db_conn_pool,
                clp_metadata_db_conn_params=clp_config.database.get_clp_connection_params_and_type(
                    True
                ),
                results_cache_uri=clp_config.results_cache.get_uri(),
                jobs_poll_delay=clp_config.query_scheduler.jobs_poll_delay,
                num_archives_to_search_per_sub_job=batch_size,
            )
        )
        reducer_handler = asyncio.create_task(reducer_handler.serve_forever())
        done, pending = await asyncio.wait(
            [job_handler, reducer_handler], return_when=asyncio.FIRST_COMPLETED
        )
        if reducer_handler in done:
            logger.error("reducer_handler completed unexpectedly.")
            try:
                reducer_handler.result()
            except Exception:
                logger.exception("reducer_handler failed.")
        if job_handler in done:
            logger.error("job_handler completed unexpectedly.")
            try:
                job_handler.result()
            except Exception:
                logger.exception("job_handler failed.")
    except Exception:
        logger.exception(f"Uncaught exception in job handling loop.")

    return 0


if "__main__" == __name__:
    sys.exit(asyncio.run(main(sys.argv)))
