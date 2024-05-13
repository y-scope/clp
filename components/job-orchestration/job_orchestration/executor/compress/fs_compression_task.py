import datetime
import json
import os
import pathlib
import subprocess
from contextlib import closing

import yaml
from celery.app.task import Task
from celery.utils.log import get_task_logger
from clp_py_utils.clp_config import (
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_TASKS_TABLE_NAME,
    Database,
    StorageEngine,
)
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.executor.compress.celery import app
from job_orchestration.scheduler.constants import CompressionTaskStatus
from job_orchestration.scheduler.job_config import ClpIoConfig, PathsToCompress
from job_orchestration.scheduler.scheduler_data import CompressionTaskResult

# Setup logging
logger = get_task_logger(__name__)


def update_compression_task_metadata(db_cursor, task_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f"""
    UPDATE {COMPRESSION_TASKS_TABLE_NAME}
    SET {", ".join(field_set_expressions)}
    WHERE id={task_id}
    """
    db_cursor.execute(query)


def increment_compression_job_metadata(db_cursor, job_id, kv):
    if not len(kv):
        logger.error("Must specify at least one field to update")
        raise ValueError

    field_set_expressions = [f"{k}={k}+{v}" for k, v in kv.items()]
    query = f"""
    UPDATE {COMPRESSION_JOBS_TABLE_NAME}
    SET {", ".join(field_set_expressions)}
    WHERE id={job_id}
    """
    db_cursor.execute(query)


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
        "--target-encoded-size",
        str(clp_config.output.target_segment_size + clp_config.output.target_dictionaries_size),
        "--db-config-file", str(db_config_file_path),
    ]
    # fmt: on

    if clp_config.input.timestamp_key is not None:
        compression_cmd.append("--timestamp-key")
        compression_cmd.append(clp_config.input.timestamp_key)

    return compression_cmd


def update_tags(db_conn, db_cursor, table_prefix, archive_id, tag_ids):
    db_cursor.executemany(
        f"INSERT INTO {table_prefix}archive_tags (archive_id, tag_id) VALUES (%s, %s)",
        [(archive_id, tag_id) for tag_id in tag_ids],
    )
    db_conn.commit()


def run_clp(
    clp_config: ClpIoConfig,
    clp_home: pathlib.Path,
    data_dir: pathlib.Path,
    archive_output_dir: pathlib.Path,
    logs_dir: pathlib.Path,
    job_id: int,
    task_id: int,
    tag_ids,
    paths_to_compress: PathsToCompress,
    db_conn,
    db_cursor,
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
    :param tag_ids:
    :param paths_to_compress: PathToCompress
    :param db_conn:
    :param db_cursor:
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
    last_archive_id = ""
    total_uncompressed_size = 0
    total_compressed_size = 0
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        stats = json.loads(line.decode("ascii"))
        if stats["id"] != last_archive_id:
            total_uncompressed_size += stats["uncompressed_size"]
            total_compressed_size += stats["size"]
            if tag_ids is not None:
                update_tags(
                    db_conn,
                    db_cursor,
                    clp_metadata_db_connection_config["table_prefix"],
                    stats["id"],
                    tag_ids,
                )
            increment_compression_job_metadata(
                db_cursor,
                job_id,
                dict(
                    uncompressed_size=stats["uncompressed_size"],
                    compressed_size=stats["size"],
                ),
            )
            db_conn.commit()

        last_archive_id = stats["id"]

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

    worker_output = {
        "total_uncompressed_size": total_uncompressed_size,
        "total_compressed_size": total_compressed_size,
    }
    if compression_successful:
        return CompressionTaskStatus.SUCCEEDED, worker_output
    else:
        worker_output["error_message"] = f"See logs {stderr_log_path}"
        return CompressionTaskStatus.FAILED, worker_output


@app.task(bind=True)
def compress(
    self: Task,
    job_id: int,
    task_id: int,
    tag_ids,
    clp_io_config_json: str,
    paths_to_compress_json: str,
    clp_metadata_db_connection_config,
):
    clp_home_str = os.getenv("CLP_HOME")
    data_dir_str = os.getenv("CLP_DATA_DIR")
    archive_output_dir_str = os.getenv("CLP_ARCHIVE_OUTPUT_DIR")
    logs_dir_str = os.getenv("CLP_LOGS_DIR")

    # Set logging level
    clp_logging_level = str(os.getenv("CLP_LOGGING_LEVEL"))
    set_logging_level(logger, clp_logging_level)

    clp_io_config = ClpIoConfig.parse_raw(clp_io_config_json)
    paths_to_compress = PathsToCompress.parse_raw(paths_to_compress_json)

    sql_adapter = SQL_Adapter(Database.parse_obj(clp_metadata_db_connection_config))

    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        start_time = datetime.datetime.now()
        logger.info(f"[job_id={job_id} task_id={task_id}] COMPRESSION STARTED.")
        compression_task_status, worker_output = run_clp(
            clp_io_config,
            pathlib.Path(clp_home_str),
            pathlib.Path(data_dir_str),
            pathlib.Path(archive_output_dir_str),
            pathlib.Path(logs_dir_str),
            job_id,
            task_id,
            tag_ids,
            paths_to_compress,
            db_conn,
            db_cursor,
            clp_metadata_db_connection_config,
        )
        duration = (datetime.datetime.now() - start_time).total_seconds()
        logger.info(f"[job_id={job_id} task_id={task_id}] COMPRESSION COMPLETED.")

        update_compression_task_metadata(
            db_cursor,
            task_id,
            dict(
                start_time=start_time,
                status=compression_task_status,
                partition_uncompressed_size=worker_output["total_uncompressed_size"],
                partition_compressed_size=worker_output["total_compressed_size"],
                duration=duration,
            ),
        )
        db_conn.commit()

        compression_task_result = CompressionTaskResult(
            task_id=task_id,
            status=compression_task_status,
            duration=duration,
        )

        if CompressionTaskStatus.SUCCEEDED == compression_task_status:
            increment_compression_job_metadata(db_cursor, job_id, dict(num_tasks_completed=1))
            db_conn.commit()
        else:
            compression_task_result.error_message = worker_output["error_message"]

        return compression_task_result.dict()
