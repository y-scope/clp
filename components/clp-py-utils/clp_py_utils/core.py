import pathlib

import yaml
from yaml.parser import ParserError


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
            config = yaml.safe_load(yaml_config_file)
        except ParserError as ex:
            raise ValueError(f"Unable to parse configuration from {yaml_config_file_path}: {ex}")
    return config


def validate_path_could_be_dir(path: pathlib.Path):
    part = path
    while True:
        if part.exists():
            if not part.is_dir():
                raise ValueError(f"{part} is not a directory.")
            return
        part = part.parent
