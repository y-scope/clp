#!/usr/bin/env python3

import argparse
import logging
import os
import socket
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional

import mysql.connector

import msgpack
import pathlib

#from celery.canvas import Signature
from celery import group, signature
from celery.exceptions import TimeoutError

from clp_py_utils.clp_config import SEARCH_JOBS_TABLE_NAME
from job_orchestration.executor.search.fs_search_method import (
    search as fs_search
)

from pydantic import ValidationError

from clp_py_utils.clp_config import CLPConfig, Database, ResultsCache
from clp_py_utils.clp_logging import get_logging_level
from clp_py_utils.core import read_yaml_config_file

from .common import JobStatus  # type: ignore
from .search_db_manager import DBManager, MongoDBManager

# Setup logging
# Create logger
logger = logging.getLogger("search-job-handler")
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)
# Prevents double logging from sub loggers for specific jobs
logger.propagate = False

class SearchJob:
    def __init__(self, task: any, reducer: bool) -> None:
        self.task: any = task
        self.reducer: bool = reducer

# dictionary of active jobs indexed by job id
active_jobs = {}

class SchedulerConfig:
    def __init__(self, host: str, port: int) -> None:
        self.host: str = host
        self.host_ip: str = socket.gethostbyname(host)
        self.port: int = port

def fetch_new_search_jobs(db_cursor) -> list:
    db_cursor.execute(f"""
        SELECT {SEARCH_JOBS_TABLE_NAME}.id as job_id,
        {SEARCH_JOBS_TABLE_NAME}.status as job_status,
        {SEARCH_JOBS_TABLE_NAME}.num_tasks,
        {SEARCH_JOBS_TABLE_NAME}.num_tasks_completed,
        {SEARCH_JOBS_TABLE_NAME}.search_config,
        {SEARCH_JOBS_TABLE_NAME}.creation_time,
        {SEARCH_JOBS_TABLE_NAME}.reducer_host,
        {SEARCH_JOBS_TABLE_NAME}.reducer_port
        FROM {SEARCH_JOBS_TABLE_NAME}
        WHERE {SEARCH_JOBS_TABLE_NAME}.status='{JobStatus.PENDING}'
        OR {SEARCH_JOBS_TABLE_NAME}.status='{JobStatus.REDUCER_READY}'
    """)
    return db_cursor.fetchall()

def fetch_cancelling_search_jobs(db_cursor) -> list:
    db_cursor.execute(f"""
        SELECT {SEARCH_JOBS_TABLE_NAME}.id as job_id,
        {SEARCH_JOBS_TABLE_NAME}.search_config
        FROM {SEARCH_JOBS_TABLE_NAME}
        WHERE {SEARCH_JOBS_TABLE_NAME}.status='{JobStatus.CANCELLING}'
    """)
    return db_cursor.fetchall()

def setup_search_jobs_table(db_conn):
    cursor = db_conn.cursor()
    cursor.execute(f"""
        CREATE TABLE IF NOT EXISTS `{SEARCH_JOBS_TABLE_NAME}` (
            `id` INT NOT NULL AUTO_INCREMENT,
            `status` INT NOT NULL DEFAULT '{JobStatus.PENDING}',
            `status_msg` VARCHAR(255) NOT NULL DEFAULT '',
            `creation_time` DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
            `start_time` DATETIME NULL DEFAULT NULL,
            `duration` INT NULL DEFAULT NULL,
            `num_tasks` INT NOT NULL DEFAULT '0',
            `num_tasks_completed` INT NOT NULL DEFAULT '0',
            `reducer_host` VARCHAR(32) NOT NULL DEFAULT '127.0.0.1',
            `reducer_port` INT NOT NULL DEFAULT '0',
            `search_config` VARBINARY(60000) NOT NULL,
            PRIMARY KEY (`id`) USING BTREE,
            INDEX `JOB_STATUS` (`status`) USING BTREE
        ) ROW_FORMAT=DYNAMIC
    """)
    db_conn.commit()
    cursor.close()

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

def set_job_status_and_update_search_config(
    db_conn, job_id: str, status: JobStatus, prev_status: JobStatus, search_config: dict
) -> bool:
    cursor = db_conn.cursor()

    update = f'UPDATE {SEARCH_JOBS_TABLE_NAME} SET status={status}, search_config=%s WHERE id={job_id} AND status={prev_status}'
    cursor.execute(update, (msgpack.packb(search_config),))

    db_conn.commit()

    rval = cursor.rowcount != 0
    cursor.close()
    return rval

