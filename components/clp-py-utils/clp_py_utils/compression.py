import pathlib
import typing

import Levenshtein

# Constants
FILE_GROUPING_MIN_LEVENSHTEIN_RATIO = 0.6


class FileMetadata:
    __slots__ = ("path", "size", "estimated_uncompressed_size")

    def __init__(self, path: pathlib.Path, size: int):
        self.path = path
        self.size = size
        self.estimated_uncompressed_size = size

        filename = path.name
        if any(filename.endswith(extension) for extension in [".gz", ".gzip", ".tgz", ".tar.gz"]):
            self.estimated_uncompressed_size *= 13
        elif any(
            filename.endswith(extension)
            for extension in [".zstd", ".zstandard", ".tar.zstd", ".tar.zstandard"]
        ):
            self.estimated_uncompressed_size *= 8


class FilesPartition:
    def __init__(self):
        self.__files = []
        self.__file_paths = []
        self.__group_ids = []
        self.__st_sizes = []
        self.__total_file_size = 0

    def add_file(self, file_metadata: FileMetadata, group_id: int):
        self.__files.append(file_metadata)
        self.__file_paths.append(str(file_metadata.path))
        self.__group_ids.append(group_id)
        self.__st_sizes.append(file_metadata.size)
        self.__total_file_size += file_metadata.estimated_uncompressed_size

    def add_file_if_empty(self, file_metadata: FileMetadata, group_id: int):
        if file_metadata.estimated_uncompressed_size > 0:
            return False

        self.__files.append(file_metadata)
        self.__file_paths.append(str(file_metadata.path))
        self.__group_ids.append(group_id)
        self.__st_sizes.append(file_metadata.size)
        return True

    def pop_files(self):
        files = self.__files
        file_paths = self.__file_paths
        group_ids = self.__group_ids
        st_sizes = self.__st_sizes
        total_file_size = self.__total_file_size

        self.__files = []
        self.__file_paths = []
        self.__group_ids = []
        self.__st_sizes = []
        self.__total_file_size = 0

        return files, file_paths, group_ids, st_sizes, total_file_size

    def get_total_file_size(self):
        return self.__total_file_size

    def contains_files(self):
        return len(self.__files) > 0


def file_paths_in_same_group(a: pathlib.Path, b: pathlib.Path):
    return Levenshtein.ratio(str(a.name), str(b.name)) >= FILE_GROUPING_MIN_LEVENSHTEIN_RATIO


def group_files_by_similar_filenames(files: typing.List[FileMetadata]):
    groups = []

    if len(files) == 0:
        return groups

    current_group_id = 0
    current_group = {"id": current_group_id, "files": []}
    groups.append(current_group)

    # Sort by filename
    files.sort(key=lambda x: x.path.name)

    file_ix = 0
    file = files[file_ix]
    current_group["files"].append(file)
    last_file_path = file.path

    for file_ix in range(1, len(files)):
        file = files[file_ix]
        if not file_paths_in_same_group(last_file_path, file.path):
            current_group_id += 1
            current_group = {"id": current_group_id, "files": []}
            groups.append(current_group)

        current_group["files"].append(file)
        last_file_path = file.path

    return groups


def validate_path_and_get_info(required_parent_dir: pathlib.Path, path: pathlib.Path):
    file = None
    empty_directory = None

    # Verify that path is absolute
    if not path.is_absolute():
        raise ValueError(f'"{path}" is not absolute.')

    # Verify that path exists
    if not path.exists():
        raise ValueError(f'"{path}" does not exist.')

    # Verify that path points to a file/dir within required parent dir
    try:
        path.resolve().relative_to(required_parent_dir)
    except ValueError:
        raise ValueError(f'"{path}" is not within {required_parent_dir}')

    # Convert path to a path within required parent dir if necessary
    # (e.g., if path is a symlink outside parent dir, but points to a file/dir inside parent dir)
    try:
        path.relative_to(required_parent_dir)
    except ValueError:
        # Not within parent dir, so resolve it
        path = path.resolve()

    if path.is_dir():
        # Check if directory is empty
        if next(path.iterdir(), None) is None:
            empty_directory = str(path)
    else:
        file_size = path.stat().st_size
        file = FileMetadata(path, file_size)

    return file, empty_directory
