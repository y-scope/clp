import logging
import os
import sys
import signal
import subprocess
from pathlib import Path
from typing import Any, Dict

from celery.app.task import Task
from celery.utils.log import get_task_logger

from clp_py_utils.clp_logging import get_logging_formatter, set_logging_level

from job_orchestration.executor.search.celery import app
from job_orchestration.job_config import SearchConfig


logger = get_task_logger(__name__)

@app.task(bind=True)
def search(
    self: Task,
    job_id: str,
    search_config_obj: dict,
    archive_id: str,
    results_cache_uri: str,
) -> Dict[str, Any]:
    task_id = str(self.request.id)
    clp_home = Path(os.getenv("CLP_HOME"))
    archive_directory = Path(os.getenv('CLP_ARCHIVE_OUTPUT_DIR'))
    clp_logs_dir = Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = str(os.getenv("CLP_LOGGING_LEVEL"))

    # Setup logging to file
    worker_logs_dir = clp_logs_dir / job_id
    worker_logs_dir.mkdir(exist_ok=True, parents=True)
    worker_logs = worker_logs_dir / f"{task_id}.log"
    logging_file_handler = logging.FileHandler(filename=worker_logs, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)
    set_logging_level(logger, clp_logging_level)
    stderr_log_path = worker_logs_dir / f"{task_id}-stderr.log"
    stderr_log_file = open(stderr_log_path, "w")

    logger.info(f"Started job {job_id}. Task Id={task_id}.")

    search_config = SearchConfig.parse_obj(search_config_obj)
    search_cmd = [
        str(clp_home / "bin" / "clo"),
        results_cache_uri,
        job_id,
        str(archive_directory / archive_id),
        search_config.query_string,
    ]

    if search_config.begin_timestamp is not None:
        search_cmd.append('--tge')
        search_cmd.append(str(search_config.begin_timestamp))
    if search_config.end_timestamp is not None:
        search_cmd.append('--tle')
        search_cmd.append(str(search_config.end_timestamp))
    if search_config.path_filter is not None:
        search_cmd.append(search_config.path_filter)

    logger.info(f'Running: {" ".join(search_cmd)}')
    search_successful = False
    search_proc = subprocess.Popen(
        search_cmd,
        preexec_fn=os.setpgrp,
        close_fds=True,
        stdout=stderr_log_file,
        stderr=stderr_log_file,
    )

    def sigterm_handler(_signo, _stack_frame):
        logger.debug("Entered sigterm handler")
        if search_proc.poll() is None:
            logger.debug("Trying to kill search process")
            # Kill the process group in case the search process also forked
            os.killpg(os.getpgid(search_proc.pid), signal.SIGTERM)
            os.waitpid(search_proc.pid, 0)
            logger.info(f"Cancelling search task: {task_id}")
        sys.exit(_signo + 128)

    # Register the function to kill the child process at exit
    signal.signal(signal.SIGTERM, sigterm_handler)

    logger.info("Waiting for search to finish...")
    search_proc.communicate()
    return_code = search_proc.returncode
    if 0 != return_code:
        logger.error(f"Failed to search, job {job_id}. Task Id={task_id}, return_code={return_code}")
    else:
        search_successful = True
        logger.info(f"Search completed for job {job_id}. Task Id={task_id}")

    # Close log files
    stderr_log_file.close()
    logger.removeHandler(logging_file_handler)
    logging_file_handler.close()

    results = {
        'status': search_successful,
        'job_id': job_id,
        'task_id': task_id,
    }

    return results