def poll_and_handle_cancelling_search_jobs(db_conn, mongodb_manager) -> None:
    global active_jobs

    cursor = db_conn.cursor()
    cancelling_jobs = fetch_cancelling_search_jobs(cursor)
    db_conn.commit()
    cursor.close()

    for job_id, search_config in cancelling_jobs:
        if job_id in active_jobs:
            active_jobs[job_id].task.revoke(terminate=True)
            del active_jobs[job_id]
            logger.info(f"Cancelled job {job_id}")

        #TODO: also update time
        set_job_status(db_conn, job_id, JobStatus.CANCELLED, prev_status=JobStatus.CANCELLING)
        mongodb_manager.update_job_status(job_id, JobStatus.CANCELLED.to_str())

def get_search_tasks_for_job(
    db_conn,
    base_kwargs: Dict[str, Any],
    job_id: str,
    query: dict,
): #-> Group
    cursor = db_conn.cursor()
    #TODO: create more advanced queries
    cursor.execute(
        f'''SELECT id as archive_id
        FROM clp_archives
        ORDER BY end_timestamp DESC
        '''
    )
    task = group(
        fs_search.s(
            **base_kwargs,
            job_id_str=job_id,
            archive_id=archive_id,
            query=query,
        ) for archive_id, in cursor.fetchall()
    )
    db_conn.commit()
    cursor.close()
    return task

def dispatch_search_job(
    db_conn,
    base_kwargs: Dict[str, Any],
    job_id: int,
    query: dict,
    aggregation_job: bool,
) -> None:
    global active_jobs
    task_group = get_search_tasks_for_job(db_conn, base_kwargs, str(job_id), query)
    active_jobs[job_id] = SearchJob(task_group.apply_async(), aggregation_job)


def parse_and_modify_query(query_info: dict):
    split_query = [part.strip() for part in query_info["pipeline_string"].split("|")]

    if len(split_query) != 2:
        return False
    
    if split_query[1] == "count":
        query_info["pipeline_string"] = split_query[0]
        query_info["count"] = True
        return True
    
    return False

def is_reducer_job(query_info: dict) -> bool:
    if "count" in query_info:
        return True, False

    modified = parse_and_modify_query(query_info)
    return modified, modified
    

def poll_and_submit_pending_search_jobs(db_conn, mongodb_manager, base_kwargs) -> None:
    global active_jobs

    cursor = db_conn.cursor()
    new_jobs = fetch_new_search_jobs(cursor)
    db_conn.commit()
    cursor.close()

    job_detection_ts = time.time()
    # FIXME: add extra instrumentation to account for reducer job acquisition time
    jobs_list = []

    for job_id, job_status, num_tasks, num_tasks_completed, search_config, creation_time, reducer_host, reducer_port in new_jobs:
        query_info = msgpack.unpackb(search_config)

        aggregation_job, modified = is_reducer_job(query_info)
        if job_status == JobStatus.PENDING and aggregation_job:
            logger.info(f"Requesting reducer for job {job_id}")
            if modified:
                set_job_status_and_update_search_config(db_conn, job_id, JobStatus.PENDING_REDUCER, JobStatus.PENDING, query_info)
            else:
                set_job_status(db_conn, job_id, JobStatus.PENDING_REDUCER, JobStatus.PENDING)
            continue
        elif job_status == JobStatus.REDUCER_READY:
            query_info["reducer_host"] = reducer_host
            query_info["reducer_port"] = reducer_port
            logger.info(f"Got reducer for job {job_id} at {reducer_host}:{reducer_port}")
        
        job_status_str = JobStatus.RUNNING.to_str()
        enqueued_ts = time.time()
        dispatch_search_job(db_conn, base_kwargs, job_id, query_info, aggregation_job)
        if set_job_status(db_conn, job_id, JobStatus.RUNNING, job_status):
            logger.info(f"Submitted job {job_id}")
        else:
            logger.error(f"Failed to submit job {job_id}... cancelling")
            active_jobs[job_id].task.revoke()
            job_status_str = JobStatus.CANCELLED.to_str()
            set_job_status(db_conn, job_id, JobStatus.CANCELLED)

        jobs_list.append(
            mongodb_manager.prepare_job_document(
                job_id,
                creation_time.timestamp(),
                job_detection_ts,
                enqueued_ts,
                job_status_str,
            )
        )
    if len(jobs_list) != 0:
        mongodb_manager.insert_jobs(jobs_list)

