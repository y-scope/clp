import copy
import json
import pathlib
import typing

import brotli
import msgpack
from clp_py_utils.compression import (
    FileMetadata,
    FilesPartition,
    group_files_by_similar_filenames,
)
from job_orchestration.scheduler.job_config import ClpIoConfig, PathsToCompress


class PathsToCompressBuffer:
    def __init__(
        self,
        maintain_file_ordering: bool,
        empty_directories_allowed: bool,
        scheduling_job_id: int,
        clp_io_config: ClpIoConfig,
        clp_metadata_db_connection_config: dict,
    ):
        self.__files: typing.List[FileMetadata] = []
        self.__tasks: typing.List[typing.Dict[str, typing.Any]] = []
        self.__partition_info: typing.List[typing.Dict[str, typing.Any]] = []
        self.__maintain_file_ordering: bool = maintain_file_ordering
        if empty_directories_allowed:
            self.__empty_directories: typing.Optional[typing.List[str]] = []
        else:
            self.__empty_directories: typing.Optional[typing.List[str]] = None
        self.__total_file_size: int = 0
        self.__target_archive_size: int = clp_io_config.output.target_archive_size
        self.__file_size_to_trigger_compression: int = clp_io_config.output.target_archive_size * 2

        self.num_tasks = 0
        self.__task_arguments = {
            "job_id": scheduling_job_id,
            "tag_ids": None,
            "task_id": -1,
            "clp_io_config_json": clp_io_config.json(exclude_none=True),
            "paths_to_compress_json": None,
            "clp_metadata_db_connection_config": clp_metadata_db_connection_config,
        }

    def get_tasks(self):
        return self.__tasks

    def get_partition_info(self):
        return self.__partition_info

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
            self.__empty_directories and len(self.__empty_directories) > 0
        )

    def __submit_partition_for_compression(self, partition: FilesPartition):
        files, file_paths, group_ids, st_sizes, partition_total_file_size = partition.pop_files()
        paths_to_compress = PathsToCompress(
            file_paths=file_paths, group_ids=group_ids, st_sizes=st_sizes
        )

        if self.__empty_directories is not None and len(self.__empty_directories) > 0:
            paths_to_compress.empty_directories = self.__empty_directories
            self.__empty_directories = []

        self.__partition_info.append(
            {
                "partition_original_size": str(sum(st_sizes)),
                "clp_paths_to_compress": brotli.compress(
                    msgpack.packb(paths_to_compress.dict(exclude_none=True)), quality=4
                ),
            }
        )

        task_arguments = self.__task_arguments.copy()
        task_arguments["paths_to_compress_json"] = paths_to_compress.json(exclude_none=True)
        self.__tasks.append(copy.deepcopy(task_arguments))
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
            group_id = groups[group_ix]["id"]
            group_files = groups[group_ix]["files"]

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
                        self.__total_file_size -= self.__submit_partition_for_compression(partition)
                        if self.__total_file_size < self.__target_archive_size:
                            # Not enough files to fill a partition, so break
                            break
                # Pop compressed files
                self.__files = self.__files[file_ix + 1 :]

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
                group_id = groups[group_ix]["id"]
                group_files = groups[group_ix]["files"]

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
                group_files = group["files"]
                group_file_ix = next_file_ix_per_group[group_ix]
                for i in range(group_file_ix, len(group_files)):
                    remaining_files.append(group_files[i])
            self.__files = remaining_files

            # Compress any remaining empty directories
            if flush_buffer and self.contains_paths():
                self.__total_file_size -= self.__submit_partition_for_compression(partition)
                self.__files = []
