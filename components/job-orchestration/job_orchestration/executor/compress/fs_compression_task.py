import datetime
import json
import logging
import os
import pathlib
import subprocess

import yaml
from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import StorageEngine
from job_orchestration.executor.compress.celery import app
from job_orchestration.scheduler.constants import CompressionTaskStatus
from job_orchestration.scheduler.job_config import ClpIoConfig, PathsToCompress
from job_orchestration.scheduler.scheduler_data import (
    CompressionTaskFailureResult,
    CompressionTaskSuccessResult,
)

# Setup logging
logger = get_task_logger(__name__)


def make_clp_command(
    clp_home: pathlib.Path,
    archive_output_dir: pathlib.Path,
    clp_config: ClpIoConfig,
    db_config_file_path: pathlib.Path,
):
    path_prefix_to_remove = clp_config.input.path_prefix_to_remove

    # fmt: off
    compression_cmd = [
        str(clp_home / "bin" / "clp"),
        "c", str(archive_output_dir),
        "--print-archive-stats-progress",
        "--target-dictionaries-size", str(clp_config.output.target_dictionaries_size),
        "--target-segment-size", str(clp_config.output.target_segment_size),
        "--target-encoded-file-size", str(clp_config.output.target_encoded_file_size),
        "--db-config-file", str(db_config_file_path),
    ]
    # fmt: on
    if path_prefix_to_remove:
        compression_cmd.append("--remove-path-prefix")
        compression_cmd.append(path_prefix_to_remove)

    # Use schema file if it exists
    schema_path: pathlib.Path = clp_home / "etc" / "clp-schema.txt"
    if schema_path.exists():
        compression_cmd.append("--schema-path")
        compression_cmd.append(str(schema_path))

    return compression_cmd


def make_clp_s_command(
    clp_home: pathlib.Path,
    archive_output_dir: pathlib.Path,
    clp_config: ClpIoConfig,
    db_config_file_path: pathlib.Path,
):
    # fmt: off
    compression_cmd = [
        str(clp_home / "bin" / "clp-s"),
        "c", str(archive_output_dir),
        "--print-archive-stats",
        "--target-encoded-size", str(clp_config.output.target_segment_size + clp_config.output.target_dictionaries_size),
        "--db-config-file", str(db_config_file_path),
    ]
    # fmt: on

    if clp_config.input.timestamp_key is not None:
        compression_cmd.append("--timestamp-key")
        compression_cmd.append(clp_config.input.timestamp_key)

    return compression_cmd