'''
Polling doesn't appear to work correctly with the amqp backend --
results never show up until get() is called. To circumvent this we call
get with a short timeout here to see if results happen to be ready instead
of polling ready() like we should be able to.

https://github.com/celery/celery/issues/4084
^Issue open since 2017 showing that this is a bug
'''
def get_results_or_timeout(result):
    try:
        return result.get(timeout=0.01)
    except TimeoutError:
        return None

def check_job_status_and_update_db(db_conn, mongodb_manager):
    global active_jobs

    for job_id in list(active_jobs.keys()):
        result = active_jobs[job_id].task
        reducer = active_jobs[job_id].reducer
        returned_results = get_results_or_timeout(result)
        if returned_results is not None:
            # Initialize tracker variables
            completion_ts = time.time()
            results_count = 0
            all_task_succeed = True
            tasks_list = []
            job_status = JobStatus.SUCCESS
            if reducer:
                job_status = JobStatus.PENDING_REDUCER_DONE

            for task_result in returned_results:
                results_count += task_result["num_matches"]
                tasks_list.append(task_result)
                if not task_result['status']:
                    task_id = task_result['task_id']
                    all_task_succeed = False
                    job_status = JobStatus.FAILED
                    logger.warning(f"task {task_id} failed")
            # clean up
            del active_jobs[job_id]

            if all_task_succeed:
                logger.info(f"Completed search phase for job {job_id}")
            else:
                logger.info(f"job {job_id} finished with failures")

            set_job_status(db_conn, job_id, job_status)
            job_stats = {
                "results_count": results_count,
                "status": job_status.to_str()
            }
            # I am hoping those two won't take too long
            mongodb_manager.finalize_job_stats(job_id, completion_ts, job_stats)
            mongodb_manager.insert_tasks(tasks_list)

def handle_jobs(
    db_conn,
    mongodb_manager,
    celery_worker_method_base_kwargs: Dict[str, Any],
    jobs_poll_delay: float,
) -> None:
    # Changes the os directory for job specific logs
    while True:
        poll_and_submit_pending_search_jobs(db_conn, mongodb_manager, celery_worker_method_base_kwargs)
        poll_and_handle_cancelling_search_jobs(db_conn, mongodb_manager)
        check_job_status_and_update_db(db_conn, mongodb_manager)
        time.sleep(jobs_poll_delay)

    logger.info("Exiting...")


def main(argv: List[str]) -> int:
    # fmt: off
    args_parser = argparse.ArgumentParser(description="Wait for and run search jobs.")
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')

    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / "search_scheduler.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(logging_formatter)
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    logging_level_str = os.getenv("CLP_LOGGING_LEVEL")
    logging_level = get_logging_level(logging_level_str)
    logger.setLevel(logging_level)
    logger.info(f"Start logging level = {logging.getLevelName(logging_level)}")

    # Load configuration
    config_path = pathlib.Path(parsed_args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
        return -1
    except Exception as ex:
        logger.error(ex)
        # read_yaml_config_file already logs the parsing error inside
        return -1

    output_config = dict(clp_config.results_cache)

    celery_worker_method_base_kwargs: Dict[str, Any] = {
        "output_config": output_config
    }

    db_conn = mysql.connector.connect(
        host=clp_config.database.host,
        port=clp_config.database.port,
        database=clp_config.database.name,
        user=clp_config.database.username,
        password=clp_config.database.password,
    )

    setup_search_jobs_table(db_conn)
    mongodb_manager: DBManager = MongoDBManager(logger, clp_config.results_cache.get_uri())

    jobs_poll_delay = clp_config.search_scheduler.jobs_poll_delay
    logger.info("search-job-handler started.")
    logger.debug(f"job-poll-delay = {jobs_poll_delay} seconds")
    handle_jobs(
        db_conn=db_conn,
        mongodb_manager=mongodb_manager,
        celery_worker_method_base_kwargs=celery_worker_method_base_kwargs,
        jobs_poll_delay=jobs_poll_delay
    )
    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
