import os
import pathlib
import subprocess

from celery.utils.log import get_task_logger

from job_orchestration.job_config import SearchConfig
from job_orchestration.executor.celery import app
from job_orchestration.executor.utils import append_message_to_task_results_queue
from job_orchestration.scheduler.constants import TaskUpdateType, TaskStatus
from job_orchestration.scheduler.scheduler_data import TaskUpdate, TaskFailureUpdate

# Setup logging
logger = get_task_logger(__name__)


def run_clo(job_id: int, task_id: int, clp_home: pathlib.Path, archive_output_dir: pathlib.Path,
            logs_dir: pathlib.Path, search_config: SearchConfig, archive_id: str):
    """
    Searches the given archive for the given wildcard query

    :param job_id:
    :param task_id:
    :param clp_home:
    :param archive_output_dir:
    :param logs_dir:
    :param search_config:
    :param archive_id:
    :return: tuple -- (whether the search was successful, output messages)
    """
    # Assemble search command
    cmd = [
        str(clp_home / 'bin' / 'clo'),
        search_config.search_controller_host,
        str(search_config.search_controller_port),
        str(archive_output_dir / archive_id),
        search_config.wildcard_query
    ]
    if search_config.begin_timestamp is not None:
        cmd.append('--tge')
        cmd.append(str(search_config.begin_timestamp))
    if search_config.end_timestamp is not None:
        cmd.append('--tle')
        cmd.append(str(search_config.end_timestamp))
    if search_config.path_filter is not None:
        cmd.append(search_config.path_filter)

    # Open stderr log file
    stderr_filename = f'search-job-{job_id}-task-{task_id}-stderr.log'
    stderr_log_path = logs_dir / stderr_filename
    stderr_log_file = open(stderr_log_path, 'w')

    # Start search
    logger.debug("Searching started...")
    search_successful = False
    proc = subprocess.Popen(cmd, stderr=stderr_log_file)

    # Wait for search to finish
    return_code = proc.wait()
    if 0 != return_code:
        logger.error(f'Failed to search, return_code={str(return_code)}')
    else:
        search_successful = True
    logger.debug("Search complete.")

    # Close stderr log file
    stderr_log_file.close()

    if search_successful:
        return search_successful, None
    else:
        return search_successful, f"See {stderr_filename} in logs directory."


@app.task()
def search(job_id: int, task_id: int, search_config_json: str, archive_id: str):
    clp_home = os.getenv('CLP_HOME')
    archive_output_dir = os.getenv('CLP_ARCHIVE_OUTPUT_DIR')
    logs_dir = os.getenv('CLP_LOGS_DIR')
    celery_broker_url = os.getenv('BROKER_URL')

    search_config = SearchConfig.parse_raw(search_config_json)

    task_update = TaskUpdate(
        type=TaskUpdateType.SEARCH,
        job_id=job_id,
        task_id=task_id,
        status=TaskStatus.IN_PROGRESS
    )
    append_message_to_task_results_queue(celery_broker_url, True, task_update.dict())
    logger.info(f"[job_id={job_id} task_id={task_id}] Search started.")

    search_successful, worker_output = run_clo(job_id, task_id, pathlib.Path(clp_home),
                                               pathlib.Path(archive_output_dir),
                                               pathlib.Path(logs_dir), search_config, archive_id)

    if search_successful:
        task_update.status = TaskStatus.SUCCEEDED
    else:
        task_update = TaskFailureUpdate(
            type=TaskUpdateType.SEARCH,
            job_id=job_id,
            task_id=task_id,
            status=TaskStatus.FAILED,
            error_message=worker_output
        )
    append_message_to_task_results_queue(celery_broker_url, False, task_update.dict())
    logger.info(f"[job_id={job_id} task_id={task_id}] Search complete.")
