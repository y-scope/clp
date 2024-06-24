import datetime
import os
import signal
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict

from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import Database, StorageEngine
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.query.celery import app
from job_orchestration.scheduler.job_config import SearchJobConfig
from job_orchestration.scheduler.scheduler_data import QueryTaskResult, QueryTaskStatus

from .utils import get_logger_file_path, generate_final_task_results, update_query_task_metadata

# Setup logging
logger = get_task_logger(__name__)


def make_command(
    storage_engine: str,
    clp_home: Path,
    archives_dir: Path,
    archive_id: str,
    search_config: SearchJobConfig,
    results_cache_uri: str,
    results_collection: str,
):
    if StorageEngine.CLP == storage_engine:
        command = [str(clp_home / "bin" / "clo"), "s", str(archives_dir / archive_id)]
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
    task_id: int,
    job_config_obj: dict,
    archive_id: str,
    clp_metadata_db_conn_params: dict,
    results_cache_uri: str,
) -> Dict[str, Any]:
    clp_home = Path(os.getenv("CLP_HOME"))
    archive_directory = Path(os.getenv("CLP_ARCHIVE_OUTPUT_DIR"))
    clp_logs_dir = Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = str(os.getenv("CLP_LOGGING_LEVEL"))
    clp_storage_engine = str(os.getenv("CLP_STORAGE_ENGINE"))

    # Setup logging to file
    set_logging_level(logger, clp_logging_level)
    clo_log_path = get_logger_file_path(clp_logs_dir, job_id, task_id)
    clo_log_file = open(clo_log_path, "w")

    logger.info(f"Started task for job {job_id}")

    search_config = SearchJobConfig.parse_obj(job_config_obj)
    sql_adapter = SQL_Adapter(Database.parse_obj(clp_metadata_db_conn_params))

    start_time = datetime.datetime.now()
    task_status: QueryTaskStatus
    try:
        task_command = make_command(
            storage_engine=clp_storage_engine,
            clp_home=clp_home,
            archives_dir=archive_directory,
            archive_id=archive_id,
            search_config=search_config,
            results_cache_uri=results_cache_uri,
            results_collection=job_id,
        )
    except ValueError as e:
        error_message = f"Error creating search command: {e}"
        logger.error(error_message)
        clo_log_file.write(error_message)

        task_status = QueryTaskStatus.FAILED
        update_query_task_metadata(
            sql_adapter,
            task_id,
            dict(status=task_status, duration=0, start_time=start_time),
        )

        clo_log_file.close()
        return QueryTaskResult(
            task_id=task_id,
            status=task_status,
            duration=0,
            error_log_path=str(clo_log_path),
        ).dict()

    search_status = QueryTaskStatus.RUNNING
    update_query_task_metadata(
        sql_adapter, task_id, dict(status=search_status, start_time=start_time)
    )

    logger.info(f'Running: {" ".join(task_command)}')
    search_proc = subprocess.Popen(
        task_command,
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
        task_status = QueryTaskStatus.FAILED
        logger.error(f"Search task {task_id} failed for job {job_id} - return_code={return_code}")
    else:
        task_status = QueryTaskStatus.SUCCEEDED
        logger.info(f"Search task {task_id} completed for job {job_id}")

    clo_log_file.close()
    duration = (datetime.datetime.now() - start_time).total_seconds()

    update_query_task_metadata(
        sql_adapter, task_id, dict(status=task_status, start_time=start_time, duration=duration)
    )

    return generate_final_task_results(task_id, task_status, duration, clo_log_path)
