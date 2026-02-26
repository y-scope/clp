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
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Any

import celery
import msgpack
import pymongo
from clp_py_utils.clp_config import (
    ClpConfig,
    QUERY_JOBS_TABLE_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_TASKS_TABLE_NAME,
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.clp_metadata_db_utils import (
    fetch_existing_datasets,
    get_archives_table_name,
    get_files_table_name,
)
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.decorators import exception_default_value
from clp_py_utils.sql_adapter import SqlAdapter
from pydantic import ValidationError

from job_orchestration.executor.query.extract_stream_task import extract_stream
from job_orchestration.executor.query.fs_search_task import search
from job_orchestration.garbage_collector.constants import MIN_TO_SECONDS, SECOND_TO_MILLISECOND
from job_orchestration.scheduler.constants import (
    QueryJobStatus,
    QueryJobType,
    QueryTaskStatus,
    SchedulerType,
)
from job_orchestration.scheduler.job_config import (
    ExtractIrJobConfig,
    ExtractJsonJobConfig,
    QueryJobConfig,
    SearchJobConfig,
)
from job_orchestration.scheduler.query.reducer_handler import (
    handle_reducer_connection,
    ReducerHandlerMessage,
    ReducerHandlerMessageQueues,
    ReducerHandlerMessageType,
)
from job_orchestration.scheduler.scheduler_data import (
    ExtractIrJob,
    ExtractJsonJob,
    InternalJobState,
    QueryJob,
    QueryTaskResult,
    SearchJob,
)
from job_orchestration.scheduler.utils import kill_hanging_jobs

# Setup logging
logger = get_logger("search-job-handler")

# Dictionary of active jobs indexed by job id
active_jobs: dict[str, QueryJob] = {}

# Dictionary that maps IDs of file splits being extracted to IDs of jobs waiting for them
active_file_split_ir_extractions: dict[str, list[str]] = {}

# Dictionary that maps IDs of clp-s archives being extracted to IDs of jobs waiting for them
active_archive_json_extractions: dict[str, list[str]] = {}

reducer_connection_queue: asyncio.Queue | None = None


class StreamExtractionHandle(ABC):
    def __init__(self, job_id: str):
        self._job_id = job_id
        self._archive_id: str | None = None

    def get_archive_id(self) -> str | None:
        return self._archive_id

    @abstractmethod
    def get_stream_id(self) -> str: ...

    @abstractmethod
    def is_stream_extraction_active(self) -> bool: ...

    @abstractmethod
    def is_stream_extracted(self, results_cache_uri: str, stream_collection_name: str) -> bool: ...

    @abstractmethod
    def mark_job_as_waiting(self) -> None: ...

    @abstractmethod
    def create_stream_extraction_job(self) -> QueryJob: ...


class IrExtractionHandle(StreamExtractionHandle):
    def __init__(
        self,
        job_id: str,
        job_config: dict[str, Any],
        db_conn,
        table_prefix: str,
    ):
        super().__init__(job_id)
        self.__job_config = ExtractIrJobConfig.model_validate(job_config)
        self._archive_id, self.__file_split_id = get_archive_and_file_split_ids_for_ir_extraction(
            db_conn, table_prefix, self.__job_config
        )
        if self._archive_id is None:
            raise ValueError("Job parameters don't resolve to an existing archive")

        self.__job_config.file_split_id = self.__file_split_id

    def get_stream_id(self) -> str:
        return self.__file_split_id

    def is_stream_extraction_active(self) -> bool:
        return self.__file_split_id in active_file_split_ir_extractions

    def is_stream_extracted(self, results_cache_uri: str, stream_collection_name: str) -> bool:
        return document_exists(
            results_cache_uri, stream_collection_name, "file_split_id", self.__file_split_id
        )

    def mark_job_as_waiting(self) -> None:
        global active_file_split_ir_extractions
        file_split_id = self.__file_split_id
        if file_split_id not in active_file_split_ir_extractions:
            active_file_split_ir_extractions[file_split_id] = []
        active_file_split_ir_extractions[file_split_id].append(self._job_id)

    def create_stream_extraction_job(self) -> QueryJob:
        logger.info(
            f"Creating IR extraction job {self._job_id} for file_split: {self.__file_split_id}"
        )
        return ExtractIrJob(
            id=self._job_id,
            extract_ir_config=self.__job_config,
            state=InternalJobState.WAITING_FOR_DISPATCH,
        )


class JsonExtractionHandle(StreamExtractionHandle):
    def __init__(
        self,
        job_id: str,
        job_config: dict[str, Any],
        db_conn,
        table_prefix: str,
    ):
        super().__init__(job_id)
        self.__job_config = ExtractJsonJobConfig.model_validate(job_config)
        self._archive_id = self.__job_config.archive_id
        if not archive_exists(db_conn, table_prefix, self.__job_config.dataset, self._archive_id):
            raise ValueError(f"Archive {self._archive_id} doesn't exist")

    def get_stream_id(self) -> str:
        return self._archive_id

    def is_stream_extraction_active(self) -> bool:
        return self._archive_id in active_archive_json_extractions

    def is_stream_extracted(self, results_cache_uri: str, stream_collection_name: str) -> bool:
        return document_exists(
            results_cache_uri, stream_collection_name, "orig_file_id", self._archive_id
        )

    def mark_job_as_waiting(self) -> None:
        global active_archive_json_extractions
        archive_id = self._archive_id
        if archive_id not in active_archive_json_extractions:
            active_archive_json_extractions[archive_id] = []
        active_archive_json_extractions[archive_id].append(self._job_id)

    def create_stream_extraction_job(self) -> QueryJob:
        logger.info(f"Creating json extraction job {self._job_id} on archive: {self._archive_id}")
        return ExtractJsonJob(
            id=self._job_id,
            extract_json_config=self.__job_config,
            state=InternalJobState.WAITING_FOR_DISPATCH,
        )


def document_exists(mongodb_uri, collection_name, field, value):
    with pymongo.MongoClient(mongodb_uri) as mongo_client:
        collection = mongo_client.get_default_database()[collection_name]
        return 0 != collection.count_documents({field: value})


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
def fetch_new_query_jobs(db_conn) -> list:
    """
    Fetches query jobs with status=PENDING from the database.
    :param db_conn:
    :return: The pending query jobs on success. An empty list if an exception occurs while
    interacting with the database.
    """
    with contextlib.closing(db_conn.cursor(dictionary=True)) as db_cursor:
        db_cursor.execute(
            f"""
            SELECT {QUERY_JOBS_TABLE_NAME}.id as job_id,
            {QUERY_JOBS_TABLE_NAME}.job_config,
            {QUERY_JOBS_TABLE_NAME}.type,
            {QUERY_JOBS_TABLE_NAME}.creation_time
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
            AND {QUERY_JOBS_TABLE_NAME}.type={QueryJobType.SEARCH_OR_AGGREGATION}
            """
        )
        return db_cursor.fetchall()


@exception_default_value(default=False)
def set_job_or_task_status(
    db_conn,
    table_name: str,
    job_id: str,
    status: QueryJobStatus | QueryTaskStatus,
    prev_status: QueryJobStatus | QueryTaskStatus | None = None,
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
        f"UPDATE {table_name} SET {', '.join(field_set_expressions)} WHERE {id_col_name}={job_id}"
    )

    if prev_status is not None:
        update += f" AND status={prev_status}"

    with contextlib.closing(db_conn.cursor()) as cursor:
        cursor.execute(update)
        row_changed = cursor.rowcount != 0
        db_conn.commit()
    return row_changed


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

            set_job_or_task_status_kwargs = {}
            if job.start_time is not None:
                set_job_or_task_status_kwargs["duration"] = (
                    datetime.datetime.now() - job.start_time
                ).total_seconds()

            if set_job_or_task_status(
                db_conn,
                QUERY_JOBS_TABLE_NAME,
                job_id,
                QueryJobStatus.CANCELLED,
                QueryJobStatus.CANCELLING,
                **set_job_or_task_status_kwargs,
            ):
                logger.info(f"Cancelled job {job_id}.")
            else:
                logger.error(f"Failed to cancel job {job_id}.")


def insert_query_tasks_into_db(db_conn, job_id, archive_ids: list[str]) -> list[int]:
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
    table_prefix: str,
    search_config: SearchJobConfig,
    archive_end_ts_lower_bound: int | None,
):
    dataset = search_config.dataset
    query = f"""SELECT id as archive_id, end_timestamp
            FROM {get_archives_table_name(table_prefix, dataset)}
            """
    filter_clauses = []
    if search_config.end_timestamp is not None:
        filter_clauses.append(f"begin_timestamp <= {search_config.end_timestamp}")
    if search_config.begin_timestamp is not None:
        filter_clauses.append(f"end_timestamp >= {search_config.begin_timestamp}")
    if archive_end_ts_lower_bound is not None:
        filter_clauses.append(
            f"(end_timestamp >= {archive_end_ts_lower_bound} OR end_timestamp = 0)"
        )
    if len(filter_clauses) > 0:
        query += " WHERE " + " AND ".join(filter_clauses)
    query += " ORDER BY end_timestamp DESC"

    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
        cursor.execute(query)
        archives_for_search = list(cursor.fetchall())
    return archives_for_search


