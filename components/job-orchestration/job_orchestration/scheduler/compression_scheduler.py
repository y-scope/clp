#!/usr/bin/env python3

import argparse
import logging
import os
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from celery import group, signature
from celery.exceptions import TimeoutError

from pydantic import ValidationError

from clp_package_utils.general import CONTAINER_INPUT_LOGS_ROOT_DIR
from clp_py_utils.clp_config import CLPConfig, Database, ResultsCache
from clp_py_utils.clp_logging import get_logging_level
from clp_py_utils.core import read_yaml_config_file

from clp_py_utils.compression import (  # type: ignore
    FileMetadata,
    validate_path_and_get_info,
)
from job_orchestration.executor.compression.fs_to_fs_compress_method import (
    compress as fs_to_fs_compress,
)

from .common import JobStatus  # type: ignore
from .compression_db_manager import DBManager, MongoDBManager  # type: ignore
from .partition import PathsToCompressBuffer  # type: ignore

# Setup logging
# Create logger
logger = logging.getLogger("compression-job-handler")
logger.setLevel(logging.INFO)
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
# Setup console logging
# V0.5 TODO: for some reason propogate = false is not working.
# logging_console_handler = logging.StreamHandler()
# logging_console_handler.setFormatter(logging_formatter)
# logger.addHandler(logging_console_handler)
# Prevents double logging from sub loggers for specific jobs
logger.propagate = False


class CompressionJob:
    def __init__(self, task: any, submission_ts: float, schedule_start_ts: float, celery_submission_ts: float) -> None:
        self.task: any = task
        self.submission_ts: float = submission_ts
        self.schedule_start_ts: float = schedule_start_ts
        self.celery_submission_ts: float = celery_submission_ts


active_jobs = {}

'''
Polling doesn't appear to work correctly with the amqp backend --
results never show up until get() is called. To circumvent this we call
get with a short timeout here to see if results happen to be ready instead
of polling ready() like we should be able to.

https://github.com/celery/celery/issues/4084
^Issue open since 2017 showing that this is a bug
'''


# V0.5 TODO Deduplicate
def update_job_results(results: Dict[str, Any], archive_status: Dict[str, Any]):
    results['total_uncompressed_size'] += archive_status['total_uncompressed_size']
    results['total_compressed_size'] += archive_status['total_compressed_size']
    results['num_messages'] += archive_status['num_messages']
    results['num_files'] += archive_status['num_files']
    results['begin_ts'] = min(results['begin_ts'], archive_status['begin_ts'])
    results['end_ts'] = max(results['end_ts'], archive_status['end_ts'])


def try_getting_results(result):
    if not result.ready():
        return None
    return result.get()


def prepare_fs_compression_jobs(
        job_id_str: str,
        worker_base_arguments: Dict[str, Any],
        distributed_compression_jobs: List[signature],
        target_archive_size: int,
        job_input_config: Dict[str, Any],
        job_output_config: Dict[str, Any],
) -> bool:
    # Parse the list of paths to compress
    parsed_list_paths: List[Path] = []
    for path_str in job_input_config["paths"]:
        stripped_path_str = path_str.strip()
        if "" == stripped_path_str:
            # Skip empty paths
            continue
        path = Path(stripped_path_str)
        # TODO This assumes we're running in a container
        mounted_path = CONTAINER_INPUT_LOGS_ROOT_DIR / path.relative_to(path.anchor)
        if not mounted_path.is_dir():
            if mounted_path.exists():
                parsed_list_paths.append(mounted_path)
            else:
                logger.warning(f"Skipping non-existent input: {path}")
        else:
            for internal_path in mounted_path.rglob("*"):
                # V0.5 TODO: customization for filer
                # rglob on filer returns directory
                if not internal_path.is_dir():
                    parsed_list_paths.append(internal_path)

    # Create the compression buffer
    jobs_arguments: List[Dict[str, Any]] = []
    job_input_config_template = {
        "path_prefix_to_remove": job_input_config.get("path_prefix_to_remove"),
        "timestamp_key": job_input_config.get("timestamp_key"),
        "index": job_input_config.get("index"),
    }
    paths_to_compress_buffer = PathsToCompressBuffer(
        logger=logger,
        worker_base_arguments=worker_base_arguments,
        jobs_arguments=jobs_arguments,
        target_archive_size=target_archive_size,
        file_size_to_trigger_compression=target_archive_size * 2,
        maintain_file_ordering=False,
        allow_empty_directories=True,
        job_id_str=job_id_str,
        job_input_config=job_input_config_template,
        job_output_config=job_output_config,
    )

    # Process the paths list
    path_validation_erred: bool = False

    for file_path in parsed_list_paths:
        try:
            # TODO required_parent_dir replaced with
            # CONTAINER_INPUT_LOGS_ROOT_DIR
            file_metadata, empty_directory_str = validate_path_and_get_info(
                CONTAINER_INPUT_LOGS_ROOT_DIR, file_path
            )
            # TODO remove the CONTAINER_INPUT_LOGS_ROOT_DIR
            # prefix from the file_metadata path, so the
            # compression worker see the path as real path
            file_metadata.path = Path('/') / file_metadata.path.relative_to(CONTAINER_INPUT_LOGS_ROOT_DIR)
        except ValueError as e:
            logger.error(str(e))
            path_validation_erred = True
            continue

        if file_metadata is not None:
            paths_to_compress_buffer.add_file(file_metadata)
        elif empty_directory_str is not None:
            paths_to_compress_buffer.add_empty_directory(empty_directory_str)

    paths_to_compress_buffer.flush()
    for job_arguments in jobs_arguments:
        distributed_compression_jobs.append(fs_to_fs_compress.s(**job_arguments))
    return path_validation_erred