def run_clp(
    clp_config: ClpIoConfig,
    clp_home: pathlib.Path,
    data_dir: pathlib.Path,
    archive_output_dir: pathlib.Path,
    logs_dir: pathlib.Path,
    job_id: int,
    task_id: int,
    paths_to_compress: PathsToCompress,
    clp_metadata_db_connection_config,
):
    """
    Compresses files from an FS into archives on an FS

    :param clp_config: ClpIoConfig
    :param clp_home:
    :param data_dir:
    :param archive_output_dir:
    :param logs_dir:
    :param job_id:
    :param task_id:
    :param paths_to_compress: PathToCompress
    :param clp_metadata_db_connection_config
    :return: tuple -- (whether compression was successful, output messages)
    """
    clp_storage_engine = str(os.getenv("CLP_STORAGE_ENGINE"))

    instance_id_str = f"compression-job-{job_id}-task-{task_id}"

    # Generate database config file for clp
    db_config_file_path = data_dir / f"{instance_id_str}-db-config.yml"
    db_config_file = open(db_config_file_path, "w")
    yaml.safe_dump(clp_metadata_db_connection_config, db_config_file)
    db_config_file.close()

    if StorageEngine.CLP == clp_storage_engine:
        compression_cmd = make_clp_command(
            clp_home=clp_home,
            archive_output_dir=archive_output_dir,
            clp_config=clp_config,
            db_config_file_path=db_config_file_path,
        )
    elif StorageEngine.CLP_S == clp_storage_engine:
        compression_cmd = make_clp_s_command(
            clp_home=clp_home,
            archive_output_dir=archive_output_dir,
            clp_config=clp_config,
            db_config_file_path=db_config_file_path,
        )
    else:
        logger.error(f"Unsupported storage engine {clp_storage_engine}")
        return False, {"error_message": f"Unsupported storage engine {clp_storage_engine}"}

    # Prepare list of paths to compress for clp
    file_paths = paths_to_compress.file_paths
    log_list_path = data_dir / f"{instance_id_str}-log-paths.txt"
    with open(log_list_path, "w") as file:
        if len(file_paths) > 0:
            for path_str in file_paths:
                file.write(path_str)
                file.write("\n")
        if paths_to_compress.empty_directories and len(paths_to_compress.empty_directories) > 0:
            # Prepare list of paths to compress for clp
            for path_str in paths_to_compress.empty_directories:
                file.write(path_str)
                file.write("\n")

        compression_cmd.append("--files-from")
        compression_cmd.append(str(log_list_path))

    # Open stderr log file
    stderr_log_path = logs_dir / f"{instance_id_str}-stderr.log"
    stderr_log_file = open(stderr_log_path, "w")

    # Start compression
    logger.debug("Compressing...")
    compression_successful = False
    proc = subprocess.Popen(compression_cmd, stdout=subprocess.PIPE, stderr=stderr_log_file)

    # Compute the total amount of data compressed
    last_archive_stats = None
    total_uncompressed_size = 0
    total_compressed_size = 0
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        stats = json.loads(line.decode("ascii"))
        if last_archive_stats is not None and stats["id"] != last_archive_stats["id"]:
            # We've started a new archive so add the previous archive's last
            # reported size to the total
            total_uncompressed_size += last_archive_stats["uncompressed_size"]
            total_compressed_size += last_archive_stats["size"]
        last_archive_stats = stats
    if last_archive_stats is not None:
        # Add the last archive's last reported size
        total_uncompressed_size += last_archive_stats["uncompressed_size"]
        total_compressed_size += last_archive_stats["size"]

    # Wait for compression to finish
    return_code = proc.wait()
    if 0 != return_code:
        logger.error(f"Failed to compress, return_code={str(return_code)}")
    else:
        compression_successful = True

        # Remove generated temporary files
        if log_list_path:
            log_list_path.unlink()
        db_config_file_path.unlink()
    logger.debug("Compressed.")

    # Close stderr log file
    stderr_log_file.close()

    if compression_successful:
        return compression_successful, {
            "total_uncompressed_size": total_uncompressed_size,
            "total_compressed_size": total_compressed_size,
        }
    else:
        return compression_successful, {"error_message": f"See logs {stderr_log_path}"}


@app.task(bind=True)
def compress(
    self: Task,
    job_id: int,
    task_id: int,
    clp_io_config_json: str,
    paths_to_compress_json: str,
    clp_metadata_db_connection_config,
):
    clp_home_str = os.getenv("CLP_HOME")
    data_dir_str = os.getenv("CLP_DATA_DIR")
    archive_output_dir_str = os.getenv("CLP_ARCHIVE_OUTPUT_DIR")
    logs_dir_str = os.getenv("CLP_LOGS_DIR")

    clp_io_config = ClpIoConfig.parse_raw(clp_io_config_json)
    paths_to_compress = PathsToCompress.parse_raw(paths_to_compress_json)

    start_time = datetime.datetime.now()
    logger.info(f"[job_id={job_id} task_id={task_id}] COMPRESSION STARTED.")
    compression_successful, worker_output = run_clp(
        clp_io_config,
        pathlib.Path(clp_home_str),
        pathlib.Path(data_dir_str),
        pathlib.Path(archive_output_dir_str),
        pathlib.Path(logs_dir_str),
        job_id,
        task_id,
        paths_to_compress,
        clp_metadata_db_connection_config,
    )
    duration = (datetime.datetime.now() - start_time).total_seconds()
    logger.info(f"[job_id={job_id} task_id={task_id}] COMPRESSION COMPLETED.")

    if compression_successful:
        return CompressionTaskSuccessResult(
            task_id=task_id,
            status=CompressionTaskStatus.SUCCEEDED,
            start_time=start_time,
            duration=duration,
            total_uncompressed_size=worker_output["total_uncompressed_size"],
            total_compressed_size=worker_output["total_compressed_size"],
        ).dict()
    else:
        return CompressionTaskFailureResult(
            task_id=task_id,
            status=CompressionTaskStatus.FAILED,
            start_time=start_time,
            duration=duration,
            error_message=worker_output["error_message"],
        ).dict()