def get_archive_and_file_split_ids_for_ir_extraction(
    db_conn,
    table_prefix: str,
    extract_ir_config: ExtractIrJobConfig,
) -> tuple[str | None, str | None]:
    orig_file_id = extract_ir_config.orig_file_id
    msg_ix = extract_ir_config.msg_ix

    results = get_archive_and_file_split_ids(db_conn, table_prefix, orig_file_id, msg_ix)
    if len(results) == 0:
        logger.error(f"No matching file splits for orig_file_id={orig_file_id}, msg_ix={msg_ix}")
        return None, None
    if len(results) > 1:
        logger.error(f"Multiple file splits found for orig_file_id={orig_file_id}, msg_ix={msg_ix}")
        for result in results:
            logger.error(f"{result['archive_id']}:{result['id']}")
        return None, None

    return results[0]["archive_id"], results[0]["file_split_id"]


@exception_default_value(default=[])
def get_archive_and_file_split_ids(
    db_conn,
    table_prefix: str,
    orig_file_id: str,
    msg_ix: int,
):
    """
    Fetches the IDs of the file split and the archive containing the file split based on the
    following criteria:
    1. The file split's original file id = `orig_file_id`
    2. The file split includes the message with index = `msg_ix`
    :param db_conn:
    :param table_prefix:
    :param orig_file_id: Original file id of the split
    :param msg_ix: Index of the message that the file split must include
    :return: A list of (archive id, file split id) on success. An empty list if
    an exception occurs while interacting with the database.
    """
    query = f"""SELECT archive_id, id as file_split_id
            FROM {get_files_table_name(table_prefix, None)} WHERE
            orig_file_id = '{orig_file_id}' AND
            begin_message_ix <= {msg_ix} AND
            (begin_message_ix + num_messages) > {msg_ix}
            """

    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
        cursor.execute(query)
        results = list(cursor.fetchall())
    return results