'''
def handle_job(
        job_id_str: str,
        output_type: str,
        output_config: Dict[str, Any],
        db_manager: DBManager,
        celery_worker_method_base_kwargs: Dict[str, Any],
        target_archive_size: int,
) -> Tuple[JobStatus, float]:
    logger.info(f"Starting job `{job_id_str}`.")
    # Extract job metadata
    try:
        job_metadata: Dict[str, Any] = db_manager.get_job_metadata(job_id_str)
        job_input_type: str = job_metadata["input_type"]
        job_input_config: Dict[str, Any] = job_metadata["input_config"]
        job_output_config: Dict[str, Any] = job_metadata["output_config"]
        submission_ts: float = job_metadata["submission_timestamp"] / 1000
        schedule_start_ts: float = job_metadata["begin_timestamp"].timestamp()
    except Exception as e:
        logger.error(f"Failed to parse initial job configurations: {e}")
        return JobStatus.FAILED_TO_SUBMIT, 0

    # Override output config with settings specified in the job
    target_archive_size = job_output_config.get("target_archive_size", target_archive_size)

    for key, value in output_config.items():
        if key not in job_output_config:
            job_output_config[key] = value

    # Create populate the job buffer with Celery job signatures
    distributed_compression_jobs: List[signature] = []
    try:
        if "fs" == job_input_type and "fs" == output_type:
            job_completed_with_errors = prepare_fs_compression_jobs(
                job_id_str=job_id_str,
                worker_base_arguments=celery_worker_method_base_kwargs,
                distributed_compression_jobs=distributed_compression_jobs,
                target_archive_size=target_archive_size,
                job_input_config=job_input_config,
                job_output_config=job_output_config,
            )
        else:
            raise NotImplementedError("Unsupported input/output compression source pair.")
    except Exception as e:
        logger.error(str(e))
        return JobStatus.FAILED_TO_SUBMIT, 0

    if len(distributed_compression_jobs) == 0:
        logger.warning(f"No valid inputs, skipping job {job_id_str}")
        return JobStatus.NO_INPUTS_TO_COMPRESS, 0

    celery_submission_ts = time.time()
    logger.info(f"Submitting job {job_id_str} to celery")
    jobs_group = group(distributed_compression_jobs)
    job_results_async = jobs_group.apply_async()
    num_compression_jobs = len(job_results_async)
    logger.info(f"Waiting for job {job_id_str}'s {num_compression_jobs} sub-job(s) to finish.")

    # Monitor dispatched jobs' statuses
    all_worker_jobs_successful = True
    # V0.5 TODO: ideally we should update database for each successful subtask but
    # I don't have time to do it properly. instead, I do the aggregation here
    job_result: Dict[str, Any] = {
        "total_uncompressed_size": 0,
        "total_compressed_size": 0,
        "num_messages": 0,
        "num_files": 0,
        "begin_ts": sys.maxsize,
        "end_ts": 0
    }

    task_results_list = []
    while True:
        try:
            returned_results = try_getting_results(job_results_async)
            if returned_results != None:
                job_completion_ts = time.time()
                # Check for finished jobs
                for task_result in returned_results:
                    logger.info(task_result)
                    task_results_list.append(task_result)
                    if not task_result["status"]:
                        all_worker_jobs_successful = False
                        logger.error(f"Worker of {job_id_str} failed. See the worker logs for details.")
                    else:
                        update_job_results(job_result, task_result["archive_stat"])
                        db_manager.update_job_progression(job_id_str)
                break
        except Exception as e:
            all_worker_jobs_successful = False
            logger.error(f"job `{job_id_str}` failed: {e}")
            break

        logger.info(f"Waiting for sub-job(s) of job {job_id_str} to finish.")
        time.sleep(1)
    logger.info(f"Finished job `{job_id_str}`.")

    job_metrics = {
        "submission_ts": submission_ts,
        "scheduler_time": schedule_start_ts - submission_ts,
        "job_setup_time": celery_submission_ts - schedule_start_ts,
        "celery_worker_time": job_completion_ts - celery_submission_ts,
        "end_to_end_time": job_completion_ts - submission_ts
    }
    if not db_manager.update_job_stats(job_id_str, job_result):
        logger.warning(f"stats of job {job_id_str} not updated, job doesn't exist")
    if not db_manager.update_job_metrics(job_id_str, job_metrics):
        logger.warning(f"metrics of job {job_id_str} not updated, job doesn't exist")
    db_manager.insert_tasks_metrics(task_results_list)

    if not all_worker_jobs_successful:
        return JobStatus.FAILED, job_completion_ts
    if job_completed_with_errors:
        return JobStatus.SUCCESS_WITH_ERRORS, job_completion_ts
    else:
        return JobStatus.SUCCESS, job_completion_ts
'''


