import argparse
import datetime
import logging
import os
import pathlib
import sys
import time
from contextlib import closing
from typing import Any

import brotli
import msgpack
from clp_py_utils.clp_config import (
    AwsAuthentication,
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLPConfig,
    COMPRESSION_JOBS_TABLE_NAME,
    StorageType,
)
from clp_py_utils.pretty_size import pretty_size
from clp_py_utils.s3_utils import parse_s3_url
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.constants import (
    CompressionJobCompletionStatus,
    CompressionJobStatus,
)
from job_orchestration.scheduler.job_config import (
    ClpIoConfig,
    FsInputConfig,
    OutputConfig,
    S3InputConfig,
)

from clp_package_utils.general import (
    CONTAINER_INPUT_LOGS_ROOT_DIR,
    get_clp_home,
    load_config_file,
    S3_KEY_PREFIX_COMPRESSION,
    S3_OBJECT_COMPRESSION,
)

logger = logging.getLogger(__name__)


def print_compression_job_status(job_row: dict[str, Any]) -> None:
    job_uncompressed_size = job_row["uncompressed_size"]
    job_compressed_size = job_row["compressed_size"]
    compression_ratio = float(job_uncompressed_size) / job_compressed_size
    if CompressionJobStatus.SUCCEEDED == job_row["status"]:
        compression_speed = job_uncompressed_size / job_row["duration"]
    else:
        compression_speed = (
            job_uncompressed_size
            / (datetime.datetime.now() - job_row["start_time"]).total_seconds()
        )
    logger.info(
        "Compressed %s into %s (%.2fx). Speed: %s/s.",
        pretty_size(job_uncompressed_size),
        pretty_size(job_compressed_size),
        compression_ratio,
        pretty_size(compression_speed),
    )


def handle_job_update(db: Any, db_cursor: Any, job_id: int, no_progress_reporting: bool) -> None:
    if no_progress_reporting:
        polling_query = (
            f"SELECT status, status_msg FROM {COMPRESSION_JOBS_TABLE_NAME} WHERE id={job_id}"
        )
    else:
        polling_query = (
            f"SELECT start_time, status, status_msg, uncompressed_size, compressed_size, duration "
            f"FROM {COMPRESSION_JOBS_TABLE_NAME} WHERE id={job_id}"
        )

    job_last_uncompressed_size = 0

    while True:
        db_cursor.execute(polling_query)
        results = db_cursor.fetchall()
        db.commit()
        if len(results) > 1:
            logging.error("Duplicated job_id")
        if len(results) == 0:
            msg = f"Job with id={job_id} not found in database"
            raise RuntimeError(msg)

        job_row = results[0]
        job_status = job_row["status"]

        if CompressionJobStatus.SUCCEEDED == job_status:
            # All tasks in the job is done
            if not no_progress_reporting:
                logger.info("Compression finished.")
                print_compression_job_status(job_row)
            break  # Done
        if CompressionJobStatus.FAILED == job_status:
            # One or more tasks in the job has failed
            logger.error("Compression failed. %s", job_row["status_msg"])
            break  # Done
        if CompressionJobStatus.KILLED == job_status:
            # The job is killed
            logger.error("Compression killed. %s", job_row["status_msg"])
            break  # Done

        if CompressionJobStatus.RUNNING == job_status:
            if not no_progress_reporting:
                job_uncompressed_size = job_row["uncompressed_size"]
                if job_last_uncompressed_size < job_uncompressed_size:
                    print_compression_job_status(job_row)
                    job_last_uncompressed_size = job_uncompressed_size
        elif CompressionJobStatus.PENDING == job_status:
            pass  # Simply wait another iteration
        else:
            error_msg = f"Unhandled CompressionJobStatus: {job_status}"
            raise NotImplementedError(error_msg)

        time.sleep(0.5)