@exception_default_value(default=False)
def archive_exists(
    db_conn,
    table_prefix: str,
    dataset: str | None,
    archive_id: str,
) -> bool:
    archives_table_name = get_archives_table_name(table_prefix, dataset)
    query = f"SELECT 1 FROM {archives_table_name} WHERE id = %s"
    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
        cursor.execute(query, (archive_id,))
        if cursor.fetchone():
            return True

    return False


def get_task_group_for_job(
    archive_ids: list[str],
    task_ids: list[int],
    job: QueryJob,
    clp_metadata_db_conn_params: dict[str, any],
    results_cache_uri: str,
):
    job_config = job.get_config().model_dump()
    job_type = job.get_type()
    if QueryJobType.SEARCH_OR_AGGREGATION == job_type:
        return celery.group(
            search.s(
                job_id=job.id,
                archive_id=archive_ids[i],
                task_id=task_ids[i],
                job_config=job_config,
                clp_metadata_db_conn_params=clp_metadata_db_conn_params,
                results_cache_uri=results_cache_uri,
            )
            for i in range(len(archive_ids))
        )
    if job_type in (QueryJobType.EXTRACT_JSON, QueryJobType.EXTRACT_IR):
        return celery.group(
            extract_stream.s(
                job_id=job.id,
                archive_id=archive_ids[i],
                task_id=task_ids[i],
                job_config=job_config,
                clp_metadata_db_conn_params=clp_metadata_db_conn_params,
                results_cache_uri=results_cache_uri,
            )
            for i in range(len(archive_ids))
        )
    error_msg = f"Unexpected job type: {job_type}"
    logger.error(error_msg)
    raise NotImplementedError(error_msg)


