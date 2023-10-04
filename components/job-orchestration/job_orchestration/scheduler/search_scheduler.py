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
from clp_py_utils.core import read_yaml_config_file

from .common import JobStatus  # type: ignore

import pymongo

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
        {SEARCH_JOBS_TABLE_NAME}.search_config
        FROM {SEARCH_JOBS_TABLE_NAME}
        WHERE {SEARCH_JOBS_TABLE_NAME}.status='{JobStatus.PENDING}'
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
            `creation_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
            `start_time` DATETIME NULL DEFAULT NULL,
            `duration` INT NULL DEFAULT NULL,
            `num_tasks` INT NOT NULL DEFAULT '0',
            `num_tasks_completed` INT NOT NULL DEFAULT '0',
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


def poll_and_handle_cancelling_search_jobs(db_conn) -> None:
    global active_jobs

    cursor = db_conn.cursor()
    cancelling_jobs = fetch_cancelling_search_jobs(cursor)
    db_conn.commit()
    cursor.close()

    for job_id, search_config in cancelling_jobs:
        if job_id in active_jobs:
            active_jobs[job_id].revoke(terminate=True)
            del active_jobs[job_id]
            logger.info(f"Cancelled job {job_id}")

        #TODO: also update time
        set_job_status(db_conn, job_id, JobStatus.CANCELLED, prev_status=JobStatus.CANCELLING)

def get_search_tasks_for_job(
    db_conn,
    base_kwargs: Dict[str, Any],
    job_id: str,
    query: str,
): #-> Group
    cursor = db_conn.cursor()
    #TODO: create more advanced queries
    cursor.execute(
        f'''SELECT id as archive_id
        FROM clp_archives
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
    query: str,
) -> None:
    global active_jobs
    task_group = get_search_tasks_for_job(db_conn, base_kwargs, str(job_id), query)
    active_jobs[job_id] = task_group.apply_async()

def poll_and_submit_pending_search_jobs(db_conn, base_kwargs) -> None:
    global active_jobs

    cursor = db_conn.cursor()
    new_jobs = fetch_new_search_jobs(cursor)
    db_conn.commit()
    cursor.close()

    for job_id, job_status, num_tasks, num_tasks_completed, search_config in new_jobs:
        dispatch_search_job(db_conn, base_kwargs, job_id, msgpack.unpackb(search_config))
        if set_job_status(db_conn, job_id, JobStatus.RUNNING, JobStatus.PENDING):
            logger.info(f"Submitted job {job_id}")
        else:
            logger.error(f"Failed to submit job {job_id}... cancelling")
            active_jobs[job_id].revoke()
            set_job_status(db_conn, job_id, JobStatus.CANCELLED)

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

def check_job_status_and_update_db(db_conn):
    global active_jobs

    for job_id in list(active_jobs.keys()):
        result = active_jobs[job_id]
        returned_results = get_results_or_timeout(result)

        if returned_results != None:
            logger.info(f"Completed job {job_id}")
            del active_jobs[job_id]
            #FIXME: check for errors before writing SUCCESS
            set_job_status(db_conn, job_id, JobStatus.SUCCESS)

def handle_jobs(
    db_conn,
    celery_worker_method_base_kwargs: Dict[str, Any],
) -> None:
    # Changes the os directory for job specific logs
    while True:
        poll_and_submit_pending_search_jobs(db_conn, celery_worker_method_base_kwargs)
        poll_and_handle_cancelling_search_jobs(db_conn)
        check_job_status_and_update_db(db_conn)
        time.sleep(0.1)

    logger.info("Exiting...")


def main(argv: List[str]) -> int:
    # fmt: off
    args_parser = argparse.ArgumentParser(description="Wait for and run compression jobs.")
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')

    parsed_args = args_parser.parse_args(argv[1:])
    # Load configuration
    config_path = pathlib.Path(parsed_args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
    except Exception as ex:
        logger.error(ex)
        # read_yaml_config_file already logs the parsing error inside
        pass
    else:
        fs_input_config = {}
        output_config = dict(clp_config.results_cache)

        celery_worker_method_base_kwargs: Dict[str, Any] = {
            "fs_input_config": fs_input_config,
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

        logger.info("search-job-handler started.")

        handle_jobs(
            db_conn=db_conn,
            celery_worker_method_base_kwargs=celery_worker_method_base_kwargs,
        )
    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
