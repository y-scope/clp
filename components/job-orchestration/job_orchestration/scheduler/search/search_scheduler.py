#!/usr/bin/env python3

import argparse
import asyncio
import contextlib
import logging
import os
import pathlib
import sys
from pathlib import Path
from typing import Dict, List, Optional

import celery
import msgpack
import pymongo
from clp_py_utils.clp_config import CLP_METADATA_TABLE_PREFIX, CLPConfig, SEARCH_JOBS_TABLE_NAME
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.search.fs_search_task import search
from job_orchestration.scheduler.constants import SearchJobStatus
from job_orchestration.scheduler.job_config import SearchConfig
from job_orchestration.scheduler.scheduler_data import SearchJob, SearchTaskResult
from pydantic import ValidationError

# Setup logging
logger = get_logger("search-job-handler")

# Dictionary of active jobs indexed by job id
active_jobs: Dict[str, SearchJob] = {}

reducer_connection_queue = None


async def cancel_job(job_id):
    global active_jobs
    active_jobs[job_id].current_sub_job_async_task_result.revoke(terminate=True)
    try:
        active_jobs[job_id].current_sub_job_async_task_result.get()
    except Exception:
        pass
    if active_jobs[job_id].reducer_send_handle is not None:
        await active_jobs[job_id].reducer_send_handle.put(False)
    del active_jobs[job_id]


def fetch_new_search_jobs(db_cursor) -> list:
    db_cursor.execute(
        f"""
        SELECT {SEARCH_JOBS_TABLE_NAME}.id as job_id,
        {SEARCH_JOBS_TABLE_NAME}.search_config
        FROM {SEARCH_JOBS_TABLE_NAME}
        WHERE {SEARCH_JOBS_TABLE_NAME}.status={SearchJobStatus.PENDING}
        """
    )
    return db_cursor.fetchall()


def fetch_cancelling_search_jobs(db_cursor) -> list:
    db_cursor.execute(
        f"""
        SELECT {SEARCH_JOBS_TABLE_NAME}.id as job_id
        FROM {SEARCH_JOBS_TABLE_NAME}
        WHERE {SEARCH_JOBS_TABLE_NAME}.status={SearchJobStatus.CANCELLING}
        """
    )
    return db_cursor.fetchall()


def set_job_status(
    db_conn,
    job_id: str,
    status: SearchJobStatus,
    prev_status: Optional[SearchJobStatus] = None,
    **kwargs,
) -> bool:
    field_set_expressions = [f'{k}="{v}"' for k, v in kwargs.items()]
    field_set_expressions.append(f"status={status}")
    update = (
        f'UPDATE {SEARCH_JOBS_TABLE_NAME} SET {", ".join(field_set_expressions)} WHERE id={job_id}'
    )

    if prev_status is not None:
        update += f" AND status={prev_status}"

    with contextlib.closing(db_conn.cursor()) as cursor:
        cursor.execute(update)
        db_conn.commit()
        rval = cursor.rowcount != 0
    return rval


async def handle_cancelling_search_jobs(db_conn) -> None:
    global active_jobs

    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
        cancelling_jobs = fetch_cancelling_search_jobs(cursor)
        db_conn.commit()

    for job in cancelling_jobs:
        job_id = job["job_id"]
        if (
            job_id in active_jobs
            and active_jobs[job_id].current_sub_job_async_task_result is not None
        ):
            await cancel_job(job_id)
        else:
            continue
        if set_job_status(
            db_conn, job_id, SearchJobStatus.CANCELLED, prev_status=SearchJobStatus.CANCELLING
        ):
            logger.info(f"Cancelled job {job_id}.")
        else:
            logger.error(f"Failed to cancel job {job_id}.")


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
        db_conn.commit()
    return archives_for_search


def get_task_group_for_job(
    archives_for_search: List[Dict[str, any]],
    job_id: str,
    search_config: SearchConfig,
    results_cache_uri: str,
):
    search_config_obj = search_config.dict()
    return celery.group(
        search.s(
            job_id=job_id,
            archive_id=archive["archive_id"],
            search_config_obj=search_config_obj,
            results_cache_uri=results_cache_uri,
        )
        for archive in archives_for_search
    )


def dispatch_search_job(
    job: SearchJob,
    archives_for_search: List[Dict[str, any]],
    results_cache_uri: str,
) -> None:
    global active_jobs
    task_group = get_task_group_for_job(
        archives_for_search, job.id, job.search_config, results_cache_uri
    )
    job.current_sub_job_async_task_result = task_group.apply_async()