def dispatch_query_job(
    db_conn,
    job: QueryJob,
    archive_ids: list[str],
    clp_metadata_db_conn_params: dict[str, any],
    results_cache_uri: str,
) -> None:
    global active_jobs
    task_ids = insert_query_tasks_into_db(db_conn, job.id, archive_ids)

    task_group = get_task_group_for_job(
        archive_ids,
        task_ids,
        job,
        clp_metadata_db_conn_params,
        results_cache_uri,
    )
    job.current_sub_job_async_task_result = task_group.apply_async()
    job.state = InternalJobState.RUNNING


async def acquire_reducer_for_job(job: SearchJob):
    reducer_host: str | None = None
    reducer_port: int | None = None
    reducer_handler_msg_queues: ReducerHandlerMessageQueues | None = None
    while True:
        (
            reducer_host,
            reducer_port,
            reducer_handler_msg_queues,
        ) = await reducer_connection_queue.get()
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
            if msg.msg_type != ReducerHandlerMessageType.FAILURE:
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


def dispatch_job_and_update_db(
    db_conn,
    new_job: QueryJob,
    target_archives: list[str],
    clp_metadata_db_conn_params: dict[str, any],
    results_cache_uri: str,
    num_tasks: int,
) -> None:
    dispatch_query_job(
        db_conn, new_job, target_archives, clp_metadata_db_conn_params, results_cache_uri
    )
    start_time = datetime.datetime.now()
    if new_job.start_time is None:
        new_job.start_time = start_time
    set_job_or_task_status(
        db_conn,
        QUERY_JOBS_TABLE_NAME,
        new_job.id,
        QueryJobStatus.RUNNING,
        QueryJobStatus.PENDING,
        start_time=start_time,
        num_tasks=num_tasks,
    )


