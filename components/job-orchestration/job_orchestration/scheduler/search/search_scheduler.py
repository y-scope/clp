#!/usr/bin/env python3

import argparse
import contextlib
import logging
import os
import pathlib
import sys
import time
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
from job_orchestration.scheduler.scheduler_data import SearchJob, SearchSubJob, SearchTaskResult
from pydantic import ValidationError

# Setup logging
logger = get_logger("search-job-handler")

# Dictionary of active jobs indexed by job id
active_jobs: Dict[str, SearchJob] = {}


def cancel_job(job_id):
    global active_jobs
    active_jobs[job_id].current_sub_job.async_task_result.revoke(terminate=True)
    try:
        active_jobs[job_id].current_sub_job.async_task_result.get()
    except Exception:
        pass
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


def handle_cancelling_search_jobs(db_conn) -> None:
    global active_jobs

    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
        cancelling_jobs = fetch_cancelling_search_jobs(cursor)
        db_conn.commit()

    for job in cancelling_jobs:
        job_id = job["job_id"]
        if job_id in active_jobs:
            cancel_job(job_id)
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
    if len(filter_clauses) > 0:
        query += " WHERE " + " AND ".join(filter_clauses)
    query += " ORDER BY end_timestamp DESC"

    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
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
    archives_for_search: List[Dict[str, any]],
    job_id: str,
    search_config: SearchConfig,
    results_cache_uri: str,
) -> None:
    global active_jobs
    task_group = get_task_group_for_job(
        archives_for_search, job_id, search_config, results_cache_uri
    )
    active_jobs[job_id].current_sub_job = SearchSubJob(async_task_result=task_group.apply_async())


def handle_pending_search_jobs(
    db_conn, results_cache_uri: str, num_archives_to_search_per_batch: int
) -> None:
    global active_jobs
    pending_job_ids = [
        job_id for job_id in active_jobs if active_jobs[job_id].waiting_for_next_sub_job
    ]
    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
        for job in fetch_new_search_jobs(cursor):
            job_id = str(job["job_id"])
            search_config = SearchConfig.parse_obj(msgpack.unpackb(job["search_config"]))
            archives_for_search = get_archives_for_search(db_conn, search_config)
            if len(archives_for_search) == 0:
                if set_job_status(
                    db_conn,
                    job_id,
                    SearchJobStatus.NO_MATCHING_ARCHIVES,
                ):
                    logger.info(f"No matching archives, skipping job {job['job_id']}.")
                continue
            new_search_job = SearchJob(
                id=job_id,
                search_config=search_config,
                waiting_for_next_sub_job=True,
                remaining_archives_for_search=get_archives_for_search(db_conn, search_config),
                current_sub_job=None,
            )
            active_jobs[job_id] = new_search_job
            pending_job_ids.append(job_id)
        db_conn.commit()

    for job_id in pending_job_ids:
        job = active_jobs[job_id]

        if len(job.remaining_archives_for_search) > num_archives_to_search_per_batch:
            archives_for_search = job.remaining_archives_for_search[
                :num_archives_to_search_per_batch
            ]
            job.remaining_archives_for_search = job.remaining_archives_for_search[
                num_archives_to_search_per_batch:
            ]
        else:
            archives_for_search = job.remaining_archives_for_search
            job.remaining_archives_for_search = []

        dispatch_search_job(archives_for_search, job_id, job.search_config, results_cache_uri)
        if set_job_status(db_conn, job_id, SearchJobStatus.RUNNING):
            logger.info(
                f"Dispatched job {job_id} with {len(archives_for_search)} archives to search."
            )
        job.waiting_for_next_sub_job = False


def try_getting_task_result(async_task_result):
    if not async_task_result.ready():
        return None
    return async_task_result.get()


def check_job_status_and_update_db(db_conn, results_cache_uri):
    global active_jobs

    for job_id in list(active_jobs.keys()):
        job = active_jobs[job_id]
        try:
            returned_results = try_getting_task_result(job.current_sub_job.async_task_result)
        except Exception as e:
            logger.error(f"Job `{job_id}` failed: {e}.")
            # clean up
            del active_jobs[job_id]
            set_job_status(db_conn, job_id, SearchJobStatus.FAILED, SearchJobStatus.RUNNING)
            continue

        if returned_results is not None:
            new_job_status = SearchJobStatus.RUNNING
            for task_result_obj in returned_results:
                task_result = SearchTaskResult.parse_obj(task_result_obj)
                if not task_result.success:
                    task_id = task_result.task_id
                    new_job_status = SearchJobStatus.FAILED
                    logger.debug(f"Task {task_id} failed - result {task_result}.")

            if new_job_status != SearchJobStatus.FAILED:
                max_num_results = job.search_config.max_num_results
                # check if we have searched all archives
                if len(job.remaining_archives_for_search) == 0:
                    new_job_status = SearchJobStatus.SUCCEEDED
                # check if we have reached max results
                elif max_num_results > 0:
                    results_cache_client = pymongo.MongoClient(results_cache_uri)
                    results_cache_collection = results_cache_client.get_default_database()[job_id]
                    results_count = results_cache_collection.count_documents({})
                    if results_count >= max_num_results:
                        result = list(
                            results_cache_collection.find()
                            .sort("timestamp", -1)
                            .limit(max_num_results)
                        )
                        if result is not None:
                            min_timestamp_in_top_results = result[-1]["timestamp"]
                        else:
                            min_timestamp_in_top_results = 0
                        results_cache_client.close()
                        max_timestamp_in_archive = job.remaining_archives_for_search[0][
                            "end_timestamp"
                        ]
                        if max_timestamp_in_archive <= min_timestamp_in_top_results:
                            new_job_status = SearchJobStatus.SUCCEEDED

            if new_job_status == SearchJobStatus.RUNNING:
                job.waiting_for_next_sub_job = True
                job.current_sub_job = None
                logger.info(f"Job {job_id} waiting for more archives to search.")
            else:
                if set_job_status(db_conn, job_id, new_job_status, SearchJobStatus.RUNNING):
                    if new_job_status == SearchJobStatus.SUCCEEDED:
                        logger.info(f"Completed job {job_id}.")
                    else:
                        logger.info(f"Completed job {job_id} with failing tasks.")
                del active_jobs[job_id]


def handle_jobs(
    db_conn,
    results_cache_uri: str,
    jobs_poll_delay: float,
    num_archives_to_search_per_batch: int,
) -> None:
    while True:
        handle_pending_search_jobs(db_conn, results_cache_uri, num_archives_to_search_per_batch)
        handle_cancelling_search_jobs(db_conn)
        check_job_status_and_update_db(db_conn, results_cache_uri)
        time.sleep(jobs_poll_delay)


def main(argv: List[str]) -> int:
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

    sql_adapter = SQL_Adapter(clp_config.database)

    logger.debug(f"Job polling interval {clp_config.search_scheduler.jobs_poll_delay} seconds.")
    try:
        with contextlib.closing(sql_adapter.create_connection(True)) as db_conn:
            logger.info(
                f"Connected to archive database"
                f" {clp_config.database.host}:{clp_config.database.port}."
            )
            logger.info("Search scheduler started.")
            handle_jobs(
                db_conn=db_conn,
                results_cache_uri=clp_config.results_cache.get_uri(),
                jobs_poll_delay=clp_config.search_scheduler.jobs_poll_delay,
                num_archives_to_search_per_batch=clp_config.search_scheduler.num_archives_to_search_per_batch,
            )
    except Exception:
        logger.exception(f"Uncaught exception in job handling loop.")

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
