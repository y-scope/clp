import datetime
import json
import os
import pathlib
import shutil
import subprocess
from contextlib import closing
from typing import Any

from clp_py_utils.clp_config import (
    CLP_DB_PASS_ENV_VAR_NAME,
    CLP_DB_USER_ENV_VAR_NAME,
    ClpDbNameType,
    ClpDbUserType,
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_TASKS_TABLE_NAME,
    Database,
    S3Config,
    StorageEngine,
    StorageType,
    WorkerConfig,
)
from clp_py_utils.clp_logging import set_logging_level
from clp_py_utils.clp_metadata_db_utils import (
    get_archives_table_name,
)
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.s3_utils import (
    generate_s3_url,
    get_credential_env_vars,
    s3_put,
)
from clp_py_utils.sql_adapter import SqlAdapter

from job_orchestration.scheduler.constants import CompressionTaskStatus
from job_orchestration.scheduler.job_config import (
    ClpIoConfig,
    InputType,
    PathsToCompress,
    S3InputConfig,
)
from job_orchestration.scheduler.task_result import CompressionTaskResult


def update_compression_task_metadata(db_cursor, task_id, kv):
    if not len(kv):
        raise ValueError("Must specify at least one field to update")

    field_set_expressions = [f'{k}="{v}"' for k, v in kv.items()]
    query = f"""
    UPDATE {COMPRESSION_TASKS_TABLE_NAME}
    SET {", ".join(field_set_expressions)}
    WHERE id={task_id}
    """
    db_cursor.execute(query)


def increment_compression_job_metadata(db_cursor, job_id, kv):
    if not len(kv):
        raise ValueError("Must specify at least one field to update")

    field_set_expressions = [f"{k}={k}+{v}" for k, v in kv.items()]
    query = f"""
    UPDATE {COMPRESSION_JOBS_TABLE_NAME}
    SET {", ".join(field_set_expressions)}
    WHERE id={job_id}
    """
    db_cursor.execute(query)


def update_job_metadata(
    db_cursor,
    job_id: int,
    archive_stats: dict[str, Any],
) -> None:
    increment_compression_job_metadata(
        db_cursor,
        job_id,
        dict(
            uncompressed_size=archive_stats["uncompressed_size"],
            compressed_size=archive_stats["size"],
        ),
    )


def update_archive_metadata(
    db_cursor,
    table_prefix: str,
    dataset: str | None,
    archive_stats: dict[str, Any],
) -> None:
    stats_to_update = {
        # Use defaults for values clp-s doesn't output
        "creation_ix": 0,
        "creator_id": "",
    }

    # Validate clp-s doesn't output the set kv-pairs
    for key in stats_to_update:
        if key in archive_stats:
            raise ValueError(f"Unexpected key '{key}' in archive stats")

    required_stat_names = [
        "begin_timestamp",
        "end_timestamp",
        "id",
        "size",
        "uncompressed_size",
    ]
    for stat_name in required_stat_names:
        stats_to_update[stat_name] = archive_stats[stat_name]

    keys = ", ".join(stats_to_update.keys())
    value_placeholders = ", ".join(["%s"] * len(stats_to_update))
    archives_table_name = get_archives_table_name(table_prefix, dataset)
    query = f"INSERT INTO {archives_table_name} ({keys}) VALUES ({value_placeholders})"
    db_cursor.execute(query, list(stats_to_update.values()))


def _generate_fs_logs_list(
    output_file_path: pathlib.Path,
    paths_to_compress: PathsToCompress,
) -> None:
    file_paths = paths_to_compress.file_paths
    empty_directories = paths_to_compress.empty_directories
    with open(output_file_path, "w") as file:
        for path_str in file_paths:
            file.write(path_str)
            file.write("\n")
        if empty_directories and len(empty_directories) > 0:
            # Prepare list of paths to compress for clp
            for path_str in empty_directories:
                file.write(path_str)
                file.write("\n")


def _generate_s3_logs_list(
    output_file_path: pathlib.Path,
    paths_to_compress: PathsToCompress,
    s3_input_config: S3InputConfig,
) -> None:
    # S3 object keys are stored as file_paths in `PathsToCompress`
    object_keys = paths_to_compress.file_paths
    with open(output_file_path, "w") as file:
        for object_key in object_keys:
            s3_url = generate_s3_url(
                s3_input_config.endpoint_url,
                s3_input_config.region_code,
                s3_input_config.bucket,
                object_key,
            )
            file.write(s3_url)
            file.write("\n")


