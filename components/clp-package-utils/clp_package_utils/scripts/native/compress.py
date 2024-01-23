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

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    validate_and_load_config_file,
    get_clp_home
)
from clp_py_utils.pretty_size import pretty_size
from clp_py_utils.sql_adapter import SQL_Adapter

from job_orchestration.job_config import (
    ClpIoConfig,
    InputConfig,
    OutputConfig
)
from job_orchestration.scheduler.constants import (
    JobStatus,
    JobCompletionStatus
)

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.DEBUG)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def handle_job(sql_adapter: SQL_Adapter, clp_io_config: ClpIoConfig, no_progress_reporting: bool):
    # Instantiate zstdandard compression context
    zstd_cctx = zstd.ZstdCompressor(level=3)

    # Connect to SQL Database
    with closing(sql_adapter.create_connection(True)) as scheduling_db, \
            closing(scheduling_db.cursor(dictionary=True)) as scheduling_db_cursor:
        try:
            scheduling_db_cursor.execute(
                'INSERT INTO compression_jobs (clp_config) VALUES (%s);',
                (zstd_cctx.compress(msgpack.packb(clp_io_config.dict(exclude_none=True, exclude_unset=True))),)
            )
            scheduling_db.commit()
            scheduling_job_id = scheduling_db_cursor.lastrowid

            if no_progress_reporting:
                polling_query = f"SELECT status, status_msg FROM compression_jobs WHERE id={scheduling_job_id}"
            else:
                polling_query = f"SELECT status, status_msg, uncompressed_size, compressed_size " \
                                f"FROM compression_jobs WHERE id={scheduling_job_id}"

            completion_query = f"SELECT duration, uncompressed_size, compressed_size " \
                               f"FROM compression_jobs WHERE id={scheduling_job_id}"

            job_last_uncompressed_size = 0
            while True:
                scheduling_db_cursor.execute(polling_query)
                results = scheduling_db_cursor.fetchall()
                scheduling_db.commit()
                if len(results) > 1:
                    logging.error("Duplicated job_id")
                    logging.error(str(results))
                if len(results) == 0:
                    time.sleep(0.5)
                    continue
                job_row = results[0]
                job_status = job_row['status']

                if not no_progress_reporting:
                    job_uncompressed_size = job_row['uncompressed_size']
                    job_compressed_size = job_row['compressed_size']
                    if job_uncompressed_size > 0:
                        compression_ratio = float(job_uncompressed_size) / job_compressed_size
                        if job_last_uncompressed_size < job_uncompressed_size:
                            logger.info(
                                f'Compressed {pretty_size(job_uncompressed_size)} into '
                                f'{pretty_size(job_compressed_size)} ({compression_ratio:.2f})')
                            job_last_uncompressed_size = job_uncompressed_size

                if JobStatus.SCHEDULED == job_status or JobStatus.SCHEDULING == job_status:
                    pass  # Simply wait another iteration
                elif JobStatus.SUCCEEDED == job_status:
                    # All tasks in the job is done
                    speed = 0
                    if not no_progress_reporting:
                        scheduling_db_cursor.execute(completion_query)
                        job_row = scheduling_db_cursor.fetchone()
                        if job_row['duration'] and job_row['duration'] > 0:
                            speed = job_row['uncompressed_size'] / job_row['duration']
                        logger.info(f"Compression finished. Runtime: {str(job_row['duration'])}s. "
                                    f"Speed: {pretty_size(speed)}/s.")
                    break  # Done
                elif JobStatus.FAILED == job_status:
                    # One or more tasks in the job has failed
                    logger.error(f"Compression failed. {job_row['status_msg']}")
                    break  # Done
                else:
                    logger.info(f'handler for job_status "{job_status}" is not implemented')
                    raise NotImplementedError

                time.sleep(0.5)
        except Exception as ex:
            return JobCompletionStatus.FAILED

        logger.debug(f'Finished job {scheduling_job_id}')

        return JobCompletionStatus.SUCCEEDED


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Compresses log files.")
    args_parser.add_argument('--config', '-c', default=str(default_config_file_path),
                             help="CLP package configuration file.")
    args_parser.add_argument('paths', metavar='PATH', nargs='*', help="Paths to compress.")
    args_parser.add_argument('-f', '--input-list', dest='input_list', help="A file listing all paths to compress.")
    args_parser.add_argument('--remove-path-prefix', metavar='DIR',
                             help="Removes the given path prefix from each compressed file/dir.")
    args_parser.add_argument('--no-progress-reporting', action='store_true', help="Disables progress reporting.")
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
        clp_config = validate_and_load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_input_logs_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    comp_jobs_dir = clp_config.logs_directory / 'comp-jobs'
    comp_jobs_dir.mkdir(parents=True, exist_ok=True)

    if parsed_args.input_list is None:
        # Write paths to file
        log_list_path = comp_jobs_dir / f'{str(uuid.uuid4())}.txt'
        with open(log_list_path, 'w') as f:
            for path in parsed_args.paths:
                stripped_path = path.strip()
                if '' == stripped_path:
                    # Skip empty paths
                    continue
                resolved_path = pathlib.Path(stripped_path).resolve()

                f.write(f"{resolved_path}\n")
    else:
        # Copy to jobs directory
        log_list_path = pathlib.Path(parsed_args.input_list).resolve()
        shutil.copy(log_list_path, comp_jobs_dir / log_list_path.name)

    logger.info("Compression job submitted.")

    mysql_adapter = SQL_Adapter(clp_config.database)
    clp_input_config = InputConfig(list_path=str(log_list_path))
    if parsed_args.remove_path_prefix:
        clp_input_config.path_prefix_to_remove = parsed_args.remove_path_prefix
    clp_io_config = ClpIoConfig(
        input=clp_input_config,
        output=OutputConfig.parse_obj(clp_config.archive_output)
    )

    handle_job(sql_adapter=mysql_adapter, clp_io_config=clp_io_config,
               no_progress_reporting=parsed_args.no_progress_reporting)

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