def handle_pending_query_jobs(
    db_conn_pool,
    clp_metadata_db_conn_params: dict[str, any],
    results_cache_uri: str,
    stream_collection_name: str,
    num_archives_to_search_per_sub_job: int,
    existing_datasets: set[str],
    archive_retention_period: int | None,
) -> list[asyncio.Task]:
    global active_jobs

    reducer_acquisition_tasks = []
    pending_search_jobs = [
        job
        for job in active_jobs.values()
        if InternalJobState.WAITING_FOR_DISPATCH == job.state
        and job.get_type() == QueryJobType.SEARCH_OR_AGGREGATION
    ]

    with (
        contextlib.closing(db_conn_pool.connect()) as db_conn,
        contextlib.closing(db_conn.cursor(dictionary=True)) as db_cursor,
    ):
        for job in fetch_new_query_jobs(db_conn):
            job_id = str(job["job_id"])
            job_type = job["type"]
            job_config = msgpack.unpackb(job["job_config"])
            job_creation_time = job["creation_time"].timestamp()

            table_prefix = clp_metadata_db_conn_params["table_prefix"]
            dataset = QueryJobConfig.model_validate(job_config).dataset
            if dataset is not None and dataset not in existing_datasets:
                # NOTE: This assumes we never delete a dataset.
                existing_datasets.update(fetch_existing_datasets(db_cursor, table_prefix))
                if dataset not in existing_datasets:
                    logger.error(f"Dataset `{dataset}` doesn't exist.")
                    if not set_job_or_task_status(
                        db_conn,
                        QUERY_JOBS_TABLE_NAME,
                        job_id,
                        QueryJobStatus.FAILED,
                        QueryJobStatus.PENDING,
                        start_time=datetime.datetime.now(),
                        duration=0,
                    ):
                        logger.error(f"Failed to set job {job_id} as failed.")
                    continue

            if QueryJobType.SEARCH_OR_AGGREGATION == job_type:
                # Avoid double-dispatch when a job is WAITING_FOR_REDUCER
                if job_id in active_jobs:
                    continue

                search_config = SearchJobConfig.model_validate(job_config)
                archive_end_ts_lower_bound: int | None = None
                if archive_retention_period is not None:
                    archive_end_ts_lower_bound = SECOND_TO_MILLISECOND * (
                        job_creation_time - archive_retention_period * MIN_TO_SECONDS
                    )

                archives_for_search = get_archives_for_search(
                    db_conn, table_prefix, search_config, archive_end_ts_lower_bound
                )
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
                        logger.info(f"No matching archives, skipping job {job_id}.")
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
                    new_search_job.search_config.aggregation_config.job_id = int(job_id)
                    new_search_job.state = InternalJobState.WAITING_FOR_REDUCER
                    new_search_job.reducer_acquisition_task = asyncio.create_task(
                        acquire_reducer_for_job(new_search_job)
                    )
                    reducer_acquisition_tasks.append(new_search_job.reducer_acquisition_task)
                else:
                    pending_search_jobs.append(new_search_job)
                active_jobs[job_id] = new_search_job

            elif job_type in (QueryJobType.EXTRACT_IR, QueryJobType.EXTRACT_JSON):
                job_handle: StreamExtractionHandle
                try:
                    if QueryJobType.EXTRACT_IR == job_type:
                        job_handle = IrExtractionHandle(job_id, job_config, db_conn, table_prefix)
                    else:
                        job_handle = JsonExtractionHandle(job_id, job_config, db_conn, table_prefix)
                except ValueError:
                    logger.exception("Failed to initialize extraction job handle")
                    if not set_job_or_task_status(
                        db_conn,
                        QUERY_JOBS_TABLE_NAME,
                        job_id,
                        QueryJobStatus.FAILED,
                        QueryJobStatus.PENDING,
                        start_time=datetime.datetime.now(),
                        num_tasks=0,
                        duration=0,
                    ):
                        logger.error(f"Failed to set job {job_id} as failed")
                    continue

                # NOTE: The following two if blocks for `is_stream_extraction_active` and
                # `is_stream_extracted` should not be reordered.
                #
                # The logic below works as follows:
                # 1. It checks if a stream is already being extracted
                #    (`is_stream_extraction_active`) and if so, it marks the new job as waiting for
                #    the old job to finish.
                # 2. Otherwise, it checks if a stream has already been extracted
                #    (`is_stream_extracted`) and if so, it marks the new job as complete.
                # 3. Otherwise, it creates a new stream extraction job.
                #
                # `is_stream_extracted` only checks if a single stream has been extracted rather
                # than whether all required streams have been extracted. This means that we can't
                # use it to check if the old job is complete; instead, we need to employ the
                # aforementioned logic.

                # Check if the required streams are currently being extracted; if so, add the job ID
                # to the list of jobs waiting for it.
                if job_handle.is_stream_extraction_active():
                    job_handle.mark_job_as_waiting()
                    logger.info(
                        f"Stream {job_handle.get_stream_id()} is already being extracted,"
                        f" so mark job {job_id} as running."
                    )
                    if not set_job_or_task_status(
                        db_conn,
                        QUERY_JOBS_TABLE_NAME,
                        job_id,
                        QueryJobStatus.RUNNING,
                        QueryJobStatus.PENDING,
                        start_time=datetime.datetime.now(),
                        num_tasks=0,
                    ):
                        logger.error(f"Failed to set job {job_id} as running")
                    continue

                # Check if a required stream file has already been extracted
                if job_handle.is_stream_extracted(results_cache_uri, stream_collection_name):
                    logger.info(
                        f"Stream {job_handle.get_stream_id()} already extracted,"
                        f" so mark job {job_id} as succeeded."
                    )
                    if not set_job_or_task_status(
                        db_conn,
                        QUERY_JOBS_TABLE_NAME,
                        job_id,
                        QueryJobStatus.SUCCEEDED,
                        QueryJobStatus.PENDING,
                        start_time=datetime.datetime.now(),
                        num_tasks=0,
                        duration=0,
                    ):
                        logger.error(f"Failed to set job {job_id} as succeeded")
                    continue

                new_stream_extraction_job = job_handle.create_stream_extraction_job()
                archive_id = job_handle.get_archive_id()
                dispatch_job_and_update_db(
                    db_conn,
                    new_stream_extraction_job,
                    [archive_id],
                    clp_metadata_db_conn_params,
                    results_cache_uri,
                    1,
                )

                job_handle.mark_job_as_waiting()
                active_jobs[job_id] = new_stream_extraction_job
                logger.info(f"Dispatched stream extraction job {job_id} for archive: {archive_id}")

            else:
                # NOTE: We're skipping the job for this iteration, but its status will remain
                # unchanged. So this log will print again in the next iteration unless the user
                # cancels the job.
                logger.error(f"Unexpected job type: {job_type}, skipping job {job_id}")
                continue

        for job in pending_search_jobs:
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

            archive_ids_for_search = [archive["archive_id"] for archive in archives_for_search]

            dispatch_job_and_update_db(
                db_conn,
                job,
                archive_ids_for_search,
                clp_metadata_db_conn_params,
                results_cache_uri,
                job.num_archives_to_search,
            )
            logger.info(
                f"Dispatched job {job_id} with {len(archive_ids_for_search)} archives to search."
            )

    return reducer_acquisition_tasks


