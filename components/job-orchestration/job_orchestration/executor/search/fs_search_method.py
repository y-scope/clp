import logging
import os
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

script_dir = Path(__file__).parent.resolve()

def time_helper(prev_t):
    temp_t = time.time()
    measured_t = temp_t - prev_t
    prev_t = temp_t
    return measured_t, prev_t

@app.task(bind=True)
def search(
    self: Task,
    job_id_str: str,
    output_config: Dict[str, Any], # used to indicate how to output the results
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

    startup_time, prev_t = time_helper(prev_t)
    server_cmd = [
        "python3",
        str(script_dir / "proxy_server.py"),
        output_config["host"],
        str(output_config["port"]),
        output_config["db_name"],
        output_config["results_collection_name"] + "_" + query['sessionId'],
    ]
    server_proc = subprocess.Popen(
        server_cmd,
        close_fds=True,
        stdout=subprocess.PIPE,
        stderr=stderr_log_file,
    )
    ip_and_port = server_proc.stdout.readline().decode().strip('\n').split(' ')
    ip = ip_and_port[0]
    port = ip_and_port[1]

    proxy_time, prev_t = time_helper(prev_t)
    # Assemble the search command
    # fmt: off
    clp_home = Path(os.getenv("CLP_HOME"))
    search_cmd = [
        str(clp_home / "bin" / "clo"),
        ip,
        port,
        str(archive_directory / archive_id),
        query["pipeline_string"],
        '-c',
        '--tge', str(query['timestamp_begin']),
        '--tle', str(query['timestamp_end']),
    ]
    if query['path_regex']:
        search_cmd.append(query['path_regex'])
    if not query['match_case']:
        search_cmd.append('-i')

    # Start compression
    logger.info("Searching...")
    logger.debug(" ".join(search_cmd))

    search_successful = False

    search_proc = subprocess.Popen(
        search_cmd,
        close_fds=True,
        stdout=subprocess.PIPE,
        stderr=stderr_log_file,
    )

    # Wait for compression to finish
    return_code = search_proc.wait()
    search_time, prev_t = time_helper(prev_t)
    if 0 != return_code:
        logger.error(f"Failed to search, return_code={return_code}")
    else:
        search_successful = True
        parts = search_proc.stdout.readline().decode().split(":")
        num_matches = int(parts[1].strip())

    # a hack to ensure that proxy server flush all data
    # since the next message will only be print out after
    # proxy flushes all data
    proxy_end_message = server_proc.stdout.readline().decode()
    flush_time, prev_t = time_helper(prev_t)

    server_proc.terminate()
    server_proc.wait()
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