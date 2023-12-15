import argparse
import logging
import pathlib
import subprocess
import sys
import uuid

import yaml

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    validate_and_load_config_file,
    get_clp_home
)
from clp_py_utils.clp_config import CLPConfig

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def decompress_paths(clp_home: pathlib.Path, paths, list_path: pathlib.Path,
                     clp_config: CLPConfig, archives_dir: pathlib.Path,
                     logs_dir: pathlib.Path, extraction_dir: pathlib.Path):
    # Generate database config file for clp
    db_config_file_path = logs_dir / f'.decompress-db-config-{uuid.uuid4()}.yml'
    with open(db_config_file_path, 'w') as f:
        yaml.safe_dump(clp_config.database.get_clp_connection_params_and_type(True), f)

    decompression_cmd = [
        str(clp_home / 'bin' / 'clp'),
        'x', str(archives_dir), str(extraction_dir),
        '--db-config-file', str(db_config_file_path),
    ]
    files_to_decompress_list_path = None
    if list_path is not None:
        decompression_cmd.append('-f')
        decompression_cmd.append(str(list_path))
    elif len(paths) > 0:
        # Write paths to file
        files_to_decompress_list_path = logs_dir / f'paths-to-decompress-{uuid.uuid4()}.txt'
        with open(files_to_decompress_list_path, 'w') as stream:
            for path in paths:
                stream.write(path + '\n')

        decompression_cmd.append('-f')
        decompression_cmd.append(str(files_to_decompress_list_path))

    proc = subprocess.Popen(decompression_cmd)
    return_code = proc.wait()
    if 0 != return_code:
        logger.error(f"Decompression failed, return_code={return_code}")
        return return_code

    # Remove generated files
    if files_to_decompress_list_path is not None:
        files_to_decompress_list_path.unlink()
    db_config_file_path.unlink()

    return 0


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Decompresses logs.")
    args_parser.add_argument('--config', '-c', required=True, default=str(default_config_file_path),
                             help="CLP configuration file.")
    args_parser.add_argument('paths', metavar='PATH', nargs='*', help="Paths to decompress.")
    args_parser.add_argument('-f', '--files-from', help="Decompress all paths in the given list.")
    args_parser.add_argument('-d', '--extraction-dir', metavar='DIR', help="Decompress files into DIR", default='.')
    parsed_args = args_parser.parse_args(argv[1:])

    # Validate paths were specified using only one method
    if len(parsed_args.paths) > 0 and parsed_args.files_from is not None:
        args_parser.error("Paths cannot be specified both on the command line and through a file.")

    # Validate extraction directory
    extraction_dir = pathlib.Path(parsed_args.extraction_dir)
    if not extraction_dir.is_dir():
        logger.error(f"extraction-dir ({extraction_dir}) is not a valid directory.")
        return -1

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_archive_output_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    return decompress_paths(clp_home, parsed_args.paths, parsed_args.files_from, clp_config,
                            clp_config.archive_output.directory, clp_config.logs_directory, extraction_dir)


if '__main__' == __name__:
    sys.exit(main(sys.argv))