def try_getting_task_result(async_task_result):
    if not async_task_result.ready():
        return None
    return async_task_result.get(interval=0.005)


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


async def handle_finished_search_job(
    db_conn, job: SearchJob, task_results: Any | None, results_cache_uri: str
) -> None:
    global active_jobs

    job_id = job.id
    is_reducer_job = job.reducer_handler_msg_queues is not None
    new_job_status = QueryJobStatus.RUNNING
    for task_result_obj in task_results:
        task_result = QueryTaskResult.model_validate(task_result_obj)
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
        return

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


async def handle_finished_stream_extraction_job(
    db_conn, job: QueryJob, task_results: list[Any]
) -> None:
    global active_jobs
    global active_archive_json_extractions
    global active_file_split_ir_extractions

    job_id = job.id
    new_job_status = QueryJobStatus.SUCCEEDED

    num_tasks = len(task_results)
    if 1 != num_tasks:
        logger.error(
            f"Unexpected number of tasks for extraction job {job_id}. Expected 1, got {num_tasks}."
        )
        new_job_status = QueryJobStatus.FAILED
    else:
        task_result = QueryTaskResult.model_validate(task_results[0])
        task_id = task_result.task_id
        if not QueryTaskStatus.SUCCEEDED == task_result.status:
            logger.error(
                f"Extraction task job-{job_id}-task-{task_id} failed. "
                f"Check {task_result.error_log_path} for details."
            )
            new_job_status = QueryJobStatus.FAILED
        else:
            logger.info(
                f"Extraction task job-{job_id}-task-{task_id} succeeded in "
                f"{task_result.duration} second(s)."
            )

    if set_job_or_task_status(
        db_conn,
        QUERY_JOBS_TABLE_NAME,
        job_id,
        new_job_status,
        QueryJobStatus.RUNNING,
        num_tasks_completed=num_tasks,
        duration=(datetime.datetime.now() - job.start_time).total_seconds(),
    ):
        if new_job_status == QueryJobStatus.SUCCEEDED:
            logger.info(f"Completed stream extraction job {job_id}.")
        else:
            logger.info(f"Completed stream extraction job {job_id} with failing tasks.")

    waiting_jobs: list[str]
    if QueryJobType.EXTRACT_IR == job.get_type():
        extract_ir_config: ExtractIrJobConfig = job.get_config()
        waiting_jobs = active_file_split_ir_extractions.pop(extract_ir_config.file_split_id)
    else:
        extract_json_config: ExtractJsonJobConfig = job.get_config()
        waiting_jobs = active_archive_json_extractions.pop(extract_json_config.archive_id)

    waiting_jobs.remove(job_id)
    for waiting_job in waiting_jobs:
        logger.info(f"Setting status to {new_job_status.to_str()} for waiting jobs: {waiting_job}.")
        set_job_or_task_status(
            db_conn,
            QUERY_JOBS_TABLE_NAME,
            waiting_job,
            new_job_status,
            QueryJobStatus.RUNNING,
            num_tasks_completed=0,
            duration=(datetime.datetime.now() - job.start_time).total_seconds(),
        )

    del active_jobs[job_id]


