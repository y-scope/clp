import logging
import os
import sys
import signal
import queue
import shutil
import struct
import subprocess
import threading
import time
from pathlib import Path
from typing import Any, Dict

from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_logging import get_logging_level

from job_orchestration.executor.search.celery import app
from job_orchestration.executor.utils import get_profiled_memory_usage

script_dir = Path(__file__).parent.resolve()


def time_helper(prev_t):
    temp_t = time.time()
    measured_t = temp_t - prev_t
    prev_t = temp_t
    return measured_t, prev_t


def is_reducer_job(query_info: dict) -> bool:
    return 'reducer_host' in query_info and 'reducer_port' in query_info


@app.task(bind=True)
def search(
        self: Task,
        job_id_str: str,
        output_config: Dict[str, Any],  # used to indicate how to output the results
        archive_id: str,
        query: dict,
) -> Dict[str, Any]:
    prev_t = time.time()
    task_start_t = prev_t
    task_id_str = self.request.id

    logger = get_task_logger(__name__)
    logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")

    # Setup logging to file
    worker_logs_dir = Path(os.getenv("CLP_LOGS_DIR")) / job_id_str
    worker_logs_dir.mkdir(exist_ok=True, parents=True)
    worker_logs = worker_logs_dir / f"{task_id_str}.log"
    logging_file_handler = logging.FileHandler(filename=worker_logs, encoding="utf-8")
    logging_file_handler.setFormatter(logging_formatter)
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    logging_level_str = os.getenv("CLP_LOGGING_LEVEL")
    logging_level = get_logging_level(logging_level_str)
    logger.setLevel(logging_level)

    logger.info(f"Started job {job_id_str}. Task Id={task_id_str}.")

    # Open stderr log file
    stderr_log_path = worker_logs_dir / f"{task_id_str}-stderr.log"
    stderr_log_file = open(stderr_log_path, "w")

    # Obtain archive directory info
    archive_directory = Path(os.getenv('CLP_ARCHIVE_OUTPUT_DIR'))
    logger.debug(f"{archive_directory}")

    # FIXME: consider removing startup time now that we aren't using
    # proxy server
    startup_time, prev_t = time_helper(prev_t)

    # this is an ugly way to deal with the fact that sometimes we
    # hit the reducer and sometimes mongodb
    ip = "localhost"
    port = "0"

    if is_reducer_job(query):
        ip = query['reducer_host']
        port = str(query['reducer_port'])

    proxy_time, prev_t = time_helper(prev_t)
    # Assemble the search command
    # fmt: off
    clp_home = Path(os.getenv("CLP_HOME"))
    search_cmd = [
        str(clp_home / "bin" / "obs"),
        str(clp_home / "bin" / "clo"),
        ip,
        port,
        str(archive_directory / archive_id),
        query["pipeline_string"],
        '-c',
        '--tge', str(query['timestamp_begin']),
        '--tle', str(query['timestamp_end']),
        '--job-id', job_id_str,
    ]
    if 'count' in query:
        search_cmd.append('--count')
    if query['path_regex']:
        search_cmd.append(query['path_regex'])
    if not query['match_case']:
        search_cmd.append('-i')

    if not is_reducer_job(query):
        mongodb_uri = f"mongodb://{output_config['host']}:{output_config['port']}/"
        search_cmd.append("--mongodb-uri")
        search_cmd.append(str(mongodb_uri))
        search_cmd.append("--mongodb-database")
        search_cmd.append(str(output_config["db_name"]))
        search_cmd.append("--mongodb-collection")
        search_cmd.append(str(query['results_collection_name']))

    # Start compression
    logger.info("Searching...")
    logger.info(" ".join(search_cmd))

    search_successful = False

    search_proc = subprocess.Popen(
        search_cmd,
        preexec_fn=os.setpgrp,
        close_fds=True,
        stdout=subprocess.PIPE,
        stderr=stderr_log_file,
    )

    def sigterm_handler(_signo, _stack_frame):
        if search_proc.poll() is None:
            logger.debug("try to kill search process")
            # kill with group id to kill both obs and clo
            os.killpg(os.getpgid(search_proc.pid), signal.SIGTERM)
            os.waitpid(search_proc.pid, 0)
            logger.info(f"Cancelling search task: {task_id_str}")
        else:
            logger.debug("Search process is not running")
        logger.debug(f"Exiting with error code {_signo + 128}")
        sys.exit(_signo + 128)

    # Register the function to kill the child process at exit
    signal.signal(signal.SIGTERM, sigterm_handler)

    # Wait for compression to finish
    return_code = search_proc.wait()
    search_time, prev_t = time_helper(prev_t)
    num_matches: int = 0
    if 0 != return_code:
        logger.error(f"Failed to search, return_code={return_code}")
    else:
        search_successful = True
        parts = search_proc.stdout.readline().decode().split(":")
        num_matches = int(parts[1].strip())

    peak_memory_in_kbytes = get_profiled_memory_usage("profiling.out", logger)

    # FIXME: consider removing flush_time since we no longer have to flush
    # from proxy server
    flush_time, prev_t = time_helper(prev_t)

    logger.info("Search completed")
    # Close log files
    stderr_log_file.close()
    logger.removeHandler(logging_file_handler)
    logging_file_handler.close()
    teardown_time, task_end_t = time_helper(prev_t)

    results = {
        'status': search_successful,
        'job_id': job_id_str,
        'task_id': task_id_str,
        'peak_memory_kbytes': peak_memory_in_kbytes,
        'num_matches': num_matches,
        'task_start_ts': task_start_t,
        'start_up': startup_time,
        'proxy_start': proxy_time,
        'clg': search_time,
        'flush': flush_time,
        'teardown': teardown_time,
        'task_end_to_end': task_end_t - task_start_t
    }

    return results