def handle_job(
    sql_adapter: SQL_Adapter, clp_io_config: ClpIoConfig, no_progress_reporting: bool
) -> CompressionJobCompletionStatus:
    with (
        closing(sql_adapter.create_connection(True)) as db,
        closing(db.cursor(dictionary=True)) as db_cursor,
    ):
        try:
            compressed_clp_io_config = brotli.compress(
                msgpack.packb(clp_io_config.model_dump(exclude_none=True, exclude_unset=True)),
                quality=4,
            )
            db_cursor.execute(
                f"INSERT INTO {COMPRESSION_JOBS_TABLE_NAME} (clp_config) VALUES (%s)",
                (compressed_clp_io_config,),
            )
            db.commit()
            job_id = db_cursor.lastrowid
            logger.info("Compression job %s submitted.", job_id)

            handle_job_update(db, db_cursor, job_id, no_progress_reporting)
        except Exception:
            logger.exception("Failed to handle compression job.")
            return CompressionJobCompletionStatus.FAILED

        logger.debug("Finished job %s", job_id)

        return CompressionJobCompletionStatus.SUCCEEDED


def _generate_clp_io_config(
    clp_config: CLPConfig,
    logs_to_compress: list[str],
    parsed_args: argparse.Namespace,
) -> S3InputConfig | FsInputConfig:
    input_type = parsed_args.input_type

    if input_type == "fs":
        if len(logs_to_compress) == 0:
            msg = "No input paths given."
            raise ValueError(msg)
        return FsInputConfig(
            dataset=parsed_args.dataset,
            paths_to_compress=logs_to_compress,
            timestamp_key=parsed_args.timestamp_key,
            path_prefix_to_remove=str(CONTAINER_INPUT_LOGS_ROOT_DIR),
            unstructured=parsed_args.unstructured,
        )
    if input_type != "s3":
        msg = f"Unsupported input type: `{input_type}`."
        raise ValueError(msg)

    # Handle S3 inputs
    if len(logs_to_compress) < 2:
        msg = "No URLs given."
        raise ValueError(msg)

    aws_authentication = _get_aws_authentication_from_config(clp_config)

    s3_compress_subcommand = logs_to_compress[0]
    urls = logs_to_compress[1:]

    if s3_compress_subcommand == S3_OBJECT_COMPRESSION:
        region_code, bucket, key_prefix, keys = _parse_and_validate_s3_object_urls(urls)
        return S3InputConfig(
            dataset=parsed_args.dataset,
            region_code=region_code,
            bucket=bucket,
            key_prefix=key_prefix,
            keys=keys,
            aws_authentication=aws_authentication,
            timestamp_key=parsed_args.timestamp_key,
            unstructured=parsed_args.unstructured,
        )
    if s3_compress_subcommand == S3_KEY_PREFIX_COMPRESSION:
        if len(urls) != 1:
            msg = f"`{S3_KEY_PREFIX_COMPRESSION}` requires exactly one URL, got {len(urls)}"
            raise ValueError(msg)
        region_code, bucket, key_prefix = parse_s3_url(urls[0])
        return S3InputConfig(
            dataset=parsed_args.dataset,
            region_code=region_code,
            bucket=bucket,
            key_prefix=key_prefix,
            keys=None,
            aws_authentication=aws_authentication,
            timestamp_key=parsed_args.timestamp_key,
            unstructured=parsed_args.unstructured,
        )
    msg = f"Unsupported S3 compress subcommand: `{s3_compress_subcommand}`."
    raise ValueError(msg)


def _get_logs_to_compress(logs_list_path: pathlib.Path) -> list[str]:
    """
    Reads logs or URLs from the input file.

    :param logs_list_path:
    :return: List of paths/URLs.
    """
    logs_to_compress = []
    with logs_list_path.open("r") as f:
        for line in f:
            stripped_line = line.strip()
            if "" == stripped_line:
                # Skip empty lines
                continue
            logs_to_compress.append(stripped_line)

    return logs_to_compress