def _upload_archive_to_s3(
    s3_config: S3Config,
    archive_src_path: pathlib.Path,
    archive_id: str,
    dataset: str | None,
):
    dest_path = f"{dataset}/{archive_id}" if dataset is not None else archive_id
    s3_put(s3_config, archive_src_path, dest_path)


def _get_db_connection_args_for_clp_cmd(
    clp_metadata_db_connection_config: dict[str, Any],
) -> list[str]:
    """
    :param clp_metadata_db_connection_config:
    :return: List of database connection arguments for the clp command.
    """
    return [
        "--db-type",
        clp_metadata_db_connection_config["type"],
        "--db-host",
        clp_metadata_db_connection_config["host"],
        "--db-port",
        str(clp_metadata_db_connection_config["port"]),
        "--db-name",
        clp_metadata_db_connection_config["names"][ClpDbNameType.CLP],
        "--db-table-prefix",
        clp_metadata_db_connection_config["table_prefix"],
    ]


def _get_db_connection_env_vars_for_clp_cmd(
    clp_metadata_db_connection_config: dict[str, Any],
) -> dict[str, str]:
    """
    :param clp_metadata_db_connection_config:
    :return: Dictionary of database connection environment variables for the clp command.
    """
    credentials = Database.model_validate(clp_metadata_db_connection_config).credentials
    return {
        CLP_DB_USER_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].username,
        CLP_DB_PASS_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].password,
    }


