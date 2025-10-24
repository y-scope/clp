import argparse
import datetime
import logging
import os
import pathlib
import sys
import time
from contextlib import closing
from typing import List, Tuple, Union

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
)

logger = logging.getLogger(__file__)


def print_compression_job_status(job_row):
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
        f"Compressed {pretty_size(job_uncompressed_size)} into "
        f"{pretty_size(job_compressed_size)} ({compression_ratio:.2f}x). "
        f"Speed: {pretty_size(compression_speed)}/s."
    )


def handle_job_update(db, db_cursor, job_id, no_progress_reporting):
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
            raise Exception(f"Job with id={job_id} not found in database")

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
            logger.error(f"Compression failed. {job_row['status_msg']}")
            break  # Done
        if CompressionJobStatus.KILLED == job_status:
            # The job is killed
            logger.error(f"Compression killed. {job_row['status_msg']}")
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


def handle_job(sql_adapter: SQL_Adapter, clp_io_config: ClpIoConfig, no_progress_reporting: bool):
    with closing(sql_adapter.create_connection(True)) as db, closing(
        db.cursor(dictionary=True)
    ) as db_cursor:
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
            logger.info(f"Compression job {job_id} submitted.")

            handle_job_update(db, db_cursor, job_id, no_progress_reporting)
        except Exception as ex:
            logger.error(ex)
            return CompressionJobCompletionStatus.FAILED

        logger.debug(f"Finished job {job_id}")

        return CompressionJobCompletionStatus.SUCCEEDED


def _generate_clp_io_config(
    clp_config: CLPConfig,
    logs_to_compress: List[str],
    parsed_args: argparse.Namespace,
) -> Union[S3InputConfig, FsInputConfig]:
    source = parsed_args.source

    if source == "fs":
        if len(logs_to_compress) == 0:
            raise ValueError("No input paths given.")
        return FsInputConfig(
            dataset=parsed_args.dataset,
            paths_to_compress=logs_to_compress,
            timestamp_key=parsed_args.timestamp_key,
            path_prefix_to_remove=str(CONTAINER_INPUT_LOGS_ROOT_DIR),
        )
    elif source == "s3":
        if len(logs_to_compress) == 0:
            raise ValueError("No URLs given.")

        aws_authentication = _get_aws_authentication_from_config(clp_config)

        first_item = logs_to_compress[0]
        if first_item in ("s3-object", "s3-key-prefix"):
            subcommand = first_item
            urls = logs_to_compress[1:]

            if subcommand == "s3-object":
                region_code, bucket, key_prefix, keys = _parse_and_validate_s3_object_urls(urls)
                return S3InputConfig(
                    dataset=parsed_args.dataset,
                    region_code=region_code,
                    bucket=bucket,
                    key_prefix=key_prefix,
                    keys=keys,
                    aws_authentication=aws_authentication,
                    timestamp_key=parsed_args.timestamp_key,
                )
            elif subcommand == "s3-key-prefix":
                if len(urls) != 1:
                    raise ValueError(f"s3-key-prefix requires exactly one URL, got {len(urls)}")
                region_code, bucket, key_prefix = _parse_s3_key_prefix_url(urls[0])
                return S3InputConfig(
                    dataset=parsed_args.dataset,
                    region_code=region_code,
                    bucket=bucket,
                    key_prefix=key_prefix,
                    keys=None,
                    aws_authentication=aws_authentication,
                    timestamp_key=parsed_args.timestamp_key,
                )
    else:
        raise ValueError(f"Unsupported source type: {source}")


def _get_logs_to_compress(logs_list_path: pathlib.Path) -> List[str]:
    """
    Read logs or URLs from the input file.

    :param logs_list_path: Path to the list file.
    :return: List of paths/URLs.
    """
    logs_to_compress = []
    with open(logs_list_path, "r") as f:
        for line in f:
            stripped_line = line.strip()
            if "" == stripped_line:
                # Skip empty lines
                continue
            logs_to_compress.append(stripped_line)

    return logs_to_compress


