import datetime
import os
from pathlib import Path
from typing import Any, Dict, List, Optional

from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import Database, StorageEngine, StorageType, WorkerConfig
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.query.celery import app
from job_orchestration.executor.query.utils import (
    report_task_failure,
    run_query_task,
)
from job_orchestration.executor.utils import load_worker_config
from job_orchestration.scheduler.job_config import SearchJobConfig
from job_orchestration.scheduler.scheduler_data import QueryTaskStatus

# Setup logging
logger = get_task_logger(__name__)


def make_command(
    clp_home: Path,
    worker_config: WorkerConfig,
    archive_id: str,
    search_config: SearchJobConfig,
    results_cache_uri: str,
    results_collection: str,
) -> Optional[List[str]]:
    storage_engine = worker_config.package.storage_engine
    archives_dir = worker_config.archive_output.get_directory()

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
        logger.error(f"Unsupported storage engine {storage_engine}")
        return None

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
    job_config: dict,
    archive_id: str,
    clp_metadata_db_conn_params: dict,
    results_cache_uri: str,
) -> Dict[str, Any]:
    task_name = "search"

    # Setup logging to file
    clp_logs_dir = Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = os.getenv("CLP_LOGGING_LEVEL")
    set_logging_level(logger, clp_logging_level)

    logger.info(f"Started {task_name} task for job {job_id}")

    start_time = datetime.datetime.now()
    task_status: QueryTaskStatus
    sql_adapter = SQL_Adapter(Database.parse_obj(clp_metadata_db_conn_params))

    # Load configuration
    clp_config_path = Path(os.getenv("CLP_CONFIG_PATH"))
    worker_config = load_worker_config(clp_config_path, logger)
    if worker_config is None:
        return report_task_failure(
            sql_adapter=sql_adapter,
            task_id=task_id,
            start_time=start_time,
        )

    if worker_config.archive_output.storage.type == StorageType.S3:
        logger.error(f"Search is not supported for the S3 storage type")
        return report_task_failure(
            sql_adapter=sql_adapter,
            task_id=task_id,
            start_time=start_time,
        )

    # Make task_command
    clp_home = Path(os.getenv("CLP_HOME"))
    search_config = SearchJobConfig.parse_obj(job_config)

    task_command = make_command(
        clp_home=clp_home,
        worker_config=worker_config,
        archive_id=archive_id,
        search_config=search_config,
        results_cache_uri=results_cache_uri,
        results_collection=job_id,
    )
    if not task_command:
        logger.error(f"Error creating {task_name} command")
        return report_task_failure(
            sql_adapter=sql_adapter,
            task_id=task_id,
            start_time=start_time,
        )

    task_results, _ = run_query_task(
        sql_adapter=sql_adapter,
        logger=logger,
        clp_logs_dir=clp_logs_dir,
        task_command=task_command,
        task_name=task_name,
        job_id=job_id,
        task_id=task_id,
        start_time=start_time,
    )

    return task_results.dict()
