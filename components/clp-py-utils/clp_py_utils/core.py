import pathlib
import yaml


def read_yaml_config_file(yaml_config_file_path: pathlib.Path):
    with open(yaml_config_file_path, 'r') as yaml_config_file:
        config = yaml.safe_load(yaml_config_file)
    if config is None:
        raise Exception("Unable to parse configuration from " + str(yaml_config_file_path) + '.')
    return config