def _parse_and_validate_s3_object_urls(
    urls: List[str],
) -> Tuple[str, str, str, Union[List[str], None]]:
    """
    Parse and validate S3 object URLs.

    Validates:
    - All URLs have same region and bucket.
    - No duplicate keys.
    - For multiple URLs: non-empty common prefix exists.

    :param urls: List of S3 URLs
    :return: Tuple of (region_code, bucket, key_prefix, keys).
             - For single URL: key_prefix is the full key, keys is None (more efficient).
             - For multiple URLs: key_prefix is the common prefix, keys are full keys.
    :raises ValueError: If validation fails.
    """
    if len(urls) == 0:
        raise ValueError("No URLs provided")

    parsed_urls = []
    for url in urls:
        try:
            region_code, bucket, key = parse_s3_url(url)
            parsed_urls.append((region_code, bucket, key))
        except ValueError as e:
            raise ValueError(f"Failed to parse URL '{url}': {e}")

    first_region, first_bucket, _ = parsed_urls[0]
    for region, bucket, _ in parsed_urls:
        if region != first_region:
            raise ValueError(
                f"All URLs must have same region code. Found '{first_region}' and '{region}'"
            )
        if bucket != first_bucket:
            raise ValueError(
                f"All URLs must have same bucket. Found '{first_bucket}' and '{bucket}'"
            )

    keys = [key for _, _, key in parsed_urls]

    if len(keys) != len(set(keys)):
        raise ValueError("Duplicate keys found in URLs")

    # For a single key, use the full key as prefix with keys=None
    # For multiple keys, find common prefix and provide explicit keys list
    if len(keys) == 1:
        key_prefix = keys[0]
        keys = None
    else:
        key_prefix = os.path.commonprefix(keys)
        if not key_prefix:
            raise ValueError("No common prefix found among object keys")

    return first_region, first_bucket, key_prefix, keys


def _parse_s3_key_prefix_url(url: str) -> Tuple[str, str, str]:
    """
    Parse single S3 URL for key-prefix mode.

    :param url: S3 URL.
    :return: Tuple of (region_code, bucket, key_prefix).
    :raise ValueError: If parsing fails.
    """
    try:
        region_code, bucket, key_prefix = parse_s3_url(url)
    except ValueError as e:
        raise ValueError(f"Failed to parse S3 URL: {e}")

    return region_code, bucket, key_prefix


def _get_aws_authentication_from_config(clp_config: CLPConfig) -> AwsAuthentication:
    """
    Get AWS authentication configuration.

    :param clp_config: CLPConfig object.
    :return: AwsAuthentication object.
    :raise ValueError: If no authentication configured.
    """
    if hasattr(clp_config, "logs_input") and hasattr(clp_config.logs_input, "type"):
        if StorageType.S3 == clp_config.logs_input.type:
            return clp_config.logs_input.aws_authentication

    raise ValueError("No AWS authentication configured for logs_input.")


def main(argv):
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
        "--source",
        type=str,
        choices=["fs", "s3"],
        default="fs",
        help="Source type: 'fs' for filesystem paths, 's3' for S3 URLs.",
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
    except:
        logger.exception("Failed to load config.")
        return -1

    comp_jobs_dir = clp_config.logs_directory / "comp-jobs"
    comp_jobs_dir.mkdir(parents=True, exist_ok=True)

    try:
        logs_to_compress = _get_logs_to_compress(pathlib.Path(parsed_args.logs_list).resolve())
        clp_input_config = _generate_clp_io_config(clp_config, logs_to_compress, parsed_args)
    except Exception:
        logger.exception(f"Failed to process input.")
        return -1

    clp_output_config = OutputConfig.model_validate(clp_config.archive_output.model_dump())
    if parsed_args.tags:
        tag_list = [tag.strip().lower() for tag in parsed_args.tags.split(",") if tag]
        if len(tag_list) > 0:
            clp_output_config.tags = tag_list
    clp_io_config = ClpIoConfig(input=clp_input_config, output=clp_output_config)

    mysql_adapter = SQL_Adapter(clp_config.database)
    return handle_job(
        sql_adapter=mysql_adapter,
        clp_io_config=clp_io_config,
        no_progress_reporting=parsed_args.no_progress_reporting,
    )


if "__main__" == __name__:
    sys.exit(main(sys.argv))
