#!/usr/bin/env python3

import argparse
import logging
import os
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Optional

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
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)
# Prevents double logging from sub loggers for specific jobs
logger.propagate = False

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


def handle_job_impl(
        job_id_str: str,
        output_type: str,
        output_config: Dict[str, Any],
        db_manager: DBManager,
        celery_worker_method_base_kwargs: Dict[str, Any],
        progress_reporting_disabled: bool,
        target_archive_size: int,
) -> JobStatus:
    # Extract job metadata
    try:
        job_metadata: Dict[str, Any] = db_manager.get_job_metadata(job_id_str)
        job_input_type: str = job_metadata["input_type"]
        job_input_config: Dict[str, Any] = job_metadata["input_config"]
        job_output_config: Dict[str, Any] = job_metadata["output_config"]
    except Exception as e:
        logger.error(f"Failed to parse initial job configurations: {e}")
        return JobStatus.FAILED

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
        return JobStatus.FAILED
    logger.info(f"Submitting job {job_id_str} to celery")
    jobs_group = group(distributed_compression_jobs)
    active_jobs = jobs_group.apply_async()
    num_compression_jobs = len(active_jobs)
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

    # V0.5 TODO Deduplicate
    def update_job_results(results: Dict[str, Any], archive_status: Dict[str, Any]):
        results['total_uncompressed_size'] += archive_status['total_uncompressed_size']
        results['total_compressed_size'] += archive_status['total_compressed_size']
        results['num_messages'] += archive_status['num_messages']
        results['num_files'] += archive_status['num_files']
        results['begin_ts'] = min(results['begin_ts'], archive_status['begin_ts'])
        results['end_ts'] = max(results['end_ts'], archive_status['end_ts'])

    while True:
        try:
            returned_results = get_results_or_timeout(active_jobs)
            if returned_results != None:
                # Check for finished jobs
                for subtask_result in returned_results:
                    if not subtask_result["status"]:
                        all_worker_jobs_successful = False
                        logger.error(f"Worker of {job_id_str} failed. See the worker logs for details.")
                    else:
                        update_job_results(job_result, subtask_result)
                        db_manager.update_job_progression(job_id_str)
                break
        except Exception as e:
            all_worker_jobs_successful = False
            logger.error(f"job `{job_id_str}` failed: {e}")
            break

        if not progress_reporting_disabled:
            db_manager.update_job_progression(job_id_str)

        logger.info(f"Waiting for sub-job(s) of job {job_id_str} to finish.")
        time.sleep(1)

    if not all_worker_jobs_successful:
        return JobStatus.FAILED
    db_manager.update_job_stats(job_id_str, job_result)
    if job_completed_with_errors:
        return JobStatus.SUCCESS_WITH_ERRORS
    else:
        return JobStatus.SUCCESS


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
        path = CONTAINER_INPUT_LOGS_ROOT_DIR / path.relative_to(path.anchor)

        if not path.is_dir():
            parsed_list_paths.append(path)
        else:
            for internal_path in path.rglob("*"):
                # V0.5 TODO: customization for filer
                # rglob on filer returns directory
                if not internal_path.is_dir():
                    parsed_list_paths.append(internal_path)

    # Create the compression buffer
    jobs_arguments: List[Dict[str, Any]] = []
    job_input_config_template = {
        "path_prefix_to_remove": job_input_config.get("path_prefix_to_remove")
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
    target_num_archives: int = 0
    if "target_num_archives" in job_output_config:
        target_num_archives = int(job_output_config["target_num_archives"])
        files: List[FileMetadata] = []
        total_file_size: int = 0
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
            if target_num_archives > 0:
                files.append(file_metadata)
                total_file_size += file_metadata.estimated_uncompressed_size
            else:
                paths_to_compress_buffer.add_file(file_metadata)
        elif empty_directory_str is not None:
            paths_to_compress_buffer.add_empty_directory(empty_directory_str)

    if target_num_archives > 0:
        paths_to_compress_buffer.add_files(
            files=files,
            target_num_archives=target_num_archives,
            target_archive_size=int(total_file_size / target_num_archives),
        )

    paths_to_compress_buffer.flush()
    for job_arguments in jobs_arguments:
        distributed_compression_jobs.append(fs_to_fs_compress.s(**job_arguments))
    return path_validation_erred


def handle_job(
        job_id_str: str,
        output_type: str,
        output_config: Dict[str, Any],
        db_manager: DBManager,
        celery_worker_method_base_kwargs: Dict[str, Any],
        progress_reporting_disabled: bool,
        target_archive_size: int,
) -> JobStatus:
    logger.info(f"Starting job `{job_id_str}`.")

    job_completion_status = handle_job_impl(
        job_id_str=job_id_str,
        output_type=output_type,
        output_config=output_config,
        db_manager=db_manager,
        celery_worker_method_base_kwargs=celery_worker_method_base_kwargs,
        progress_reporting_disabled=progress_reporting_disabled,
        target_archive_size=target_archive_size,
    )

    logger.info(f"Finished job `{job_id_str}`.")
    return job_completion_status


def handle_jobs(
        output_type: str,
        output_config: Dict[str, Any],
        db_manager: DBManager,
        celery_worker_method_base_kwargs: Dict[str, Any],
        progress_reporting_disabled: bool,
        target_archive_size: int,
) -> None:
    last_submission_timestamp: Optional[datetime] = None
    while True:
        jobs_to_process = db_manager.get_jobs_after_timestamp(last_submission_timestamp)

        for job_id, job_status in jobs_to_process:
            last_submission_timestamp = db_manager.get_job_submission_timestamp(job_id)

            if JobStatus.CANCELLING == job_status:
                if db_manager.set_job_status(job_id, JobStatus.CANCELLED, prev_status=job_status):
                    logger.info(f"Confirmed cancellation for job `{job_id}`.")
                else:
                    logger.error(f"Failed to cancel job `{job_id}`.")
            elif JobStatus.PENDING == job_status:
                if not db_manager.set_job_status(
                        job_id,
                        JobStatus.RUNNING,
                        prev_status=job_status,
                        begin_timestamp=datetime.now(),
                ):
                    logger.error(
                        f"Failed to mark job `{job_id}` as `running`. Perhaps a cancellation was"
                        " requested."
                    )
                    if db_manager.set_job_status(
                            job_id, JobStatus.CANCELLED, prev_status=JobStatus.CANCELLING
                    ):
                        logger.info(f"Confirmed cancellation for job `{job_id}`.")
                    else:
                        logger.error(f"Failed to cancel job `{job_id}`.")
                    continue

                job_completion_status = handle_job(
                    job_id_str=job_id,
                    output_type=output_type,
                    output_config=output_config,
                    db_manager=db_manager,
                    celery_worker_method_base_kwargs=celery_worker_method_base_kwargs,
                    progress_reporting_disabled=progress_reporting_disabled,
                    target_archive_size=target_archive_size,
                )

                now = datetime.now()
                if JobStatus.FAILED == job_completion_status:
                    db_manager.set_job_status(job_id, JobStatus.FAILED, end_timestamp=now)
                elif JobStatus.SUCCESS_WITH_ERRORS == job_completion_status:
                    db_manager.set_job_status(
                        job_id, JobStatus.DONE, end_timestamp=now, errors=True
                    )
                elif JobStatus.SUCCESS == job_completion_status:
                    db_manager.set_job_status(job_id, JobStatus.DONE, end_timestamp=now)
                else:
                    logger.error(
                        f"Job `{job_id}` ended with incorrect completion status"
                        f" `{job_completion_status}`."
                    )

        db_manager.update_compression_stat()
        logger.debug("Sleeping for 1 second.")
        time.sleep(1)


def main(argv: List[str]) -> int:
    # fmt: off
    args_parser = argparse.ArgumentParser(description="Wait for and run compression jobs.")
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')
    args_parser.add_argument(
        "--no-progress-reporting", action="store_true",
        help="Disables progress reporting."
    )
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
        "target_dictionaries_size": clp_config.archive_output.target_dictionaries_size,
        "target_encoded_file_size": clp_config.archive_output.target_encoded_file_size,
        "target_segment_size": clp_config.archive_output.target_segment_size,
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
        progress_reporting_disabled=parsed_args.no_progress_reporting,
        target_archive_size=clp_config.archive_output.target_archive_size,
    )
    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))