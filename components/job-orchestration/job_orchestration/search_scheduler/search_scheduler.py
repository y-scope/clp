#!/usr/bin/env python3

import argparse
import logging
import os
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional

import mysql.connector

import msgpack
import pathlib

from celery import group

from clp_py_utils.clp_config import SEARCH_JOBS_TABLE_NAME, CLP_METADATA_TABLE_PREFIX
from job_orchestration.executor.search.fs_search_task import search
from job_orchestration.job_config import SearchConfig

from pydantic import ValidationError

from clp_py_utils.clp_config import CLPConfig
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.core import read_yaml_config_file

from .common import JobStatus  # type: ignore

# Setup logging
logger = get_logger("search-job-handler")

class SearchJob:
    def __init__(self, async_task: any) -> None:
        self.async_task: any = async_task

# dictionary of active jobs indexed by job id
active_jobs : Dict[str, SearchJob] = {}

def cancel_job(job_id):
    global active_jobs
    active_jobs[job_id].async_task.revoke(terminate=True)
    try:
        active_jobs[job_id].async_task.get()
    except:
        pass
    del active_jobs[job_id]


def fetch_new_search_jobs(db_cursor) -> list:
    db_cursor.execute(f"""
        SELECT {SEARCH_JOBS_TABLE_NAME}.id as job_id,
        {SEARCH_JOBS_TABLE_NAME}.status as job_status,
        {SEARCH_JOBS_TABLE_NAME}.search_config,
        {SEARCH_JOBS_TABLE_NAME}.submission_time,
        FROM {SEARCH_JOBS_TABLE_NAME}
        WHERE {SEARCH_JOBS_TABLE_NAME}.status='{JobStatus.PENDING}'
    """)
    return db_cursor.fetchall()


def fetch_cancelling_search_jobs(db_cursor) -> list:
    db_cursor.execute(f"""
        SELECT {SEARCH_JOBS_TABLE_NAME}.id as job_id
        FROM {SEARCH_JOBS_TABLE_NAME}
        WHERE {SEARCH_JOBS_TABLE_NAME}.status='{JobStatus.CANCELLING}'
    """)
    return db_cursor.fetchall()


def set_job_status(
    db_conn, job_id: str, status: JobStatus, prev_status: Optional[JobStatus] = None, **kwargs
) -> bool:
    cursor = db_conn.cursor()

    field_set_expressions = [f'{k}="{v}"' for k, v in kwargs.items()]
    field_set_expressions.append(f"status={status}")
    update = f'UPDATE {SEARCH_JOBS_TABLE_NAME} SET {", ".join(field_set_expressions)} WHERE id={job_id}'

    if prev_status is not None:
        update += f' AND status={prev_status}'
    cursor.execute(update)

    db_conn.commit()

    rval = cursor.rowcount != 0
    cursor.close()
    return rval


def poll_and_handle_cancelling_search_jobs(db_conn) -> None:
    global active_jobs

    cursor = db_conn.cursor()
    cancelling_jobs = fetch_cancelling_search_jobs(cursor)
    db_conn.commit()
    cursor.close()

    for job_id, in cancelling_jobs:
        if job_id in active_jobs:
            cancel_job(job_id)
        logger.info(f"Cancelled job {job_id}.")
        set_job_status(db_conn, job_id, JobStatus.CANCELLED, prev_status=JobStatus.CANCELLING)


def get_archives_for_search(
    db_conn,
    search_config: SearchConfig,
):
    cursor = db_conn.cursor()
    cursor.execute(
        f'''SELECT id as archive_id
        FROM {CLP_METADATA_TABLE_PREFIX}archives where begin_timestamp <= {search_config.end_timestamp} and end_timestamp >= {search_config.begin_timestamp}
        ORDER BY end_timestamp DESC
        '''
    )
    archives_for_search = [(archive_id) for archive_id, in cursor.fetchall()]
    db_conn.commit()
    cursor.close()
    return archives_for_search


def get_search_tasks_for_job(
    archives_for_search: List[tuple],
    job_id: str,
    search_config: SearchConfig,
    results_cache_uri: str,
):
    return group(
        search.s(
            job_id=job_id,
            archive_id=archive_id,
            search_config=search_config,
            results_cache_uri=results_cache_uri,
        ) for archive_id, in archives_for_search
    )


