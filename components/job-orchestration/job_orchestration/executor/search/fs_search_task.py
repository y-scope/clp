import os
import signal
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict

from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import StorageEngine
from clp_py_utils.clp_logging import set_logging_level
from job_orchestration.executor.search.celery import app
from job_orchestration.scheduler.job_config import SearchConfig
from job_orchestration.scheduler.scheduler_data import SearchTaskResult

# Setup logging
logger = get_task_logger(__name__)


def make_command(
    storage_engine: str,
    clp_home: Path,
    archives_dir: Path,
    archive_id: str,
    search_config: SearchConfig,
    results_cache_uri: str,
    results_collection: str,
):
    if StorageEngine.CLP == storage_engine:
        command = [str(clp_home / "bin" / "clo"), str(archives_dir / archive_id)]
        if search_config.path_filter is not None:
            command.append("--file-path")
            command.append(search_config.path_filter)
    elif StorageEngine.CLP_S == storage_engine:
        command = [
            str(clp_home / "bin" / "clp-s"),
            "s",
            str(archives_dir),
            "--archive-id",
            archive_id,
        ]
    else:
        raise ValueError(f"Unsupported storage engine {storage_engine}")

    command.append(search_config.query_string)
    if search_config.begin_timestamp is not None:
        command.append("--tge")
        command.append(str(search_config.begin_timestamp))
    if search_config.end_timestamp is not None:
        command.append("--tle")
        command.append(str(search_config.end_timestamp))
    if search_config.ignore_case:
        command.append("--ignore-case")

    if search_config.aggregation_config is not None:
        aggregation_config = search_config.aggregation_config
        if aggregation_config.do_count_aggregation is not None:
            command.append("--count")
        if aggregation_config.count_by_time_bucket_size is not None:
            command.append("--count-by-time")
            command.append(str(aggregation_config.count_by_time_bucket_size))

        # fmt: off
        command.extend((
             "reducer",
             "--host", aggregation_config.reducer_host,
             "--port", str(aggregation_config.reducer_port),
             "--job-id", str(aggregation_config.job_id)
        ))
        # fmt: on
    elif search_config.network_address is not None:
        # fmt: off
        command.extend((
            "network",
            "--host", search_config.network_address[0],
            "--port", str(search_config.network_address[1])
        ))
        # fmt: on
    else:
        # fmt: off
        command.extend((
            "results-cache",
            "--uri", results_cache_uri,
            "--collection", results_collection,
            "--max-num-results", str(search_config.max_num_results)
        ))
        # fmt: on

    return command


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
    archive_directory = Path(os.getenv("CLP_ARCHIVE_OUTPUT_DIR"))
    clp_logs_dir = Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = str(os.getenv("CLP_LOGGING_LEVEL"))
    clp_storage_engine = str(os.getenv("CLP_STORAGE_ENGINE"))

    # Setup logging to file
    worker_logs_dir = clp_logs_dir / job_id
    worker_logs_dir.mkdir(exist_ok=True, parents=True)
    set_logging_level(logger, clp_logging_level)
    clo_log_path = worker_logs_dir / f"{task_id}-clo.log"
    clo_log_file = open(clo_log_path, "w")

    logger.info(f"Started task for job {job_id}")

    search_config = SearchConfig.parse_obj(search_config_obj)

    try:
        search_command = make_command(
            storage_engine=clp_storage_engine,
            clp_home=clp_home,
            archives_dir=archive_directory,
            archive_id=archive_id,
            search_config=search_config,
            results_cache_uri=results_cache_uri,
            results_collection=job_id,
        )
    except ValueError as e:
        logger.error(f"Error creating search command: {e}")
        return SearchTaskResult(
            success=False,
            task_id=task_id,
        ).dict()

    logger.info(f'Running: {" ".join(search_command)}')
    search_successful = False
    search_proc = subprocess.Popen(
        search_command,
        preexec_fn=os.setpgrp,
        close_fds=True,
        stdout=clo_log_file,
        stderr=clo_log_file,
    )

    def sigterm_handler(_signo, _stack_frame):
        logger.debug("Entered sigterm handler")
        if search_proc.poll() is None:
            logger.debug("Trying to kill search process")
            # Kill the process group in case the search process also forked
            os.killpg(os.getpgid(search_proc.pid), signal.SIGTERM)
            os.waitpid(search_proc.pid, 0)
            logger.info(f"Cancelling search task.")
        # Add 128 to follow convention for exit codes from signals
        # https://tldp.org/LDP/abs/html/exitcodes.html#AEN23549
        sys.exit(_signo + 128)

    # Register the function to kill the child process at exit
    signal.signal(signal.SIGTERM, sigterm_handler)

    logger.info("Waiting for search to finish")
    # communicate is equivalent to wait in this case, but avoids deadlocks if we switch to piping
    # stdout/stderr in the future.
    search_proc.communicate()
    return_code = search_proc.returncode
    if 0 != return_code:
        logger.error(f"Failed search task for job {job_id} - return_code={return_code}")
    else:
        search_successful = True
        logger.info(f"Search task completed for job {job_id}")

    # Close log files
    clo_log_file.close()

    return SearchTaskResult(
        success=search_successful,
        task_id=task_id,
    ).dict()