def submit_job(
        job_id_str: str,
        output_type: str,
        output_config: Dict[str, Any],
        db_manager: DBManager,
        celery_worker_method_base_kwargs: Dict[str, Any],
        target_archive_size: int,
) -> JobStatus:
    global active_jobs
    logger.info(f"Starting job `{job_id_str}`.")
    # Extract job metadata
    try:
        job_metadata: Dict[str, Any] = db_manager.get_job_metadata(job_id_str)
        job_input_type: str = job_metadata["input_type"]
        job_input_config: Dict[str, Any] = job_metadata["input_config"]
        job_output_config: Dict[str, Any] = job_metadata["output_config"]
        submission_ts: float = job_metadata["submission_timestamp"] / 1000
        schedule_start_ts = float(job_metadata["begin_timestamp"].timestamp())
    except Exception as e:
        logger.error(f"Failed to parse initial job configurations: {e}")
        return JobStatus.FAILED_TO_SUBMIT

    # Override output config with settings specified in the job
    target_archive_size = job_output_config.get("target_archive_size", target_archive_size)

    for key, value in output_config.items():
        if key not in job_output_config:
            job_output_config[key] = value

    # Create populate the job buffer with Celery job signatures
    distributed_compression_jobs: List[signature] = []
    try:
        if "fs" == job_input_type and "fs" == output_type:
            job_completed_with_errors = prepare_fs_compression_jobs(
                job_id_str=job_id_str,
                worker_base_arguments=celery_worker_method_base_kwargs,
                distributed_compression_jobs=distributed_compression_jobs,
                target_archive_size=target_archive_size,
                job_input_config=job_input_config,
                job_output_config=job_output_config,
            )
        else:
            raise NotImplementedError("Unsupported input/output compression source pair.")
    except Exception as e:
        logger.error(str(e))
        return JobStatus.FAILED_TO_SUBMIT

    if len(distributed_compression_jobs) == 0:
        logger.warning(f"No valid inputs, skipping job {job_id_str}")
        return JobStatus.NO_INPUTS_TO_COMPRESS

    celery_submission_ts = float(time.time())
    logger.info(f"Submitting job {job_id_str} to celery")
    jobs_group = group(distributed_compression_jobs)
    active_jobs[job_id_str] = CompressionJob(jobs_group.apply_async(), submission_ts, schedule_start_ts, celery_submission_ts)
    num_compression_jobs = len(active_jobs[job_id_str].task)
    logger.info(f"Waiting for job {job_id_str}'s {num_compression_jobs} sub-job(s) to finish.")
    return JobStatus.RUNNING