async def check_job_status_and_update_db(db_conn_pool, results_cache_uri):
    global active_jobs

    with contextlib.closing(db_conn_pool.connect()) as db_conn:
        for job_id in [
            id for id, job in active_jobs.items() if InternalJobState.RUNNING == job.state
        ]:
            job = active_jobs[job_id]
            try:
                returned_results = try_getting_task_result(job.current_sub_job_async_task_result)
            except Exception as e:
                logger.error(f"Job `{job_id}` failed: {e}.")
                # Clean up
                if QueryJobType.SEARCH_OR_AGGREGATION == job.get_type():
                    if job.reducer_handler_msg_queues is not None:
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
            job_type = job.get_type()
            if QueryJobType.SEARCH_OR_AGGREGATION == job_type:
                search_job: SearchJob = job
                await handle_finished_search_job(
                    db_conn, search_job, returned_results, results_cache_uri
                )
            elif job_type in (QueryJobType.EXTRACT_JSON, QueryJobType.EXTRACT_IR):
                await handle_finished_stream_extraction_job(db_conn, job, returned_results)
            else:
                logger.error(f"Unexpected job type: {job_type}, skipping job {job_id}")


async def handle_job_updates(db_conn_pool, results_cache_uri: str, jobs_poll_delay: float):
    while True:
        interval_start_time = datetime.datetime.now()
        await handle_cancelling_search_jobs(db_conn_pool)
        await check_job_status_and_update_db(db_conn_pool, results_cache_uri)
        interval_end_time = datetime.datetime.now()
        await asyncio.sleep(
            jobs_poll_delay - (interval_end_time - interval_start_time).total_seconds()
        )


async def handle_jobs(
    db_conn_pool,
    clp_metadata_db_conn_params: dict[str, any],
    results_cache_uri: str,
    stream_collection_name: str,
    jobs_poll_delay: float,
    num_archives_to_search_per_sub_job: int,
    archive_retention_period: int | None,
) -> None:
    handle_updating_task = asyncio.create_task(
        handle_job_updates(db_conn_pool, results_cache_uri, jobs_poll_delay)
    )

    tasks = [handle_updating_task]
    existing_datasets: set[str] = set()
    while True:
        reducer_acquisition_tasks = handle_pending_query_jobs(
            db_conn_pool,
            clp_metadata_db_conn_params,
            results_cache_uri,
            stream_collection_name,
            num_archives_to_search_per_sub_job,
            existing_datasets,
            archive_retention_period,
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


async def main(argv: list[str]) -> int:
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
        clp_config = ClpConfig.model_validate(read_yaml_config_file(config_path))
        clp_config.database.load_credentials_from_env()
    except (ValidationError, ValueError) as err:
        logger.error(err)
        return -1
    except Exception:
        logger.exception(f"Failed to initialize {QUERY_SCHEDULER_COMPONENT_NAME}.")
        return -1

    reducer_connection_queue = asyncio.Queue(32)

    sql_adapter = SqlAdapter(clp_config.database)

    try:
        killed_jobs = kill_hanging_jobs(sql_adapter, SchedulerType.QUERY)
        if killed_jobs is not None:
            logger.info(f"Killed {len(killed_jobs)} hanging query jobs.")
    except Exception:
        logger.exception("Failed to kill hanging query jobs.")
        return -1

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
            f"Connected to archive database {clp_config.database.host}:{clp_config.database.port}."
        )
        logger.info(f"{QUERY_SCHEDULER_COMPONENT_NAME} started.")
        batch_size = clp_config.query_scheduler.num_archives_to_search_per_sub_job
        job_handler = asyncio.create_task(
            handle_jobs(
                db_conn_pool=db_conn_pool,
                clp_metadata_db_conn_params=clp_config.database.get_clp_connection_params_and_type(
                    True
                ),
                results_cache_uri=clp_config.results_cache.get_uri(),
                stream_collection_name=clp_config.results_cache.stream_collection_name,
                jobs_poll_delay=clp_config.query_scheduler.jobs_poll_delay,
                num_archives_to_search_per_sub_job=batch_size,
                archive_retention_period=clp_config.archive_output.retention_period,
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
        logger.exception("Uncaught exception in job handling loop.")

    return 0


if "__main__" == __name__:
    sys.exit(asyncio.run(main(sys.argv)))
