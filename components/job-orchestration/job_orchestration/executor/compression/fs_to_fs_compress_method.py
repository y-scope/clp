"""
This module is specifically to hold the remote method, easing the process of
figuring out what imports it requires.
"""
import json
import pathlib
import subprocess
import sys

import celery.utils.nodenames
import yaml
from celery.utils.log import get_task_logger

from clp_py_utils.clp_io_config import ClpIoConfig, PathsToCompress


def compress(clp_config: ClpIoConfig, clp_home_str: str, data_dir_str: str, logs_dir_str: str,
             job_id_str: str, task_id_str: str, paths_to_compress: PathsToCompress, database_connection_params):
    """
    Compresses files from an FS into archives on an FS

    :param clp_config: ClpIoConfig
    :param clp_home_str:
    :param data_dir_str:
    :param logs_dir_str:
    :param job_id_str:
    :param task_id_str:
    :param paths_to_compress: PathToCompress
    :param database_connection_params:
    :return: tuple -- (whether compression was successful, output messages)
    """
    # Setup logging
    logger = get_task_logger(__name__)

    instance_id_str = f'job-{job_id_str}-task-{task_id_str}'

    clp_home = pathlib.Path(clp_home_str)

    # Add clp package to sys.path
    python_site_packages_path = clp_home / 'lib' / 'python3' / 'site-packages'
    if not python_site_packages_path.is_dir():
        logger.error('Failed to load python3 packages bundled with CLP.')
        return False, 0
    # Add packages to the front of the path
    sys.path.insert(0, str(python_site_packages_path))

    # Expand parameters
    path_prefix_to_remove = clp_config.input.path_prefix_to_remove

    file_paths = paths_to_compress.file_paths

    data_dir = pathlib.Path(data_dir_str).resolve()
    logs_dir = pathlib.Path(logs_dir_str).resolve()

    # Generate database config file for clp
    db_config_file_path = data_dir / f'{instance_id_str}-db-config.yml'
    db_config_file = open(db_config_file_path, 'w')
    yaml.safe_dump(database_connection_params, db_config_file)
    db_config_file.close()

    # Start assembling compression command
    archives_dir = data_dir / 'archives'
    compression_cmd = [
        str(clp_home / 'bin' / 'clp'),
        'c', str(archives_dir),
        '--print-archive-stats-progress',
        '--target-dictionaries-size',
        str(clp_config.output.target_dictionaries_size),
        '--target-segment-size', str(clp_config.output.target_segment_size),
        '--target-encoded-file-size', str(clp_config.output.target_encoded_file_size),
        '--storage-id',
        '--db-config-file', str(db_config_file_path)
    ]
    if clp_config.output.storage_is_node_specific:
        compression_cmd.append(celery.utils.nodenames.gethostname())
    else:
        # Mark as globally-accessible
        compression_cmd.append("*")
    if path_prefix_to_remove:
        compression_cmd.append("--remove-path-prefix")
        compression_cmd.append(path_prefix_to_remove)

    # Prepare list of paths to compress for clp
    log_list_path = data_dir / f'{instance_id_str}-log-paths.txt'
    with open(log_list_path, 'w') as file:
        if len(file_paths) > 0:
            for path_str in file_paths:
                file.write(path_str)
                file.write('\n')
        if paths_to_compress.empty_directories and len(paths_to_compress.empty_directories) > 0:
            # Prepare list of paths to compress for clp
            for path_str in paths_to_compress.empty_directories:
                file.write(path_str)
                file.write('\n')

        compression_cmd.append('--files-from')
        compression_cmd.append(str(log_list_path))

    # Open stderr log file
    stderr_log_path = logs_dir / f'{instance_id_str}-stderr.log'
    stderr_log_file = open(stderr_log_path, 'w')

    # Start compression
    logger.debug('Compressing...')
    compression_successful = False
    proc = subprocess.Popen(compression_cmd, close_fds=True, stdout=subprocess.PIPE,
                            stderr=stderr_log_file)

    # Compute the total amount of data compressed
    last_archive_stats = None
    total_uncompressed_size = 0
    total_compressed_size = 0
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        stats = json.loads(line.decode('ascii'))
        if last_archive_stats is not None and stats["id"] != last_archive_stats["id"]:
            # We've started a new archive so add the previous archive's last
            # reported size to the total
            total_uncompressed_size += last_archive_stats["uncompressed_size"]
            total_compressed_size += last_archive_stats["size"]
        last_archive_stats = stats
    if last_archive_stats is not None:
        # Add the last archive's last reported size
        total_uncompressed_size += last_archive_stats["uncompressed_size"]
        total_compressed_size += last_archive_stats["size"]

    # Wait for compression to finish
    return_code = proc.wait()
    if 0 != return_code:
        logger.error(f'Failed to compress, return_code={str(return_code)}')
    else:
        compression_successful = True

        # Remove generated temporary files
        if log_list_path:
            log_list_path.unlink()
        db_config_file_path.unlink()
    logger.debug('Compressed.')

    # Close stderr log file
    stderr_log_file.close()

    if compression_successful:
        return compression_successful, {
            'total_uncompressed_size': total_uncompressed_size,
            'total_compressed_size': total_compressed_size,
        }
    else:
        return compression_successful, {
            'error_message': f'See logs {str(stderr_log_path)}'
        }
