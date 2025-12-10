import argparse
import logging
import pathlib
import sys

from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH
from clp_py_utils.core import resolve_host_path_in_container

from clp_package_utils.controller import DockerComposeController, get_or_create_instance_id
from clp_package_utils.general import (
    get_clp_home,
    load_config_file,
)

logger = logging.getLogger(__file__)


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Stops CLP")
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )

    parsed_args = args_parser.parse_args(argv[1:])

    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(resolve_host_path_in_container(config_file_path))
    except:
        logger.exception("Failed to load config.")
        return -1

    try:
        instance_id = get_or_create_instance_id(clp_config)
        controller = DockerComposeController(clp_config, instance_id)
        controller.stop()
    except:
        logger.exception("Failed to stop CLP.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
