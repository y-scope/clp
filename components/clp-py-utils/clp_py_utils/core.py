import os
import pathlib

import yaml
from yaml.parser import ParserError

CONTAINER_DIR_FOR_HOST_ROOT = pathlib.Path("/") / "mnt" / "host"


class FileMetadata:
    __slots__ = ("estimated_uncompressed_size", "path", "size")

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


def get_config_value(config, key):
    """
    Gets a value from the given dictionary using a dot-separated configuration
    key, where each dot represents a deeper dictionary. NOTE: This method does
    not support keys that contain dots since that is indistinguishable from a
    deeper dictionary.

    :param config:
    :param key:
    """
    singular_keys = key.split(".")
    current_config = config
    for current_key in singular_keys:
        current_config = current_config[current_key]
    return current_config


def make_config_path_absolute(default_root: pathlib.Path, config_path: pathlib.Path):
    """
    Turns relative paths into absolute paths by prefixing them with the
    default_root

    :param default_root:
    :param config_path:
    """
    if config_path.is_absolute():
        return config_path
    return default_root / config_path


def read_yaml_config_file(yaml_config_file_path: pathlib.Path):
    with open(yaml_config_file_path, "r") as yaml_config_file:
        try:
            config = yaml.load(yaml_config_file, Loader=yaml.CSafeLoader)
        except ParserError as ex:
            raise ValueError(f"Unable to parse configuration from {yaml_config_file_path}: {ex}")
    return config


def resolve_host_path(host_path: pathlib.Path) -> pathlib.Path:
    """
    Resolves a host path:

    - Tilde (~) paths are expanded before processing.
    - Relative paths are resolved relative to the current working directory on the host.
    - Symlinks are resolved recursively until a non-symlink path is reached or a cycle is detected.

    :param host_path: The host path.
    :return: The resolved host path.
    """
    resolved_container_path = resolve_host_path_in_container(host_path)
    return pathlib.Path("/") / resolved_container_path.relative_to(CONTAINER_DIR_FOR_HOST_ROOT)


def resolve_host_path_in_container(host_path: pathlib.Path) -> pathlib.Path:
    """
    Resolves a host path and translates it into its container-mount equivalent absolute path. See
    `resolve_host_path` for details about the resolution.

    :param host_path: The host path.
    :return: The translated path (with /mnt/host prefix).
    """
    host_path = host_path.expanduser()

    if not host_path.is_absolute():
        pwd_host = os.getenv("CLP_PWD_HOST")
        if pwd_host is None:
            raise ValueError("Relative host path provided but CLP_PWD_HOST is not set.")
        host_path = pathlib.Path(pwd_host) / host_path
        host_path = host_path.absolute()

    resolved_path = _resolve_symlinks_in_path(host_path)
    if resolved_path is not None:
        return resolved_path

    return CONTAINER_DIR_FOR_HOST_ROOT / host_path.relative_to("/")


def validate_path_could_be_dir(path: pathlib.Path):
    part = path
    while True:
        if part.exists():
            if not part.is_dir():
                raise ValueError(f"{part} is not a directory.")
            return
        part = part.parent


def _resolve_symlinks_in_path(host_path: pathlib.Path) -> pathlib.Path | None:
    """
    Resolves symlinks in a path by walking through each component and following symlinks.

    :param host_path: The host path.
    :return: The resolved path with all symlinks followed, or None on error.
    """
    try:
        visited_symlink_inodes = set()
        current_path = CONTAINER_DIR_FOR_HOST_ROOT
        for part in host_path.relative_to("/").parts:
            current_path = current_path / part

            while current_path.is_symlink():
                stat = current_path.lstat()
                if stat.st_ino in visited_symlink_inodes:
                    break
                visited_symlink_inodes.add(stat.st_ino)

                link_target = current_path.readlink()
                if link_target.is_absolute():
                    current_path = CONTAINER_DIR_FOR_HOST_ROOT / link_target.relative_to("/")
                else:
                    # If the symlink points to a relative path, resolve it relative to the symlink's
                    # parent.
                    current_path = (current_path.parent / link_target).resolve()

        return current_path
    except OSError:
        # Ignore if reading the symlink fails (e.g., broken link or permission error).
        return None
