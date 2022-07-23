#!/usr/bin/env python3
import argparse
import logging
import pathlib
import sys
import time
import typing
from contextlib import closing

import msgpack
import zstandard
import zstandard as zstd
from pydantic import ValidationError

from clp_py_utils.clp_config import CLPConfig
from clp_py_utils.clp_io_config import PathsToCompress, InputConfig, OutputConfig, ClpIoConfig
from clp_py_utils.compression import FileMetadata, FilesPartition, \
    group_files_by_similar_filenames, validate_path_and_get_info
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.pretty_size import pretty_size
from clp_py_utils.sql_adapter import SQL_Adapter
from .utils.common import JobCompletionStatus

# Setup logging
# Create logger
logger = logging.getLogger('compression-job-handler')
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter('%(asctime)s [%(levelname)s] [%(name)s] %(message)s')
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


class PathsToCompressBuffer:
    def __init__(self, scheduler_db_cursor, maintain_file_ordering: bool,
                 empty_directories_allowed: bool, target_archive_size: int,
                 file_size_to_trigger_compression, scheduling_job_id: int, zstd_cctx):
        self.__files: typing.List[FileMetadata] = []
        self.__maintain_file_ordering: bool = maintain_file_ordering
        if empty_directories_allowed:
            self.__empty_directories: typing.Optional[typing.List[str]] = []
        else:
            self.__empty_directories: typing.Optional[typing.List[str]] = None
        self.__total_file_size: int = 0
        self.__target_archive_size: int = target_archive_size
        self.__file_size_to_trigger_compression: int = file_size_to_trigger_compression
        self.__scheduling_job_id: int = scheduling_job_id
        self.scheduling_job_id: int = scheduling_job_id
        self.__zstd_cctx = zstd_cctx

        self.__scheduler_db_cursor = scheduler_db_cursor
        self.num_tasks = 0

    def add_file(self, file: FileMetadata):
        self.__files.append(file)
        self.__total_file_size += file.estimated_uncompressed_size

        if self.__total_file_size >= self.__file_size_to_trigger_compression:
            self.__partition_and_compress(False)

    def add_empty_directory(self, path: pathlib.Path):
        if self.__empty_directories is None:
            return
        self.__empty_directories.append(str(path))

    def flush(self):
        self.__partition_and_compress(True)

    def contains_paths(self):
        return len(self.__files) > 0 or (
                self.__empty_directories and len(self.__empty_directories) > 0)

    def __submit_partition_for_compression(self, partition: FilesPartition):
        files, file_paths, group_ids, st_sizes, partition_total_file_size = partition.pop_files()
        paths_to_compress = PathsToCompress(file_paths=file_paths, group_ids=group_ids, st_sizes=st_sizes)

        if self.__empty_directories is not None and len(self.__empty_directories) > 0:
            paths_to_compress.empty_directories = self.__empty_directories
            self.__empty_directories = []

        # Note: partition_total_file_size => estimated size, aggregate
        # the st_size => real original size
        self.__scheduler_db_cursor.execute(
            f'INSERT INTO compression_tasks '
            f'(job_id, partition_original_size, clp_paths_to_compress) '
            f'VALUES({str(self.__scheduling_job_id)}, {str(sum(st_sizes))}, %s);',
            (self.__zstd_cctx.compress(msgpack.packb(paths_to_compress.dict(exclude_none=True))),)
        )
        self.num_tasks += 1

        return partition_total_file_size

    def add_files(self, target_num_archives: int, target_archive_size: int, files):
        target_num_archives = min(len(files), target_num_archives)

        groups = group_files_by_similar_filenames(files)
        next_file_ix_per_group = [0 for _ in range(len(groups))]

        partitions = [FilesPartition() for _ in range(target_num_archives)]

        # Distribute files across partitions in round-robin order; full partitions are skipped
        next_partition_ix = 0
        group_ix = 0
        while len(groups) > 0:
            group_file_ix = next_file_ix_per_group[group_ix]
            group_id = groups[group_ix]['id']
            group_files = groups[group_ix]['files']

            file = group_files[group_file_ix]

            # Look for a partition with space
            while True:
                partition = partitions[next_partition_ix]
                next_partition_ix = (next_partition_ix + 1) % target_num_archives
                if partition.get_total_file_size() < target_archive_size:
                    break

            partition.add_file(file, group_id)

            group_file_ix += 1
            if len(group_files) == group_file_ix:
                groups.pop(group_ix)
                next_file_ix_per_group.pop(group_ix)
            else:
                next_file_ix_per_group[group_ix] = group_file_ix
                group_ix += 1
            if len(groups) > 0:
                group_ix %= len(groups)

        # Compress partitions
        for partition in partitions:
            self.__submit_partition_for_compression(partition)

    def __partition_and_compress(self, flush_buffer: bool):
        if not flush_buffer and self.__total_file_size < self.__target_archive_size:
            # Not enough data for a full partition and we don't need to exhaust the buffer
            return
        if not self.contains_paths():
            # Nothing to compress
            return

        partition = FilesPartition()

        if self.__maintain_file_ordering:
            # NOTE: grouping by filename is not supported when maintaining file ordering,
            # so we give each file its own group ID to maintain ordering

            group_ix = 0
            # Compress full partitions
            if self.__total_file_size >= self.__target_archive_size:
                file_ix = 0
                for file_ix, file in enumerate(self.__files):
                    partition.add_file(file, group_ix)
                    group_ix += 1

                    # Compress partition if ready
                    if partition.get_total_file_size() >= self.__target_archive_size:
                        self.__total_file_size -= self.__submit_partition_for_compression(
                            partition)
                        if self.__total_file_size < self.__target_archive_size:
                            # Not enough files to fill a partition, so break
                            break
                # Pop compressed files
                self.__files = self.__files[file_ix + 1:]

            # Compress remaining partial partition if necessary
            if flush_buffer and self.contains_paths():
                for file in self.__files:
                    partition.add_file(file, group_ix)
                    group_ix += 1
                self.__total_file_size -= self.__submit_partition_for_compression(partition)
                self.__files = []
        else:
            groups = group_files_by_similar_filenames(self.__files)
            next_file_ix_per_group = [0 for _ in range(len(groups))]

            group_ix = 0
            while len(groups) > 0:
                group_file_ix = next_file_ix_per_group[group_ix]
                group_id = groups[group_ix]['id']
                group_files = groups[group_ix]['files']

                file = group_files[group_file_ix]

                partition.add_file(file, group_id)

                group_file_ix += 1
                if len(group_files) == group_file_ix:
                    groups.pop(group_ix)
                    next_file_ix_per_group.pop(group_ix)
                else:
                    next_file_ix_per_group[group_ix] = group_file_ix
                    group_ix += 1
                if len(groups) > 0:
                    group_ix %= len(groups)

                # Compress partition if ready
                if partition.get_total_file_size() >= self.__target_archive_size:
                    self.__total_file_size -= self.__submit_partition_for_compression(partition)
                    if not flush_buffer and self.__total_file_size < self.__target_archive_size:
                        # Not enough files to fill a partition and
                        # we don't need to exhaust the buffer, so break
                        break

            # Compress partial partition
            if partition.contains_files():
                self.__total_file_size -= self.__submit_partition_for_compression(partition)
                self.__files = []

            # Pop compressed files
            remaining_files = []
            for group_ix, group in enumerate(groups):
                group_files = group['files']
                group_file_ix = next_file_ix_per_group[group_ix]
                for i in range(group_file_ix, len(group_files)):
                    remaining_files.append(group_files[i])
            self.__files = remaining_files

            # Compress any remaining empty directories
            if flush_buffer and self.contains_paths():
                self.__total_file_size -= self.__submit_partition_for_compression(partition)
                self.__files = []


