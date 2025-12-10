#!/usr/bin/env python3
"""Script to stop the CLP Package."""

import logging
import pathlib
import sys

import click
from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

from clp_package_utils.controller import DockerComposeController, get_or_create_instance_id
from clp_package_utils.general import (
    get_clp_home,
    load_config_file,
)

logger = logging.getLogger(__name__)


@click.command()
@click.option(
    "--config",
    "-c",
    type=click.Path(exists=False, path_type=pathlib.Path),
    default=lambda: get_clp_home() / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    help="CLP package configuration file.",
)
def main(config: pathlib.Path) -> None:
    """Stop the CLP Package."""
    try:
        config_file_path = pathlib.Path(config)
        clp_config = load_config_file(resolve_host_path_in_container(config_file_path))
    except Exception:
        logger.exception("Failed to load config.")
        sys.exit(1)

    try:
        instance_id = get_or_create_instance_id(clp_config)
        controller = DockerComposeController(clp_config, instance_id)
        controller.stop()
    except Exception:
        logger.exception("Failed to stop CLP.")
        sys.exit(1)


if "__main__" == __name__:
    main()
