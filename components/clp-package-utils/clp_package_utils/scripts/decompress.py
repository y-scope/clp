#!/usr/bin/env python3
import argparse
import logging
import os
import pathlib
import subprocess
import sys
import uuid

# Setup logging
# Create logger
logger = logging.getLogger('clp')
logger.setLevel(logging.DEBUG)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def get_clp_home():
    # Determine CLP_HOME from an environment variable or this script's path
    _clp_home = None
    if 'CLP_HOME' in os.environ:
        _clp_home = pathlib.Path(os.environ['CLP_HOME'])
    else:
        for path in pathlib.Path(__file__).resolve().parents:
            if 'sbin' == path.name:
                _clp_home = path.parent
                break

    if _clp_home is None:
        logger.error("CLP_HOME is not set and could not be determined automatically.")
        return None
    elif not _clp_home.exists():
        logger.error("CLP_HOME set to nonexistent path.")
        return None

    return _clp_home.resolve()


def load_bundled_python_lib_path(_clp_home):
    python_site_packages_path = _clp_home / 'lib' / 'python3' / 'site-packages'
    if not python_site_packages_path.is_dir():
        logger.error("Failed to load python3 packages bundled with CLP.")
        return False

    # Add packages to the front of the path
    sys.path.insert(0, str(python_site_packages_path))

    return True


clp_home = get_clp_home()
if clp_home is None or not load_bundled_python_lib_path(clp_home):
    sys.exit(-1)

import yaml
from clp_package_utils.general import \
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH, \
    CONTAINER_CLP_HOME, \
    DockerMount, \
    DockerMountType, \
    generate_container_config, \
    validate_and_load_config_file, \
    validate_and_load_db_credentials_file, \
    validate_path_could_be_dir


def main(argv):
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Decompresses logs")
    args_parser.add_argument('--config', '-c', type=str, default=str(default_config_file_path),
                             help="CLP package configuration file.")
    args_parser.add_argument('paths', metavar='PATH', nargs='*', help="Files to decompress.")
    args_parser.add_argument('-f', '--files-from', help="A file listing all files to decompress.")
    args_parser.add_argument('-d', '--extraction-dir', metavar='DIR', default='.', help="Decompress files into DIR")
    parsed_args = args_parser.parse_args(argv[1:])

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()

        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except:
        logger.exception("Failed to load config.")
        return -1

    paths_to_decompress_file_path = None
    if parsed_args.files_from:
        paths_to_decompress_file_path = pathlib.Path(parsed_args.files_from)

    # Validate extraction directory
    extraction_dir = pathlib.Path(parsed_args.extraction_dir).resolve()
    try:
        validate_path_could_be_dir(extraction_dir)
    except ValueError as ex:
        logger.error(f"extraction-dir is invalid: {ex}")
        return -1
    extraction_dir.mkdir(exist_ok=True)

    container_name = f'clp-decompressor-{str(uuid.uuid4())[-4:]}'

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    container_config_filename = f'.{container_name}-config.yml'
    container_config_file_path_on_host = clp_config.logs_directory / container_config_filename
    with open(container_config_file_path_on_host, 'w') as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    container_start_cmd = [
        'docker', 'run',
        '-i',
        '--rm',
        '--network', 'host',
        '-w', str(CONTAINER_CLP_HOME),
        '-u', f'{os.getuid()}:{os.getgid()}',
        '--name', container_name,
        '--mount', str(mounts.clp_home),
    ]

    # Set up mounts
    container_extraction_dir = pathlib.Path('/') / 'mnt' / 'extraction-dir'
    necessary_mounts = [
        mounts.data_dir,
        mounts.logs_dir,
        mounts.archives_output_dir,
        DockerMount(DockerMountType.BIND, extraction_dir, container_extraction_dir),
    ]
    container_paths_to_decompress_file_path = None
    if paths_to_decompress_file_path:
        container_paths_to_decompress_file_path = pathlib.Path('/') / 'mnt' / 'paths-to-decompress.txt'
        necessary_mounts.append(
            DockerMount(DockerMountType.BIND, paths_to_decompress_file_path, container_paths_to_decompress_file_path))
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append('--mount')
            container_start_cmd.append(str(mount))

    container_start_cmd.append(clp_config.execution_container)

    decompress_cmd = [
        str(CONTAINER_CLP_HOME / 'sbin' / 'native' / 'decompress'),
        '--config', str(container_clp_config.logs_directory / container_config_filename),
        '-d', str(container_extraction_dir)
    ]
    for path in parsed_args.paths:
        decompress_cmd.append(path)
    if container_paths_to_decompress_file_path:
        decompress_cmd.append('--input-list')
        decompress_cmd.append(container_paths_to_decompress_file_path)

    cmd = container_start_cmd + decompress_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    container_config_file_path_on_host.unlink()

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