def set_job_finish_status(db_manager: DBManager, job_id_str: str, job_completion_status: JobStatus, end_timestamp: float):
    # FIXME: doesn't handle revoking for cancellation. Effectively we only allow
    # cancelling jobs that haven't yet been scheduled.
    # Written this way because it was the fastest way to finish refactoring
    # this scheduler to handle concurrent jobs
    job_completion_time = datetime.fromtimestamp(end_timestamp)

    if job_completion_status in [
        JobStatus.FAILED_TO_SUBMIT,
        JobStatus.FAILED,
        JobStatus.NO_INPUTS_TO_COMPRESS
    ]:
        db_manager.set_job_status(job_id_str, job_completion_status, end_timestamp=job_completion_time)
    elif JobStatus.SUCCESS_WITH_ERRORS == job_completion_status:
        db_manager.set_job_status(
            job_id_str, JobStatus.DONE, end_timestamp=job_completion_time, errors=True
        )
    elif JobStatus.SUCCESS == job_completion_status:
        db_manager.set_job_status(job_id_str, JobStatus.DONE, end_timestamp=job_completion_time)
    elif JobStatus.CANCELLED == status:
        db_manager.set_job_status(job_id_str, JobStatus.CANCELLED)
    else:
        logger.error(
            f"Job `{job_id_str}` ended with incorrect completion status"
            f" `{job_completion_status}`."
        )


def poll_running_jobs(db_manager: DBManager) -> None:
    global active_jobs
    for job_id_str in list(active_jobs.keys()):
        all_worker_jobs_successful = True
        job_success = False
        # V0.5 TODO: ideally we should update database for each successful subtask but
        # I don't have time to do it properly. instead, I do the aggregation here
        job_result: Dict[str, Any] = {
            "total_uncompressed_size": 0,
            "total_compressed_size": 0,
            "num_messages": 0,
            "num_files": 0,
            "begin_ts": sys.maxsize,
            "end_ts": 0
        }

        task_results_list = []
        try:
            returned_results = try_getting_results(active_jobs[job_id_str].task)
            if returned_results != None:
                job_completion_ts = float(time.time())
                # Check for finished jobs
                for task_result in returned_results:
                    logger.info(task_result)
                    task_results_list.append(task_result)
                    if not task_result["status"]:
                        all_worker_jobs_successful = False
                        logger.error(f"Worker of {job_id_str} failed. See the worker logs for details.")
                    else:
                        job_success = True
                        update_job_results(job_result, task_result["archive_stat"])
                        db_manager.update_job_progression(job_id_str)
            else:
                # If results not ready check next job
                continue
        except Exception as e:
            all_worker_jobs_successful = False
            logger.error(f"job `{job_id_str}` failed: {e}")

        logger.info(f"Finished job `{job_id_str}`.")

        celery_worker_time = job_completion_ts - active_jobs[job_id_str].celery_submission_ts if job_success else 0
        end_to_end_time = job_completion_ts - active_jobs[job_id_str].submission_ts if job_success else 0
        job_metrics = {
            "submission_ts": active_jobs[job_id_str].submission_ts,
            "scheduler_time": active_jobs[job_id_str].schedule_start_ts - active_jobs[job_id_str].submission_ts,
            "job_setup_time": active_jobs[job_id_str].celery_submission_ts - active_jobs[job_id_str].schedule_start_ts,
            "celery_worker_time": celery_worker_time,
            "end_to_end_time": end_to_end_time
        }
        if not db_manager.update_job_stats(job_id_str, job_result):
            logger.warning(f"stats of job {job_id_str} not updated, job doesn't exist")
        if not db_manager.update_job_metrics(job_id_str, job_metrics):
            logger.warning(f"metrics of job {job_id_str} not updated, job doesn't exist")
        db_manager.insert_tasks_metrics(task_results_list)

        # delete job
        del active_jobs[job_id_str]

        job_completion_time_date = datetime.fromtimestamp(job_completion_ts)

        # FIXME: set job status here and delete
        if not all_worker_jobs_successful:
            set_job_finish_status(db_manager, job_id_str, JobStatus.FAILED, job_completion_ts)
        else:
            set_job_finish_status(db_manager, job_id_str, JobStatus.SUCCESS, job_completion_ts)
        # FIXME: what was this trying to do?
        # elif job_completed_with_errors:
        #    set_job_finish_status(db_manager, job_id_str, JobStatus.SUCCESS_WITH_ERRORS, job_completion_ts)


