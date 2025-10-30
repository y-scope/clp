import pathlib

import yaml
from yaml.parser import ParserError

CONTAINER_DIR_FOR_HOST_ROOT = pathlib.Path("/") / "mnt" / "host"


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
    else:
        return default_root / config_path


def read_yaml_config_file(yaml_config_file_path: pathlib.Path):
    with open(yaml_config_file_path, "r") as yaml_config_file:
        try:
            config = yaml.load(yaml_config_file, Loader=yaml.CSafeLoader)
        except ParserError as ex:
            raise ValueError(f"Unable to parse configuration from {yaml_config_file_path}: {ex}")
    return config


def resolve_host_path_in_container(host_path: pathlib.Path) -> pathlib.Path:
    """
    Translates a host path to its container-mount equivalent.

    :param host_path: The host path.
    :return: The translated path.
    """
    host_path = host_path.absolute()
    translated_path = CONTAINER_DIR_FOR_HOST_ROOT / host_path.relative_to("/")

    try:
        if translated_path.is_symlink():
            target_path = (translated_path.parent / translated_path.readlink()).resolve()
            translated_path = CONTAINER_DIR_FOR_HOST_ROOT / target_path.relative_to("/")
    except OSError:
        pass

    return translated_path


def validate_path_could_be_dir(path: pathlib.Path):
    part = path
    while True:
        if part.exists():
            if not part.is_dir():
                raise ValueError(f"{part} is not a directory.")
            return
        part = part.parent