def _make_clp_command_and_env(
    clp_home: pathlib.Path,
    archive_output_dir: pathlib.Path,
    clp_config: ClpIoConfig,
    clp_metadata_db_connection_config: dict[str, Any],
) -> tuple[list[str], dict[str, str] | None]:
    """
    Generates the command and environment variables for a clp compression job.
    :param clp_home:
    :param archive_output_dir:
    :param clp_config:
    :param clp_metadata_db_connection_config:
    :return: Tuple of (compression_command, compression_env_vars)
    """
    path_prefix_to_remove = clp_config.input.path_prefix_to_remove

    # fmt: off
    compression_cmd = [
        str(clp_home / "bin" / "clp"),
        "c", str(archive_output_dir),
        "--print-archive-stats-progress",
        "--target-dictionaries-size", str(clp_config.output.target_dictionaries_size),
        "--target-segment-size", str(clp_config.output.target_segment_size),
        "--target-encoded-file-size", str(clp_config.output.target_encoded_file_size),
        "--compression-level", str(clp_config.output.compression_level),
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

    # Set database connection parameters
    compression_cmd.extend(_get_db_connection_args_for_clp_cmd(clp_metadata_db_connection_config))
    compression_env_vars = dict(os.environ)
    compression_env_vars.update(
        _get_db_connection_env_vars_for_clp_cmd(clp_metadata_db_connection_config)
    )

    return compression_cmd, compression_env_vars


def _make_clp_s_command_and_env(
    clp_home: pathlib.Path,
    archive_output_dir: pathlib.Path,
    clp_config: ClpIoConfig,
    use_single_file_archive: bool,
) -> tuple[list[str], dict[str, str]]:
    """
    Generates the command and environment variables for a clp_s compression job.
    :param clp_home:
    :param archive_output_dir:
    :param clp_config:
    :param use_single_file_archive:
    :return: Tuple of (compression_command, compression_env_vars)
    """
    # fmt: off
    compression_cmd = [
        str(clp_home / "bin" / "clp-s"),
        "c", str(archive_output_dir),
        "--print-archive-stats",
        "--target-encoded-size",
        str(clp_config.output.target_segment_size + clp_config.output.target_dictionaries_size),
        "--compression-level", str(clp_config.output.compression_level),
    ]
    # fmt: on

    compression_env_vars = dict(os.environ)
    if InputType.S3 == clp_config.input.type and not clp_config.input.unstructured:
        compression_env_vars.update(get_credential_env_vars(clp_config.input.aws_authentication))
        compression_cmd.append("--auth")
        compression_cmd.append("s3")

    if use_single_file_archive:
        compression_cmd.append("--single-file-archive")

    if clp_config.input.unstructured:
        compression_cmd.append("--timestamp-key")
        compression_cmd.append("timestamp")
    elif clp_config.input.timestamp_key is not None:
        compression_cmd.append("--timestamp-key")
        compression_cmd.append(clp_config.input.timestamp_key)

    return compression_cmd, compression_env_vars


def _make_log_converter_command_and_env(
    clp_home: pathlib.Path,
    conversion_output_dir: pathlib.Path,
    clp_config: ClpIoConfig,
) -> tuple[list[str], dict[str, str]]:
    """
    Generates the command and environment variables for unstructured text log conversion.

    :param clp_home:
    :param conversion_output_dir:
    :param clp_config:
    :return: A tuple containing:
        - The conversion command.
        - The conversion environment variables.
    """
    # fmt: off
    conversion_cmd = [
        str(clp_home / "bin" / "log-converter"),
        "--output-dir",
        str(conversion_output_dir),
    ]
    # fmt: on

    conversion_env_vars = dict(os.environ)
    if InputType.S3 == clp_config.input.type:
        conversion_env_vars.update(get_credential_env_vars(clp_config.input.aws_authentication))
        conversion_cmd.append("--auth")
        conversion_cmd.append("s3")

    return conversion_cmd, conversion_env_vars


def run_clp(
    worker_config: WorkerConfig,
    clp_config: ClpIoConfig,
    clp_home: pathlib.Path,
    logs_dir: pathlib.Path,
    job_id: int,
    task_id: int,
    paths_to_compress: PathsToCompress,
    sql_adapter: SqlAdapter,
    clp_metadata_db_connection_config,
    logger,
):
    """
    Compresses logs into archives.

    :param worker_config: WorkerConfig
    :param clp_config: ClpIoConfig
    :param clp_home:
    :param logs_dir:
    :param job_id:
    :param task_id:
    :param paths_to_compress: PathToCompress
    :param sql_adapter: SqlAdapter
    :param clp_metadata_db_connection_config
    :param logger
    :return: tuple -- (whether compression was successful, output messages)
    """
    instance_id_str = f"compression-job-{job_id}-task-{task_id}"

    clp_storage_engine = worker_config.package.storage_engine
    tmp_dir = worker_config.tmp_directory
    archive_output_dir = worker_config.archive_output.get_directory()

    # Get S3 config
    s3_config: S3Config
    enable_s3_write = False
    storage_type = worker_config.archive_output.storage.type
    if StorageType.S3 == storage_type:
        if StorageEngine.CLP_S != clp_storage_engine:
            error_msg = f"S3 storage is not supported for storage engine: {clp_storage_engine}."
            logger.error(error_msg)
            return False, {"error_message": error_msg}

        s3_config = worker_config.archive_output.storage.s3_config
        enable_s3_write = True

    dataset = clp_config.input.dataset
    if StorageEngine.CLP == clp_storage_engine:
        compression_cmd, compression_env = _make_clp_command_and_env(
            clp_home=clp_home,
            archive_output_dir=archive_output_dir,
            clp_config=clp_config,
            clp_metadata_db_connection_config=clp_metadata_db_connection_config,
        )
    elif StorageEngine.CLP_S == clp_storage_engine:
        archive_output_dir = archive_output_dir / dataset
        compression_cmd, compression_env = _make_clp_s_command_and_env(
            clp_home=clp_home,
            archive_output_dir=archive_output_dir,
            clp_config=clp_config,
            use_single_file_archive=enable_s3_write,
        )
    else:
        logger.error(f"Unsupported storage engine {clp_storage_engine}")
        return False, {"error_message": f"Unsupported storage engine {clp_storage_engine}"}

    # Generate list of logs to compress
    input_type = clp_config.input.type
    logs_list_path = tmp_dir / f"{instance_id_str}-log-paths.txt"
    if InputType.FS == input_type:
        _generate_fs_logs_list(logs_list_path, paths_to_compress)
    elif InputType.S3 == input_type:
        _generate_s3_logs_list(logs_list_path, paths_to_compress, clp_config.input)
    else:
        error_msg = f"Unsupported input type: {input_type}."
        logger.error(error_msg)
        return False, {"error_message": error_msg}

    conversion_cmd = None
    converted_inputs_dir = None
    if StorageEngine.CLP_S == clp_storage_engine and clp_config.input.unstructured:
        converted_inputs_dir = tmp_dir / f"{instance_id_str}-converted-tmp"
        converted_inputs_dir.mkdir()
        conversion_cmd, conversion_env = _make_log_converter_command_and_env(
            clp_home=clp_home, conversion_output_dir=converted_inputs_dir, clp_config=clp_config
        )
        conversion_cmd.append("--inputs-from")
        conversion_cmd.append(str(logs_list_path))
        compression_cmd.append(str(converted_inputs_dir))
    else:
        compression_cmd.append("--files-from")
        compression_cmd.append(str(logs_list_path))

    def cleanup_temporary_files():
        if logs_list_path is not None:
            logs_list_path.unlink()
        if converted_inputs_dir is not None:
            shutil.rmtree(converted_inputs_dir)

    # Open stderr log file
    stderr_log_path = logs_dir / f"{instance_id_str}-stderr.log"
    stderr_log_file = open(stderr_log_path, "w")

    conversion_return_code = 0
    if conversion_cmd is not None:
        logger.debug("Execute log-converter with command: %s", conversion_cmd)
        conversion_proc = subprocess.Popen(
            conversion_cmd, stdout=subprocess.DEVNULL, stderr=stderr_log_file, env=conversion_env
        )
        conversion_return_code = conversion_proc.wait()

    if conversion_return_code != 0:
        cleanup_temporary_files()
        logger.error(
            f"Failed to convert unstructured log text, return_code={conversion_return_code}"
        )
        worker_output = {
            "total_uncompressed_size": 0,
            "total_compressed_size": 0,
            "error_message": f"Check logs in {stderr_log_path}",
        }
        stderr_log_file.close()
        return CompressionTaskStatus.FAILED, worker_output

    # Start compression
    logger.debug("Compressing...")
    compression_successful = False
    proc = subprocess.Popen(
        compression_cmd, stdout=subprocess.PIPE, stderr=stderr_log_file, env=compression_env
    )

    # Compute the total amount of data compressed
    last_archive_stats = None
    last_line_decoded = False
    total_uncompressed_size = 0
    total_compressed_size = 0

    # Handle job metadata update and S3 write if enabled
    s3_error = None
    while not last_line_decoded:
        stats: dict[str, Any] | None = None

        line = proc.stdout.readline()
        if not line:
            last_line_decoded = True
        else:
            stats = json.loads(line.decode("utf-8"))

        if last_archive_stats is not None and (
            None is stats or stats["id"] != last_archive_stats["id"]
        ):
            archive_id = last_archive_stats["id"]
            archive_path = archive_output_dir / archive_id
            if enable_s3_write:
                if s3_error is None:
                    logger.info(f"Uploading archive {archive_id} to S3...")
                    try:
                        _upload_archive_to_s3(s3_config, archive_path, archive_id, dataset)
                        logger.info(f"Finished uploading archive {archive_id} to S3.")
                    except Exception as err:
                        logger.exception(f"Failed to upload archive {archive_id}")
                        s3_error = str(err)
                        # NOTE: It's possible `proc` finishes before we call `terminate` on it, in
                        # which case the process will still return success.
                        proc.terminate()

            if s3_error is None:
                # We've started a new archive so add the previous archive's last reported size to
                # the total
                total_uncompressed_size += last_archive_stats["uncompressed_size"]
                total_compressed_size += last_archive_stats["size"]
                with (
                    closing(sql_adapter.create_connection(True)) as db_conn,
                    closing(db_conn.cursor(dictionary=True)) as db_cursor,
                ):
                    table_prefix = clp_metadata_db_connection_config["table_prefix"]
                    if StorageEngine.CLP_S == clp_storage_engine:
                        update_archive_metadata(
                            db_cursor, table_prefix, dataset, last_archive_stats
                        )
                    update_job_metadata(
                        db_cursor,
                        job_id,
                        last_archive_stats,
                    )
                    db_conn.commit()

                if StorageEngine.CLP_S == clp_storage_engine:
                    indexer_cmd = [
                        str(clp_home / "bin" / "indexer"),
                        *_get_db_connection_args_for_clp_cmd(clp_metadata_db_connection_config),
                        dataset,
                        archive_path,
                    ]

                    # Set environment variables for database credentials
                    indexer_env = dict(os.environ)
                    indexer_env.update(
                        _get_db_connection_env_vars_for_clp_cmd(clp_metadata_db_connection_config)
                    )

                    try:
                        subprocess.run(
                            indexer_cmd,
                            stdout=subprocess.DEVNULL,
                            stderr=stderr_log_file,
                            check=True,
                            env=indexer_env,
                        )
                    except subprocess.CalledProcessError:
                        logger.exception("Failed to index archive.")

            if enable_s3_write:
                archive_path.unlink()

        last_archive_stats = stats

    # Wait for compression to finish
    return_code = proc.wait()

    if 0 != return_code:
        logger.error(f"Failed to compress, return_code={return_code!s}")
    else:
        compression_successful = True
        cleanup_temporary_files()

    logger.debug("Compressed.")

    # Close stderr log file
    stderr_log_file.close()

    worker_output = {
        "total_uncompressed_size": total_uncompressed_size,
        "total_compressed_size": total_compressed_size,
    }

    if compression_successful and s3_error is None:
        return CompressionTaskStatus.SUCCEEDED, worker_output
    error_msgs = []
    if compression_successful is False:
        error_msgs.append(f"See logs {stderr_log_path}")
    if s3_error is not None:
        error_msgs.append(s3_error)
    worker_output["error_message"] = "\n".join(error_msgs)
    return CompressionTaskStatus.FAILED, worker_output


def compression_entry_point(
    job_id: int,
    task_id: int,
    clp_io_config_json: str,
    paths_to_compress_json: str,
    clp_metadata_db_connection_config: dict[str, Any],
    logger,
):
    clp_home = pathlib.Path(os.getenv("CLP_HOME"))

    # Set logging level
    logs_dir = pathlib.Path(os.getenv("CLP_LOGS_DIR"))
    clp_logging_level = str(os.getenv("CLP_LOGGING_LEVEL"))
    set_logging_level(logger, clp_logging_level)

    # Load configuration
    try:
        worker_config = WorkerConfig.model_validate(
            read_yaml_config_file(pathlib.Path(os.getenv("CLP_CONFIG_PATH")))
        )
    except Exception:
        error_msg = "Failed to load worker config"
        logger.exception(error_msg)
        return CompressionTaskResult(
            task_id=task_id,
            status=CompressionTaskStatus.FAILED,
            duration=0,
            error_message=error_msg,
        ).model_dump()

    clp_io_config = ClpIoConfig.model_validate_json(clp_io_config_json)
    paths_to_compress = PathsToCompress.model_validate_json(paths_to_compress_json)

    sql_adapter = SqlAdapter(Database.model_validate(clp_metadata_db_connection_config))

    start_time = datetime.datetime.now()
    logger.info(f"[job_id={job_id} task_id={task_id}] COMPRESSION STARTED.")
    compression_task_status, worker_output = run_clp(
        worker_config,
        clp_io_config,
        clp_home,
        logs_dir,
        job_id,
        task_id,
        paths_to_compress,
        sql_adapter,
        clp_metadata_db_connection_config,
        logger,
    )
    duration = (datetime.datetime.now() - start_time).total_seconds()
    logger.info(f"[job_id={job_id} task_id={task_id}] COMPRESSION COMPLETED.")

    with (
        closing(sql_adapter.create_connection(True)) as db_conn,
        closing(db_conn.cursor(dictionary=True)) as db_cursor,
    ):
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
        if CompressionTaskStatus.SUCCEEDED == compression_task_status:
            increment_compression_job_metadata(db_cursor, job_id, dict(num_tasks_completed=1))
        db_conn.commit()

    compression_task_result = CompressionTaskResult(
        task_id=task_id,
        status=compression_task_status,
        duration=duration,
    )

    if CompressionTaskStatus.FAILED == compression_task_status:
        compression_task_result.error_message = worker_output["error_message"]

    return compression_task_result.model_dump()