def poll_and_submit_pending_compression_jobs(
        output_type: str,
        output_config: Dict[str, Any],
        db_manager: DBManager,
        celery_worker_method_base_kwargs: Dict[str, Any],
        target_archive_size: int,
        new_job_limit: int,
) -> None:
    if new_job_limit <= 0:
        return

    jobs_to_process = db_manager.get_jobs_up_to_limit(new_job_limit)

    for job_id_str, job_status in jobs_to_process:
        if JobStatus.CANCELLING == job_status:
            if db_manager.set_job_status(job_id_str, JobStatus.CANCELLED, prev_status=job_status):
                logger.info(f"Confirmed cancellation for job `{job_id_str}`.")
            else:
                logger.error(f"Failed to cancel job `{job_id_str}`.")
        elif JobStatus.PENDING == job_status:
            db_manager.set_job_status(job_id_str, JobStatus.PENDING, begin_timestamp=datetime.now())
            new_status = submit_job(
                job_id_str=job_id_str,
                output_type=output_type,
                output_config=output_config,
                db_manager=db_manager,
                celery_worker_method_base_kwargs=celery_worker_method_base_kwargs,
                target_archive_size=target_archive_size,
            )

            if new_status != JobStatus.RUNNING:
                end_timestamp = float(time.time())
                set_job_finish_status(db_manager, job_id_str, new_status, end_timestamp)
            else:
                # FIXME: overrides cancellation
                db_manager.set_job_status(job_id_str, JobStatus.RUNNING)


def handle_jobs(
        output_type: str,
        output_config: Dict[str, Any],
        db_manager: DBManager,
        celery_worker_method_base_kwargs: Dict[str, Any],
        target_archive_size: int,
) -> None:
    global active_jobs
    # timer constant
    UPDATE_TIME = 30
    SLEEP_TIME = 0.1

    db_manager.update_compression_stat()
    update_wait_time = UPDATE_TIME
    while True:
        # TODO: set this limit based on config
        new_job_limit = 960 - len(active_jobs)
        poll_and_submit_pending_compression_jobs(
            output_type=output_type,
            output_config=output_config,
            db_manager=db_manager,
            celery_worker_method_base_kwargs=celery_worker_method_base_kwargs,
            target_archive_size=target_archive_size,
            new_job_limit=new_job_limit,
        )
        poll_running_jobs(db_manager)

        logger.debug(f"Sleeping for {1000 * SLEEP_TIME} ms.")
        time.sleep(SLEEP_TIME)

        # FIXME: this should be run whenever a new job finishes instead of
        # being called periodically
        update_wait_time -= SLEEP_TIME
        if update_wait_time <= 0:
            db_manager.update_compression_stat()
            update_wait_time = UPDATE_TIME


def main(argv: List[str]) -> int:
    # fmt: off
    args_parser = argparse.ArgumentParser(description="Wait for and run compression jobs.")
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')

    parsed_args = args_parser.parse_args(argv[1:])

    # fmt: on
    # Setup logging to file
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / "compression_job_handler.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(logging_formatter)
    logger.addHandler(logging_file_handler)
    # Update logging level based on config
    logging_level_str = os.getenv("CLP_LOGGING_LEVEL")
    logging_level = get_logging_level(logging_level_str)
    logger.setLevel(logging_level)
    logger.info(f"Start logging level = {logging.getLevelName(logging_level)}")

    # Load configuration
    config_path = Path(parsed_args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
        return -1
    except Exception as ex:
        logger.error(ex)
        # read_yaml_config_file already logs the parsing error inside
        return -1

    output_config = {
        "max_encoding_size": clp_config.archive_output.max_encoding_size,
    }
    output_type = clp_config.archive_output.type
    if "fs" == output_type:
        # V0.5 TODO. not used by compression worker yet.
        output_config["directory"] = str(Path(clp_config.archive_output.directory).resolve())

    db_manager: DBManager = MongoDBManager(logger, clp_config.results_cache.get_uri())

    celery_worker_method_base_kwargs: Dict[str, Any] = {
        "clp_db_config": dict(clp_config.database),
    }

    logger.info("compression-job-handler started.")

    handle_jobs(
        output_type=output_type,
        output_config=output_config,
        db_manager=db_manager,
        celery_worker_method_base_kwargs=celery_worker_method_base_kwargs,
        target_archive_size=clp_config.archive_output.target_archive_size,
    )
    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