async def handle_pending_search_jobs(
    db_conn, results_cache_uri: str, num_archives_to_search_per_sub_job: int, jobs_poll_delay: float
) -> None:
    global active_jobs

    while True:
        pending_jobs = [
            job for job in active_jobs.values() if job.current_sub_job_async_task_result is None
        ]
        with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
            for job in fetch_new_search_jobs(cursor):
                job_id = str(job["job_id"])
                search_config = SearchConfig.parse_obj(msgpack.unpackb(job["search_config"]))
                archives_for_search = get_archives_for_search(db_conn, search_config)
                if len(archives_for_search) == 0:
                    if set_job_status(
                        db_conn, job_id, SearchJobStatus.SUCCEEDED, SearchJobStatus.PENDING
                    ):
                        logger.info(f"No matching archives, skipping job {job['job_id']}.")
                    continue

                reducer_recv_handle = None
                reducer_send_handle = None
                if search_config.count is not None:
                    search_config.job_id = job["job_id"]
                    # Get a reducer and wait for it to be ready
                    while True:
                        reducer_host, reducer_port, reducer_recv_handle, reducer_send_handle = (
                            await reducer_connection_queue.get()
                        )
                        await reducer_send_handle.put(search_config)
                        if True == await reducer_recv_handle.get():
                            break

                    search_config.reducer_host = reducer_host
                    search_config.reducer_port = reducer_port
                    logger.info(f"Got reducer for job {job_id} at {reducer_host}:{reducer_port}")

                new_search_job = SearchJob(
                    id=job_id,
                    search_config=search_config,
                    remaining_archives_for_search=archives_for_search,
                    current_sub_job_async_task_result=None,
                    reducer_recv_handle=reducer_recv_handle,
                    reducer_send_handle=reducer_send_handle,
                )
                active_jobs[job_id] = new_search_job
                pending_jobs.append(new_search_job)
            db_conn.commit()

        for job in pending_jobs:
            job_id = job.id

            if len(job.remaining_archives_for_search) > num_archives_to_search_per_sub_job:
                archives_for_search = job.remaining_archives_for_search[
                    :num_archives_to_search_per_sub_job
                ]
                job.remaining_archives_for_search = job.remaining_archives_for_search[
                    num_archives_to_search_per_sub_job:
                ]
            else:
                archives_for_search = job.remaining_archives_for_search
                job.remaining_archives_for_search = []

            dispatch_search_job(job, archives_for_search, results_cache_uri)
            logger.info(
                f"Dispatched job {job_id} with {len(archives_for_search)} archives to search."
            )
            set_job_status(db_conn, job_id, SearchJobStatus.RUNNING, SearchJobStatus.PENDING)

        await asyncio.sleep(jobs_poll_delay)


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


async def check_job_status_and_update_db(db_conn, results_cache_uri):
    global active_jobs

    for job_id in [
        id for id, job in active_jobs.items() if job.current_sub_job_async_task_result is not None
    ]:
        job = active_jobs[job_id]
        try:
            returned_results = try_getting_task_result(job.current_sub_job_async_task_result)
        except Exception as e:
            logger.error(f"Job `{job_id}` failed: {e}.")
            # clean up
            if active_jobs[job_id].reducer_send_handle is not None:
                await active_jobs[job_id].reducer_send_handle.put(False)
            del active_jobs[job_id]
            set_job_status(db_conn, job_id, SearchJobStatus.FAILED, SearchJobStatus.RUNNING)
            continue

        if returned_results is not None:
            is_reducer_job = active_jobs[job_id].reducer_send_handle is not None
            new_job_status = SearchJobStatus.RUNNING
            for task_result_obj in returned_results:
                task_result = SearchTaskResult.parse_obj(task_result_obj)
                if not task_result.success:
                    task_id = task_result.task_id
                    new_job_status = SearchJobStatus.FAILED
                    logger.debug(f"Task {task_id} failed - result {task_result}.")

            if new_job_status != SearchJobStatus.FAILED:
                max_num_results = job.search_config.max_num_results
                # Check if we've searched all archives
                if len(job.remaining_archives_for_search) == 0:
                    new_job_status = SearchJobStatus.SUCCEEDED
                # Check if we've reached max results
                elif False == is_reducer_job and max_num_results > 0:
                    if found_max_num_latest_results(
                        results_cache_uri,
                        job_id,
                        max_num_results,
                        job.remaining_archives_for_search[0]["end_timestamp"],
                    ):
                        new_job_status = SearchJobStatus.SUCCEEDED
            if new_job_status == SearchJobStatus.RUNNING:
                job.current_sub_job_async_task_result = None
                logger.info(f"Job {job_id} waiting for more archives to search.")
                return

            reducer_failed = False
            if active_jobs[job_id].reducer_send_handle is not None:
                # Notify reducer that it should have received all of the results
                await active_jobs[job_id].reducer_send_handle.put(True)
                if False == await active_jobs[job_id].reducer_recv_handle.get():
                    reducer_failed = True
                    new_job_status = SearchJobStatus.FAILED

            if set_job_status(db_conn, job_id, new_job_status, SearchJobStatus.RUNNING):
                if new_job_status == SearchJobStatus.SUCCEEDED:
                    logger.info(f"Completed job {job_id}.")
                elif reducer_failed:
                    logger.error(f"Completed job {job_id} with failing reducer.")
                else:
                    logger.info(f"Completed job {job_id} with failing tasks.")
            del active_jobs[job_id]


async def handle_job_updates(db_conn, results_cache_uri: str, jobs_poll_delay: float):
    while True:
        await handle_cancelling_search_jobs(db_conn)
        await check_job_status_and_update_db(db_conn, results_cache_uri)
        await asyncio.sleep(jobs_poll_delay)