def dispatch_search_job(
    archives_for_search: List[tuple],
    job_id: str,
    search_config: SearchConfig,
    results_cache_uri: str
) -> None:
    global active_jobs
    task_group = get_search_tasks_for_job(archives_for_search, job_id, search_config, results_cache_uri)
    active_jobs[job_id] = SearchJob(task_group.apply_async())


def poll_and_submit_pending_search_jobs(db_conn, results_cache_uri: str) -> None:
    global active_jobs

    cursor = db_conn.cursor()
    new_jobs = fetch_new_search_jobs(cursor)
    db_conn.commit()
    cursor.close()

    for job_id, job_status, search_config, submission_time in new_jobs:
        logger.debug(f"Got job {job_id} with status {job_status}.")
        search_config_obj = SearchConfig.parse_obj(msgpack.unpackb(search_config))
        archives_for_search = get_archives_for_search(db_conn, search_config_obj)
        if len(archives_for_search) == 0:
            if set_job_status(db_conn, job_id, JobStatus.SUCCESS, JobStatus.PENDING):
                logger.info(f"No matching archives, skipping job {job_id}.")
            else:
                logger.info(f"Marking job {job_id} with no archives to search cancelled.")
                set_job_status(db_conn, job_id, JobStatus.CANCELLED)
            continue

        dispatch_search_job(archives_for_search, str(job_id), search_config_obj, results_cache_uri)
        if set_job_status(db_conn, job_id, JobStatus.RUNNING, job_status):
            logger.info(f"Dispatched job {job_id} with {len(archives_for_search)} archives to search.")
        else:
            logger.error(f"Marking job {job_id} with {len(archives_for_search)} archives to search cancelled.")
            cancel_job(job_id)
            set_job_status(db_conn, job_id, JobStatus.CANCELLED)


def try_getting_results(result):
    if not result.ready():
        return None
    return result.get()


def check_job_status_and_update_db(db_conn):
    global active_jobs

    for job_id in list(active_jobs.keys()):
        result = active_jobs[job_id].task
        try:
            returned_results = try_getting_results(result)
        except Exception as e:
            logger.error(f"Job `{job_id}` failed: {e}.")
            # clean up
            del active_jobs[job_id]
            set_job_status(db_conn, job_id, JobStatus.FAILED)
            continue

        if returned_results is not None:
            new_job_status = JobStatus.SUCCESS
            for task_result in returned_results:
                if not task_result['status']:
                    task_id = task_result['task_id']
                    new_job_status = JobStatus.FAILED
                    logger.debug(f"Task {task_id} failed - result {task_result}.")

            del active_jobs[job_id]

            # TODO: setting SUCCESS/FAILED unconditionally here violates the state diagram as
            # written.
            set_job_status(db_conn, job_id, new_job_status)
            if new_job_status != JobStatus.FAILED:
                logger.info(f"Completed job {job_id}.")
            else:
                logger.info(f"Completed job {job_id} with failing tasks.")


def handle_jobs(
    db_conn,
    results_cache_uri: str,
    jobs_poll_delay: float,
) -> None:
    while True:
        poll_and_submit_pending_search_jobs(db_conn, results_cache_uri)
        poll_and_handle_cancelling_search_jobs(db_conn)
        check_job_status_and_update_db(db_conn)
        time.sleep(jobs_poll_delay)


def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(description="Wait for and run search jobs.")
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')

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

    db_conn = mysql.connector.connect(
        host=clp_config.database.host,
        port=clp_config.database.port,
        database=clp_config.database.name,
        user=clp_config.database.username,
        password=clp_config.database.password,
    )

    logger.info(f"Connected to archive database {clp_config.database.host}:{clp_config.database.port}.")
    logger.info("Search scheduler started.")
    logger.debug(f"Polling interval {jobs_poll_delay} seconds.")
    handle_jobs(
        db_conn=db_conn,
        results_cache_uri=clp_config.results_cache.get_uri(),
        jobs_poll_delay=clp_config.search_scheduler.jobs_poll_delay,
    )
    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
