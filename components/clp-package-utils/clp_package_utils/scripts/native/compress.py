import argparse
import datetime
import logging
import pathlib
import sys
import time
import typing
from contextlib import closing
from typing import List

import brotli
import msgpack
from clp_py_utils.clp_config import COMPRESSION_JOBS_TABLE_NAME, S3Credentials
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
    InputType,
    OutputConfig,
    S3InputConfig,
)

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CONTAINER_INPUT_LOGS_ROOT_DIR,
    get_clp_home,
    load_config_file,
)

logger = logging.getLogger(__file__)


def print_compression_job_status(job_row, current_time):
    job_uncompressed_size = job_row["uncompressed_size"]
    job_compressed_size = job_row["compressed_size"]
    job_start_time = job_row["start_time"]
    compression_ratio = float(job_uncompressed_size) / job_compressed_size
    compression_speed = job_uncompressed_size / (current_time - job_start_time).total_seconds()
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
            f"SELECT start_time, status, status_msg, uncompressed_size, compressed_size "
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
        current_time = datetime.datetime.now()

        if CompressionJobStatus.SUCCEEDED == job_status:
            # All tasks in the job is done
            if not no_progress_reporting:
                logger.info("Compression finished.")
                print_compression_job_status(job_row, current_time)
            break  # Done
        if CompressionJobStatus.FAILED == job_status:
            # One or more tasks in the job has failed
            logger.error(f"Compression failed. {job_row['status_msg']}")
            break  # Done

        if CompressionJobStatus.RUNNING == job_status:
            if not no_progress_reporting:
                job_uncompressed_size = job_row["uncompressed_size"]
                if job_last_uncompressed_size < job_uncompressed_size:
                    print_compression_job_status(job_row, current_time)
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
                msgpack.packb(clp_io_config.dict(exclude_none=True, exclude_unset=True)), quality=4
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
    logs_to_compress: List[str], parsed_args: argparse.Namespace
) -> typing.Union[S3InputConfig, FsInputConfig]:
    input_type = parsed_args.input_type

    if InputType.FS == input_type:
        return FsInputConfig(
            paths_to_compress=logs_to_compress,
            timestamp_key=parsed_args.timestamp_key,
            path_prefix_to_remove=str(CONTAINER_INPUT_LOGS_ROOT_DIR),
        )
    elif InputType.S3 == input_type:
        if len(logs_to_compress) != 1:
            ValueError(f"Too many URLs: {len(logs_to_compress)} > 1")

        s3_url = logs_to_compress[0]
        region_code, bucket_name, key_prefix = parse_s3_url(s3_url)
        return S3InputConfig(
            region_code=region_code,
            bucket=bucket_name,
            key_prefix=key_prefix,
            credentials=S3Credentials(
                access_key_id=parsed_args.aws_access_key_id,
                secret_access_key=parsed_args.aws_secret_access_key,
            ),
            timestamp_key=parsed_args.timestamp_key,
        )
    else:
        raise ValueError(f"Unsupported input type: {input_type}")


def _get_logs_to_compress(logs_list_path: pathlib.Path) -> List[str]:
    # Define the path processing function based on the input type
    process_path_func: typing.Callable[[str], str]

    # Read logs from the input file
    logs_to_compress = []
    with open(logs_list_path, "r") as f:
        for path in f:
            stripped_path_str = path.strip()
            if "" == stripped_path_str:
                # Skip empty paths
                continue
            logs_to_compress.append(stripped_path_str)

    return logs_to_compress


def _add_common_arguments(args_parser: argparse.ArgumentParser) -> None:
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
    input_type_args_parser = args_parser.add_subparsers(dest="input_type")

    fs_compressor_parser = input_type_args_parser.add_parser(InputType.FS)
    _add_common_arguments(fs_compressor_parser)

    s3_compressor_parser = input_type_args_parser.add_parser(InputType.S3)
    _add_common_arguments(s3_compressor_parser)
    s3_compressor_parser.add_argument(
        "--aws-access-key-id", type=str, default=None, help="AWS access key ID."
    )
    s3_compressor_parser.add_argument(
        "--aws-secret-access-key", type=str, default=None, help="AWS secret access key."
    )

    parsed_args = args_parser.parse_args(argv[1:])

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_input_logs_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    comp_jobs_dir = clp_config.logs_directory / "comp-jobs"
    comp_jobs_dir.mkdir(parents=True, exist_ok=True)

    logs_to_compress = _get_logs_to_compress(pathlib.Path(parsed_args.logs_list).resolve())

    clp_input_config = _generate_clp_io_config(logs_to_compress, parsed_args)
    clp_output_config = OutputConfig.parse_obj(clp_config.archive_output)
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
