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

from job_orchestration.executor.search.celery import app

celery_central_logger = get_task_logger(__name__)
celery_central_logger.setLevel(logging.INFO)

script_dir = Path(__file__).parent.resolve()

def get_clp_home() -> Path:
    # Determine CLP_HOME from an environment variable or this script's path
    _clp_home = None
    if "CLP_HOME" in os.environ:
        _clp_home = Path(os.environ["CLP_HOME"])
    else:
        for path in Path(__file__).resolve().parents:
            if "job_orchestration" == path.name:
                _clp_home = path.parent / "clp"
                break

    if _clp_home is None:
        logger.error("CLP_HOME is not set and could not be determined automatically.")
        sys.exit(-1)
    elif not _clp_home.exists():
        logger.error("CLP_HOME set to nonexistent path.")
        sys.exit(-1)

    return _clp_home.resolve()


@app.task(bind=True)
def search(
    self: Task,
    job_id_str: str,
    fs_input_config: Dict[str, Any], # Not used for now
    output_config: Dict[str, Any], # used to indicate how to output the results
    archive_id: str,
    search_query: Dict[str, str],
) -> bool:

    celery_clp_home_str = get_clp_home()
    celery_logs_dir_str = celery_clp_home_str / "var" / "log" / "celery"
    task_id_str = self.request.id
    celery_central_logger.info(f"Started distributed search job {job_id_str}. Task Id={task_id_str} on archive {archive_id}")

    # Setup logging folder
    celery_logs_dir = Path(celery_logs_dir_str).resolve()
    celery_logs_dir.mkdir(parents=True, exist_ok=True)

    # Create logger
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    # Setup logging to file
    script_log_path = celery_logs_dir / f"{task_id_str}-search-worker-script.log"
    celery_logging_file_handler = logging.FileHandler(filename=script_log_path, encoding="utf-8")
    celery_logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
    celery_logging_file_handler.setFormatter(celery_logging_formatter)
    logger.addHandler(celery_logging_file_handler)

    # Open stderr log file
    stderr_log_path = celery_logs_dir / f"{task_id_str}-stderr.log"
    stderr_log_file = open(stderr_log_path, "w")

    # Obtain archive directory info
    archive_directory = Path(os.getenv('CLP_ARCHIVE_OUTPUT_DIR'))
    logger.debug(f"{archive_directory}")
    server_cmd = [
        "python3", 
        str(script_dir / "proxy_server.py"),
        output_config["database_ip"],
        str(output_config["database_port"]),
        output_config["db_name"],
        job_id_str
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

    # Assemble the search command
    # fmt: off
    clp_home = Path(celery_clp_home_str).resolve()
    search_cmd = [
        str(clp_home / "bin" / "clo"),
        ip,
        port,
        str(archive_directory / archive_id),
    ]
    # Append queries and options
    for option, value in search_query.items():
        if option != "query":
            search_cmd.append(f"--{option}")
        search_cmd.append(value)


    # Start compression
    logger.debug("Searching...")
    search_successful = False

    proc = subprocess.Popen(
        search_cmd,
        close_fds=True,
        stdout=subprocess.PIPE,
        stderr=stderr_log_file,
    )

    # Wait for compression to finish
    return_code = proc.wait()
    if 0 != return_code:
        logger.error(f"Failed to search, return_code={return_code}")
    else:
        search_successful = True

    # a hack to ensure that proxy server flush all data
    proxy_end_message = server_proc.stdout.readline().decode()

    server_proc.terminate()
    server_proc.wait()
    logger.debug("Search completed")
    # Close log files
    stderr_log_file.close()
    logger.removeHandler(celery_logging_file_handler)
    celery_logging_file_handler.close()

    return search_successful