def handle_job(scheduling_db, scheduling_db_cursor, clp_io_config: ClpIoConfig, logs_dir_abs: str,
               fs_logs_required_parent_dir: pathlib.Path, zstd_cctx: zstandard.ZstdCompressor,
               no_progress_reporting: bool) -> JobCompletionStatus:
    job_logger = None
    all_worker_jobs_successful = True

    try:
        job_completed_with_errors = False
        # Create new job in the sql database
        scheduling_db_cursor.execute(
            'INSERT INTO compression_jobs (clp_config) VALUES (%s);',
            (zstd_cctx.compress(msgpack.packb(clp_io_config.dict(exclude_none=True, exclude_unset=True))),)
        )
        scheduling_db.commit()
        scheduling_job_id = scheduling_db_cursor.lastrowid

        # Create job-specific logger
        job_str = f'job-{scheduling_job_id}'
        # FIXME: This will write to the current working directory which may require elevated privileges
        job_logger = logging.getLogger(job_str)
        job_logger.setLevel(logging.INFO)
        combined_log_file_path = f'{logs_dir_abs}/{job_str}.log'
        job_logger_file_handler = logging.FileHandler(combined_log_file_path)
        job_logger_file_handler.setFormatter(logging_formatter)
        job_logger.addHandler(logging_console_handler)
        job_logger.addHandler(job_logger_file_handler)

        job_logger.debug(f'Starting job {scheduling_job_id}')

        paths_to_compress_buffer = PathsToCompressBuffer(
            scheduler_db_cursor=scheduling_db_cursor,
            maintain_file_ordering=False,
            empty_directories_allowed=True,
            target_archive_size=clp_io_config.output.target_archive_size,
            file_size_to_trigger_compression=clp_io_config.output.target_archive_size * 2,
            scheduling_job_id=scheduling_job_id,
            zstd_cctx=zstd_cctx
        )

        # Compress all files at once to try and satisfy the target number of archives
        job_logger.info("Iterating and partitioning files into tasks.")
        # TODO: Handle file not found
        with open(pathlib.Path(clp_io_config.input.list_path).resolve(), 'r') as f:
            for path_idx, path in enumerate(f, start=1):
                stripped_path = path.strip()
                if '' == stripped_path:
                    # Skip empty paths
                    continue
                path = pathlib.Path(stripped_path)

                try:
                    file, empty_directory = validate_path_and_get_info(fs_logs_required_parent_dir, path)
                except ValueError as ex:
                    job_logger.error(str(ex))
                    job_completed_with_errors = True
                    continue

                if file:
                    paths_to_compress_buffer.add_file(file)
                elif empty_directory:
                    paths_to_compress_buffer.add_empty_directory(empty_directory)

                if path.is_dir():
                    for internal_path in path.rglob('*'):
                        try:
                            file, empty_directory = validate_path_and_get_info(
                                fs_logs_required_parent_dir, internal_path)
                        except ValueError as ex:
                            job_logger.error(str(ex))
                            job_completed_with_errors = True
                            continue

                        if file:
                            paths_to_compress_buffer.add_file(file)
                        elif empty_directory:
                            paths_to_compress_buffer.add_empty_directory(empty_directory)

                if path_idx % 10000 == 0:
                    scheduling_db.commit()

        paths_to_compress_buffer.flush()

        # Ensure all of the scheduled task and the total number of tasks
        # in the job row has been updated and committed
        scheduling_db_cursor.execute(
            f'UPDATE compression_jobs '
            f'SET num_tasks={paths_to_compress_buffer.num_tasks}, job_status="SCHEDULED" '
            f'WHERE job_id={scheduling_job_id};'
        )
        scheduling_db.commit()

        # TODO: what happens when commit fails, log error and crash ASAP

        # Wait for jobs to finish
        job_logger.info(f'Waiting for {paths_to_compress_buffer.num_tasks} task(s) to finish.')

        # Simply poll the job_status in the job scheduling table
        if no_progress_reporting:
            polling_query = \
                f'SELECT job_status, job_status_msg FROM compression_jobs ' \
                f'WHERE job_id={scheduling_job_id};'
        else:
            polling_query = \
                f'SELECT job_status, job_status_msg, job_uncompressed_size, job_compressed_size ' \
                f'FROM compression_jobs WHERE job_id={scheduling_job_id};'

        completion_query = \
            f'SELECT job_duration, job_uncompressed_size, job_compressed_size ' \
            f'FROM compression_jobs WHERE job_id={scheduling_job_id};'

        job_last_uncompressed_size = 0
        while True:
            scheduling_db_cursor.execute(polling_query)

            # Using fetchall() here t
            results = scheduling_db_cursor.fetchall()
            # TODO Why is this necessary in the newest MariaDB/MySQL?
            scheduling_db.commit()
            if len(results) > 1:
                logging.error("Duplicated job_id")
                logging.error(str(results))
            if len(results) == 0:
                time.sleep(1)
                continue

            job_row = results[0]
            job_status = job_row['job_status']

            if not no_progress_reporting:
                job_uncompressed_size = job_row['job_uncompressed_size']
                job_compressed_size = job_row['job_compressed_size']
                if job_uncompressed_size > 0:
                    compression_ratio = float(job_uncompressed_size) / job_compressed_size
                    if job_last_uncompressed_size < job_uncompressed_size:
                        job_logger.info(
                            f'Compressed {pretty_size(job_uncompressed_size)} into '
                            f'{pretty_size(job_compressed_size)} ({compression_ratio:.2f})')
                        job_last_uncompressed_size = job_uncompressed_size

            if job_status == 'SCHEDULED':
                pass  # Simply wait another iteration
            elif job_status == 'COMPLETED':
                # All tasks in the job is done
                if not no_progress_reporting:
                    scheduling_db_cursor.execute(completion_query)
                    job_row = scheduling_db_cursor.fetchone()
                    if job_row['job_duration']:
                        speed = job_row['job_uncompressed_size'] / job_row['job_duration']
                    job_logger.info(
                        f'Compression finished. Runtime: {str(job_row["job_duration"])}s. '
                        f'Speed: {pretty_size(speed)}/s.')
                break  # Done
            elif job_status == 'FAILED':
                # One or more tasks in the job has failed
                job_logger.error(f'Compression failed. See log file in {job_row["job_status_msg"]}')
                break  # Done
            else:
                job_logger.info(f'handler for job_status "{job_status}" is not implemented')
                raise NotImplementedError

            time.sleep(1)
    except Exception as ex:
        if job_logger:
            job_logger.exception(f'Exception while processing {job_str}.')
            job_logger.error(ex)
        all_worker_jobs_successful = False
    finally:
        if job_logger:
            job_logger.removeHandler(job_logger_file_handler)
            job_logger_file_handler.flush()
            job_logger_file_handler.close()

    if not all_worker_jobs_successful:
        return JobCompletionStatus.FAILED
    elif job_completed_with_errors:
        return JobCompletionStatus.SUCCEEDED_WITH_ERRORS

    logger.debug(f'Finished job {job_str}')

    return JobCompletionStatus.SUCCEEDED


