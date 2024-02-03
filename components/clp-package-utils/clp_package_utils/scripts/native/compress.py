import argparse
import logging
import pathlib
import shutil
import sys
import time
import uuid
from contextlib import closing

import msgpack
import zstandard as zstd
from clp_py_utils.clp_config import COMPRESSION_JOBS_TABLE_NAME
from clp_py_utils.pretty_size import pretty_size
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.constants import (
    CompressionJobCompletionStatus,
    CompressionJobStatus,
)
from job_orchestration.scheduler.job_config import ClpIoConfig, InputConfig, OutputConfig

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    get_clp_home,
    validate_and_load_config_file,
)

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def handle_job_update(db, db_cursor, job_id, no_progress_reporting):
    if no_progress_reporting:
        polling_query = (
            f"SELECT status, status_msg FROM {COMPRESSION_JOBS_TABLE_NAME} WHERE id={job_id}"
        )
    else:
        polling_query = (
            f"SELECT status, status_msg, uncompressed_size, compressed_size "
            f"FROM {COMPRESSION_JOBS_TABLE_NAME} WHERE id={job_id}"
        )

    completion_query = (
        f"SELECT duration, uncompressed_size, compressed_size "
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

        if not no_progress_reporting:
            job_uncompressed_size = job_row["uncompressed_size"]
            job_compressed_size = job_row["compressed_size"]
            if job_uncompressed_size > 0:
                compression_ratio = float(job_uncompressed_size) / job_compressed_size
                if job_last_uncompressed_size < job_uncompressed_size:
                    logger.info(
                        f"Compressed {pretty_size(job_uncompressed_size)} into "
                        f"{pretty_size(job_compressed_size)} ({compression_ratio:.2f})"
                    )
                    job_last_uncompressed_size = job_uncompressed_size

        if CompressionJobStatus.SUCCEEDED == job_status:
            # All tasks in the job is done
            speed = 0
            if not no_progress_reporting:
                db_cursor.execute(completion_query)
                job_row = db_cursor.fetchone()
                if job_row["duration"] and job_row["duration"] > 0:
                    speed = job_row["uncompressed_size"] / job_row["duration"]
                logger.info(
                    f"Compression finished. Runtime: {job_row['duration']}s. "
                    f"Speed: {pretty_size(speed)}/s."
                )
            break  # Done
        if CompressionJobStatus.FAILED == job_status:
            # One or more tasks in the job has failed
            logger.error(f"Compression failed. {job_row['status_msg']}")
            break  # Done
        if CompressionJobStatus.RUNNING == job_status or CompressionJobStatus.PENDING == job_status:
            pass  # Simply wait another iteration
        else:
            error_msg = f"Unhandled CompressionJobStatus: {job_status}"
            raise NotImplementedError(error_msg)

        time.sleep(0.5)


def handle_job(sql_adapter: SQL_Adapter, clp_io_config: ClpIoConfig, no_progress_reporting: bool):
    zstd_cctx = zstd.ZstdCompressor(level=3)

    with closing(sql_adapter.create_connection(True)) as db, closing(
        db.cursor(dictionary=True)
    ) as db_cursor:
        try:
            compressed_clp_io_config = zstd_cctx.compress(
                msgpack.packb(clp_io_config.dict(exclude_none=True, exclude_unset=True))
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


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Compresses log files.")
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )
    args_parser.add_argument("paths", metavar="PATH", nargs="*", help="Paths to compress.")
    args_parser.add_argument(
        "-f", "--input-list", dest="input_list", help="A file listing all paths to compress."
    )
    args_parser.add_argument(
        "--remove-path-prefix",
        metavar="DIR",
        help="Removes the given path prefix from each compressed file/dir.",
    )
    args_parser.add_argument(
        "--no-progress-reporting", action="store_true", help="Disables progress reporting."
    )
    args_parser.add_argument(
        "--timestamp-key",
        help="The path (e.g. x.y) for the field containing the log event's timestamp.",
    )
    parsed_args = args_parser.parse_args(argv[1:])

    # Validate some input paths were specified
    if parsed_args.input_list is None and len(parsed_args.paths) == 0:
        args_parser.error("No paths specified.")

    # Validate paths were specified using only one method
    if len(parsed_args.paths) > 0 and parsed_args.input_list is not None:
        args_parser.error("Paths cannot be specified on the command line AND through a file.")

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(
            config_file_path, default_config_file_path, clp_home
        )
        clp_config.validate_input_logs_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    comp_jobs_dir = clp_config.logs_directory / "comp-jobs"
    comp_jobs_dir.mkdir(parents=True, exist_ok=True)

    if parsed_args.input_list is None:
        # Write paths to file
        log_list_path = comp_jobs_dir / f"{str(uuid.uuid4())}.txt"
        with open(log_list_path, "w") as f:
            for path in parsed_args.paths:
                stripped_path = path.strip()
                if "" == stripped_path:
                    # Skip empty paths
                    continue
                resolved_path = pathlib.Path(stripped_path).resolve()

                f.write(f"{resolved_path}\n")
    else:
        # Copy to jobs directory
        log_list_path = pathlib.Path(parsed_args.input_list).resolve()
        shutil.copy(log_list_path, comp_jobs_dir / log_list_path.name)

    mysql_adapter = SQL_Adapter(clp_config.database)
    clp_input_config = InputConfig(
        list_path=str(log_list_path), timestamp_key=parsed_args.timestamp_key
    )
    if parsed_args.remove_path_prefix:
        clp_input_config.path_prefix_to_remove = parsed_args.remove_path_prefix
    clp_io_config = ClpIoConfig(
        input=clp_input_config, output=OutputConfig.parse_obj(clp_config.archive_output)
    )

    return handle_job(
        sql_adapter=mysql_adapter,
        clp_io_config=clp_io_config,
        no_progress_reporting=parsed_args.no_progress_reporting,
    )


if "__main__" == __name__:
    sys.exit(main(sys.argv))