def _parse_and_validate_s3_object_urls(
    urls: list[str],
) -> tuple[str, str, str, list[str]]:
    """
    Parses and validates S3 object URLs.

    The validation will ensure:
    - All URLs have the same region and bucket.
    - No duplicate keys among the URLs.
    - The URLs share a non-empty common prefix.

    :param urls:
    :return: A tuple containing:
        - The region code.
        - The bucket.
        - The common key prefix.
        - The list of keys.
    :raises ValueError: If the validation fails.
    """
    if len(urls) == 0:
        msg = "No URLs provided."
        raise ValueError(msg)

    region_code: str | None = None
    bucket_name: str | None = None
    keys = set()

    for url in urls:
        parsed_region_code, parsed_bucket_name, key = parse_s3_url(url)

        if region_code is None:
            region_code = parsed_region_code
        elif region_code != parsed_region_code:
            msg = (
                "All S3 URLs must be in the same region."
                f" Found {region_code} and {parsed_region_code}."
            )
            raise ValueError(msg)

        if bucket_name is None:
            bucket_name = parsed_bucket_name
        elif bucket_name != parsed_bucket_name:
            msg = (
                "All S3 URLs must be in the same bucket."
                f" Found {bucket_name} and {parsed_bucket_name}."
            )
            raise ValueError(msg)

        if key in keys:
            msg = f"Duplicate S3 key found: {key}."
            raise ValueError(msg)
        keys.add(key)

    key_list: list[str] = list(keys)
    key_prefix = os.path.commonprefix(key_list)

    if len(key_prefix) == 0:
        msg = "The given S3 URLs have no common prefix."
        raise ValueError(msg)

    return region_code, bucket_name, key_prefix, key_list


def _get_aws_authentication_from_config(clp_config: CLPConfig) -> AwsAuthentication:
    """
    Gets AWS authentication configuration.

    :param clp_config:
    :return: The AWS authentication configuration extracted from the CLP config.
    :raise ValueError: If no authentication provided in `clp_config`.
    """
    if StorageType.S3 == clp_config.logs_input.type:
        return clp_config.logs_input.aws_authentication

    msg = "No AWS authentication provided in `logs_input`."
    raise ValueError(msg)


def main(argv: list[str]) -> int:
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH
    args_parser = argparse.ArgumentParser(description="Compresses logs")

    # Package-level config option
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )
    args_parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Enable debug logging.",
    )
    args_parser.add_argument(
        "--dataset",
        type=str,
        default=None,
        help="The dataset that the archives belong to.",
    )
    args_parser.add_argument(
        "--input-type",
        dest="input_type",
        type=str,
        choices=["fs", "s3"],
        default="fs",
        help="Input type: 'fs' for filesystem paths, 's3' for S3 URLs.",
    )
    args_parser.add_argument(
        "-f",
        "--logs-list",
        dest="logs_list",
        help="A list of logs to compress.",
        required=True,
    )
    args_parser.add_argument(
        "--no-progress-reporting", action="store_true", help="Disables progress reporting."
    )
    args_parser.add_argument(
        "--timestamp-key",
        help="The path (e.g. x.y) for the field containing the log event's timestamp.",
    )
    args_parser.add_argument(
        "--unstructured",
        action="store_true",
        help="Treat all inputs as unstructured text logs.",
    )
    args_parser.add_argument(
        "-t", "--tags", help="A comma-separated list of tags to apply to the compressed archives."
    )
    parsed_args = args_parser.parse_args(argv[1:])
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_input_config()
        clp_config.validate_logs_dir()
        clp_config.database.load_credentials_from_env()
    except Exception:
        logger.exception("Failed to load config.")
        return -1

    comp_jobs_dir = clp_config.logs_directory / "comp-jobs"
    comp_jobs_dir.mkdir(parents=True, exist_ok=True)

    try:
        logs_to_compress = _get_logs_to_compress(pathlib.Path(parsed_args.logs_list).resolve())
        clp_input_config = _generate_clp_io_config(clp_config, logs_to_compress, parsed_args)
    except Exception:
        logger.exception("Failed to process input.")
        return -1

    clp_output_config = OutputConfig.model_validate(clp_config.archive_output.model_dump())
    if parsed_args.tags:
        tag_list = [tag.strip().lower() for tag in parsed_args.tags.split(",") if tag]
        if len(tag_list) > 0:
            clp_output_config.tags = tag_list
    clp_io_config = ClpIoConfig(input=clp_input_config, output=clp_output_config)

    mysql_adapter = SQL_Adapter(clp_config.database)
    job_completion_status = handle_job(
        sql_adapter=mysql_adapter,
        clp_io_config=clp_io_config,
        no_progress_reporting=parsed_args.no_progress_reporting,
    )
    return int(job_completion_status)


if "__main__" == __name__:
    sys.exit(main(sys.argv))