async def handle_jobs(
    db_conn_job_fetcher,
    db_conn_job_updater,
    results_cache_uri: str,
    jobs_poll_delay: float,
    num_archives_to_search_per_sub_job: int,
) -> None:
    handle_pending_task = asyncio.create_task(
        handle_pending_search_jobs(
            db_conn_job_fetcher,
            results_cache_uri,
            num_archives_to_search_per_sub_job,
            jobs_poll_delay,
        )
    )
    handle_updating_task = asyncio.create_task(
        handle_job_updates(db_conn_job_updater, results_cache_uri, jobs_poll_delay)
    )
    await asyncio.wait(
        [handle_pending_task, handle_updating_task], return_when=asyncio.FIRST_COMPLETED
    )


async def handle_reducer_connection(reader, writer):
    global reducer_connection_queue
    size_header_bytes = await reader.read(8)
    if not size_header_bytes:
        writer.close()
        await writer.wait_closed()
        return
    size_header = int.from_bytes(size_header_bytes, byteorder="little")

    message_bytes = await reader.read(size_header)
    if not message_bytes:
        writer.close()
        await writer.wait_closed()
        return
    message = msgpack.unpackb(message_bytes)

    reducer_recv_queue = asyncio.Queue(1)  # send messages from reducer to scheduler here
    reducer_send_queue = asyncio.Queue(1)  # listen here for messages from scheduler to reducer
    await reducer_connection_queue.put(
        (message["host"], message["port"], reducer_recv_queue, reducer_send_queue)
    )

    wait_for_ack = asyncio.create_task(reader.read(1))
    wait_for_job = asyncio.create_task(reducer_send_queue.get())
    done, pending = await asyncio.wait(
        [wait_for_ack, wait_for_job], return_when=asyncio.FIRST_COMPLETED
    )

    job_config = None
    if wait_for_job in done:
        job_config = wait_for_job.result()

    if wait_for_ack in done:
        await reducer_recv_queue.put(False)
        writer.close()
        await writer.wait_closed()
        return

    job_config_bytes = msgpack.packb(job_config.dict())
    size_header_bytes = (len(job_config_bytes)).to_bytes(8, byteorder="little")
    writer.write(size_header_bytes)
    writer.write(job_config_bytes)
    await writer.drain()

    ack = await wait_for_ack
    if not ack:
        await reducer_recv_queue.put(False)
        writer.close()
        await writer.wait_closed()
        return
    await reducer_recv_queue.put(True)

    wait_for_ack = asyncio.create_task(reader.read(1))
    wait_for_job_done = asyncio.create_task(reducer_send_queue.get())

    done, pending = await asyncio.wait(
        [wait_for_ack, wait_for_job_done], return_when=asyncio.FIRST_COMPLETED
    )

    if wait_for_job_done in done and False == wait_for_job_done.result():
        writer.close()
        await writer.wait_closed()
        return

    if wait_for_ack in done:
        await reducer_recv_queue.put(False)
        writer.close()
        await writer.wait_closed()
        return

    job_done_bytes = msgpack.packb({"done": True})
    size_header_bytes = (len(job_done_bytes)).to_bytes(8, byteorder="little")
    writer.write(size_header_bytes)
    writer.write(job_done_bytes)
    await writer.drain()

    ack = await wait_for_ack
    if not ack:
        await reducer_recv_queue.put(False)
    else:
        await reducer_recv_queue.put(True)
    writer.close()
    await writer.wait_closed()


async def main(argv: List[str]) -> int:
    global reducer_connection_queue
    args_parser = argparse.ArgumentParser(description="Wait for and run search jobs.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")

    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / "search_scheduler.log"
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

    logger.debug(f"Job polling interval {clp_config.search_scheduler.jobs_poll_delay} seconds.")
    try:
        server = await asyncio.start_server(
            handle_reducer_connection,
            clp_config.search_scheduler.host,
            clp_config.search_scheduler.port,
        )
        with contextlib.closing(
            sql_adapter.create_connection(True)
        ) as db_conn_job_fetcher, contextlib.closing(
            sql_adapter.create_connection(True)
        ) as db_conn_job_updater:
            logger.info(
                f"Connected to archive database"
                f" {clp_config.database.host}:{clp_config.database.port}."
            )
            server = server.serve_forever()
            logger.info("Search scheduler started.")
            batch_size = clp_config.search_scheduler.num_archives_to_search_per_sub_job
            job_handler = asyncio.create_task(
                handle_jobs(
                    db_conn_job_fetcher=db_conn_job_fetcher,
                    db_conn_job_updater=db_conn_job_updater,
                    results_cache_uri=clp_config.results_cache.get_uri(),
                    jobs_poll_delay=clp_config.search_scheduler.jobs_poll_delay,
                    num_archives_to_search_per_sub_job=batch_size,
                )
            )
            done, pending = await asyncio.wait(
                [server, job_handler], return_when=asyncio.FIRST_COMPLETED
            )
    except Exception:
        logger.exception(f"Uncaught exception in job handling loop.")

    return 0


if "__main__" == __name__:
    sys.exit(asyncio.run(main(sys.argv)))