def handle_jobs(sql_adapter: SQL_Adapter, clp_io_config: ClpIoConfig, logs_dir_abs: str,
                fs_logs_required_parent_dir: pathlib.Path, no_progress_reporting: bool):
    logger.info('compression-job-handler started.')

    # Instantiate zstdandard compression context
    zstd_cctx = zstd.ZstdCompressor(level=3)

    # Connect to SQL Database
    with closing(sql_adapter.create_connection(True)) as scheduling_db, \
            closing(scheduling_db.cursor(dictionary=True)) as scheduling_db_cursor:
        # Execute new compression job
        handle_job(scheduling_db=scheduling_db, scheduling_db_cursor=scheduling_db_cursor, clp_io_config=clp_io_config,
                   logs_dir_abs=logs_dir_abs, fs_logs_required_parent_dir=fs_logs_required_parent_dir,
                   zstd_cctx=zstd_cctx, no_progress_reporting=no_progress_reporting)


def main(argv):
    args_parser = argparse.ArgumentParser(description='Wait for and run compression jobs.')
    args_parser.add_argument('--fs-logs-required-parent-dir', default="/nonexistent",
                             help='The required parent for any logs ingested from the filesystem.')
    args_parser.add_argument('--no-progress-reporting', action='store_true', help='Disables progress reporting.')
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')
    args_parser.add_argument('--log-list-path', required=True, help='File containing list of input files to compress')
    parsed_args = args_parser.parse_args(argv[1:])

    # Load configuration
    config_path = pathlib.Path(parsed_args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
    except Exception as ex:
        # read_yaml_config_file already logs the parsing error inside
        pass
    else:
        # Configure file system directory locations   # TODO: refactor with better comment
        fs_logs_required_parent_dir = pathlib.Path(parsed_args.fs_logs_required_parent_dir)

        sql_adapter = SQL_Adapter(clp_config.database)

        clp_io_config = ClpIoConfig(
            input=InputConfig(list_path=str(pathlib.Path(parsed_args.log_list_path).resolve())),
            output=OutputConfig.parse_obj(clp_config.archive_output)
        )

        logs_directory_abs = str(pathlib.Path(clp_config.logs_directory).resolve())

        handle_jobs(sql_adapter=sql_adapter, clp_io_config=clp_io_config, logs_dir_abs=logs_directory_abs,
                    fs_logs_required_parent_dir=fs_logs_required_parent_dir,
                    no_progress_reporting=parsed_args.no_progress_reporting)

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
