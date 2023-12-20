import argparse
import logging
import pathlib
import shutil
import sys
import uuid

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    validate_and_load_config_file,
    get_clp_home
)
from clp_py_utils.sql_adapter import SQL_Adapter
from compression_job_handler.compression_job_handler import handle_jobs
from job_orchestration.job_config import (
    ClpIoConfig,
    InputConfig,
    OutputConfig
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

    logger.info("Compression job submitted to compression-job-handler.")

    mysql_adapter = SQL_Adapter(clp_config.database)
    clp_input_config = InputConfig(list_path=str(log_list_path))
    if parsed_args.remove_path_prefix:
        clp_input_config.path_prefix_to_remove = parsed_args.remove_path_prefix
    clp_io_config = ClpIoConfig(
        input=clp_input_config,
        output=OutputConfig.parse_obj(clp_config.archive_output)
    )

    # Execute compression-job-handler.handle_jobs
    logs_directory_abs = str(pathlib.Path(clp_config.logs_directory).resolve())
    handle_jobs(sql_adapter=mysql_adapter, clp_io_config=clp_io_config, logs_dir_abs=logs_directory_abs,
                fs_logs_required_parent_dir=pathlib.Path(clp_config.input_logs_directory),
                no_progress_reporting=parsed_args.no_progress_reporting)

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
