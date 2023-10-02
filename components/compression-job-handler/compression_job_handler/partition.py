import copy
from logging import Logger
from pathlib import Path
from typing import Any, Dict, List

from clp_py_utils.compression import (  # type: ignore
    FileMetadata,
    FilesPartition,
    group_files_by_similar_filenames,
)


class PathsToCompressBuffer:
    def __init__(
        self,
        logger: Logger,
        worker_base_arguments: Dict[str, Any],
        jobs_arguments: List[Dict[str, Any]],
        target_archive_size: int,
        file_size_to_trigger_compression: int,
        maintain_file_ordering: bool,
        allow_empty_directories: bool,
        job_id_str: str,
        job_input_config: Dict[str, Any],
        job_output_config: Dict[str, Any],
        clp_db_config: Dict[str, Any],
    ) -> None:
        self.__logger: Logger = logger
        self.__jobs_arguments: List[Dict[str, Any]] = jobs_arguments

        self.__files: List[FileMetadata] = []
        self.__total_file_size: int = 0
        self.__target_archive_size: int = target_archive_size
        self.__file_size_to_trigger_compression: int = file_size_to_trigger_compression

        self.__maintain_file_ordering: bool = maintain_file_ordering
        self.__allow_empty_directories: bool = allow_empty_directories
        self.__empty_directories: List[str] = []

        self.__arguments = {
            "job_id_str": job_id_str,
            "job_input_config": job_input_config,
            "job_output_config": job_output_config,
            "clp_db_config": clp_db_config,
        }

        self.__arguments.update(worker_base_arguments)

    def add_file(self, file: FileMetadata) -> None:
        self.__files.append(file)
        self.__total_file_size += file.estimated_uncompressed_size

        if self.__total_file_size >= self.__file_size_to_trigger_compression:
            self.__partition_and_compress(flush_buffer=False)

    def add_files(
        self,
        files: List[FileMetadata],
        target_num_archives: int,
        target_archive_size: int,
    ) -> None:
        groups = group_files_by_similar_filenames(files)
        next_file_ix_per_group = [0 for _ in range(len(groups))]

        target_num_archives = min(len(files), target_num_archives)
        partitions = [FilesPartition() for _ in range(target_num_archives)]

        # Distribute files across partitions in round-robin order.
        # Full partitions are skipped.
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

    def add_empty_directory(self, dir_path: Path) -> None:
        if not self.__allow_empty_directories:
            return
        self.__empty_directories.append(str(dir_path))

    def flush(self) -> None:
        self.__partition_and_compress(flush_buffer=True)

    def contains_paths(self) -> bool:
        return len(self.__files) > 0 or len(self.__empty_directories) > 0

    def __submit_partition_for_compression(self, partition: FilesPartition) -> int:
        files, file_paths, group_ids, st_sizes, partition_total_file_size = partition.pop_files()
        paths_to_compress = {
            "file_paths": file_paths,
            "group_ids": group_ids,
            "st_sizes": st_sizes,
        }

        # TODO: In a single iteration of __partition_and_compress, the empty
        # directories list only applies to the first partition. Is this correct?
        if len(self.__empty_directories) > 0:
            paths_to_compress["empty_directories"] = self.__empty_directories
            self.__empty_directories = []

        arguments_for_job = self.__arguments.copy()
        arguments_for_job["job_input_config"]["paths"] = paths_to_compress
        self.__jobs_arguments.append(copy.deepcopy(arguments_for_job))
        return partition_total_file_size

    def __ordered_partition_and_compress(self, flush_buffer: bool) -> None:
        partition = FilesPartition()

        # NOTE: Grouping by filename is not supported when maintaining file
        # ordering, so we give each file its own group ID to maintain ordering
        group_ix = 0

        # Compress full partitions
        if self.__total_file_size >= self.__target_archive_size:
            file_ix = 0
            for file_ix, file in enumerate(self.__files):
                partition.add_file(file, group_ix)
                group_ix += 1

                # Compress partition if ready
                if partition.get_total_file_size() >= self.__target_archive_size:
                    partition_size = self.__submit_partition_for_compression(partition)
                    self.__total_file_size -= partition_size
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

            # Compress partition and clears the buffer
            self.__submit_partition_for_compression(partition)
            self.__files = []
            self.__total_file_size = 0

    def __free_partition_and_compress(self, flush_buffer: bool) -> None:
        # TODO: clean up and annotate/clarify the following code logic. Since
        # a similar logic is shared by add_files, consider adding different
        # compression strategies to free compression

        partition = FilesPartition()

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
                partition_size = self.__submit_partition_for_compression(partition)
                self.__total_file_size -= partition_size
                if not flush_buffer and self.__total_file_size < self.__target_archive_size:
                    # Not enough files to fill a partition and we don't need to
                    # exhaust the buffer, so break
                    break

        # Compress partial partition
        if partition.contains_files():
            partition_size = self.__submit_partition_for_compression(partition)
            self.__total_file_size -= partition_size

            # NOTE: retain the __total_file_size field
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
            self.__submit_partition_for_compression(partition)
            self.__files = []
            self.__total_file_size = 0

    def __partition_and_compress(self, flush_buffer: bool) -> None:
        if not flush_buffer and self.__total_file_size < self.__target_archive_size:
            # Not flushing and not enough data for a full partition
            return

        if not self.contains_paths():
            # Nothing to compress
            return

        if self.__maintain_file_ordering:
            self.__ordered_partition_and_compress(flush_buffer=flush_buffer)
        else:
            self.__free_partition_and_compress(flush_buffer=flush_buffer)