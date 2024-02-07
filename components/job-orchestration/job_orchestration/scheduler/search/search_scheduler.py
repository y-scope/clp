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


def cancel_job(job_id):
    global active_jobs
    active_jobs[job_id].async_task_result.revoke(terminate=True)
    try:
        active_jobs[job_id].async_task_result.get()
    except Exception:
        pass
    del active_jobs[job_id]


def fetch_new_search_jobs(db_cursor) -> list:
    db_cursor.execute(
        f"""
        SELECT {SEARCH_JOBS_TABLE_NAME}.id as job_id,
        {SEARCH_JOBS_TABLE_NAME}.status as job_status,
        {SEARCH_JOBS_TABLE_NAME}.search_config,
        {SEARCH_JOBS_TABLE_NAME}.submission_time
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
    query = f"""SELECT id as archive_id
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
        archives_for_search = [archive["archive_id"] for archive in cursor.fetchall()]
        db_conn.commit()
    return archives_for_search


def get_task_group_for_job(
    archives_for_search: List[str],
    job_id: str,
    search_config: SearchConfig,
    results_cache_uri: str,
):
    search_config_obj = search_config.dict()
    return celery.group(
        search.s(
            job_id=job_id,
            archive_id=archive_id,
            search_config_obj=search_config_obj,
            results_cache_uri=results_cache_uri,
        )
        for archive_id in archives_for_search
    )


def dispatch_search_job(
    archives_for_search: List[str], job_id: str, search_config: SearchConfig, results_cache_uri: str
) -> None:
    global active_jobs
    task_group = get_task_group_for_job(
        archives_for_search, job_id, search_config, results_cache_uri
    )
    active_jobs[job_id] = SearchJob(task_group.apply_async())


def handle_pending_search_jobs(db_conn, results_cache_uri: str) -> None:
    global active_jobs

    with contextlib.closing(db_conn.cursor(dictionary=True)) as cursor:
        new_jobs = fetch_new_search_jobs(cursor)
        db_conn.commit()

    for job in new_jobs:
        logger.debug(f"Got job {job['job_id']} with status {job['job_status']}.")
        search_config_obj = SearchConfig.parse_obj(msgpack.unpackb(job["search_config"]))
        archives_for_search = get_archives_for_search(db_conn, search_config_obj)
        if len(archives_for_search) == 0:
            if set_job_status(db_conn, job["job_id"], SearchJobStatus.SUCCEEDED, job["job_status"]):
                logger.info(f"No matching archives, skipping job {job['job_id']}.")
            continue

        dispatch_search_job(
            archives_for_search, str(job["job_id"]), search_config_obj, results_cache_uri
        )
        if set_job_status(db_conn, job["job_id"], SearchJobStatus.RUNNING, job["job_status"]):
            logger.info(
                f"Dispatched job {job['job_id']} with {len(archives_for_search)} archives to"
                f" search."
            )


def try_getting_task_result(async_task_result):
    if not async_task_result.ready():
        return None
    return async_task_result.get()


def check_job_status_and_update_db(db_conn):
    global active_jobs

    for job_id in list(active_jobs.keys()):
        try:
            returned_results = try_getting_task_result(active_jobs[job_id].async_task_result)
        except Exception as e:
            logger.error(f"Job `{job_id}` failed: {e}.")
            # clean up
            del active_jobs[job_id]
            set_job_status(db_conn, job_id, SearchJobStatus.FAILED, SearchJobStatus.RUNNING)
            continue

        if returned_results is not None:
            new_job_status = SearchJobStatus.SUCCEEDED
            for task_result_obj in returned_results:
                task_result = SearchTaskResult.parse_obj(task_result_obj)
                if not task_result.success:
                    task_id = task_result.task_id
                    new_job_status = SearchJobStatus.FAILED
                    logger.debug(f"Task {task_id} failed - result {task_result}.")

            del active_jobs[job_id]

            if set_job_status(db_conn, job_id, new_job_status, SearchJobStatus.RUNNING):
                if new_job_status != SearchJobStatus.FAILED:
                    logger.info(f"Completed job {job_id}.")
                else:
                    logger.info(f"Completed job {job_id} with failing tasks.")


def handle_jobs(
    db_conn,
    results_cache_uri: str,
    jobs_poll_delay: float,
) -> None:
    while True:
        handle_pending_search_jobs(db_conn, results_cache_uri)
        handle_cancelling_search_jobs(db_conn)
        check_job_status_and_update_db(db_conn)
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
            )
    except Exception:
        logger.exception(f"Uncaught exception in job handling loop.")

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
