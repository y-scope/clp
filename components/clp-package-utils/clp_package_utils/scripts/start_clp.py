#!/usr/bin/env python3
"""Script to start the CLP Package."""

import logging
import pathlib
import sys

import click
from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH
from clp_py_utils.core import resolve_host_path_in_container

from clp_package_utils.cli_utils import RESTART_POLICY
from clp_package_utils.controller import DockerComposeController, get_or_create_instance_id
from clp_package_utils.general import (
    get_clp_home,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_and_load_queue_credentials_file,
    validate_and_load_redis_credentials_file,
    validate_output_storage_config,
    validate_retention_config,
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
@click.option(
    "--restart-policy",
    default="on-failure:3",
    type=RESTART_POLICY,
    help=f"Docker restart policy ({RESTART_POLICY.VALID_POLICIES_STR}).",
)
@click.option(
    "--setup-only",
    is_flag=True,
    help="Validate configuration and prepare directories without starting services.",
)
@click.option(
    "--verbose",
    "-v",
    is_flag=True,
    help="Enable debug logging.",
)
def main(
    config: pathlib.Path,
    restart_policy: str,
    setup_only: bool,
    verbose: bool,
) -> None:
    """Starts the CLP Package."""
    if verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    try:
        # Validate and load config file.
        resolved_config_path = resolve_host_path_in_container(config)
        clp_config = load_config_file(resolved_config_path)
        clp_home = get_clp_home()

        validate_and_load_db_credentials_file(clp_config, clp_home, True)
        if clp_config.queue is not None:
            validate_and_load_queue_credentials_file(clp_config, clp_home, True)
        if clp_config.redis is not None:
            validate_and_load_redis_credentials_file(clp_config, clp_home, True)
        clp_config.validate_logs_input_config(True)
        validate_output_storage_config(clp_config)
        validate_retention_config(clp_config)

        clp_config.validate_api_server()
        clp_config.validate_aws_config_dir(True)
        clp_config.validate_data_dir(True)
        clp_config.validate_logs_dir(True)
        clp_config.validate_tmp_dir(True)
        clp_config.validate_filters_dir(True)
    except Exception:
        logger.exception("Failed to load config.")
        sys.exit(1)

    try:
        # Create necessary directories.
        resolve_host_path_in_container(clp_config.data_directory).mkdir(parents=True, exist_ok=True)
        resolve_host_path_in_container(clp_config.logs_directory).mkdir(parents=True, exist_ok=True)
        resolve_host_path_in_container(clp_config.tmp_directory).mkdir(parents=True, exist_ok=True)
        filters_dir = resolve_host_path_in_container(clp_config.get_filters_directory())
        filters_dir.mkdir(parents=True, exist_ok=True)
        (filters_dir / "staging").mkdir(parents=True, exist_ok=True)
        (filters_dir / "packs").mkdir(parents=True, exist_ok=True)
        resolve_host_path_in_container(clp_config.archive_output.get_directory()).mkdir(
            parents=True, exist_ok=True
        )
        resolve_host_path_in_container(clp_config.stream_output.get_directory()).mkdir(
            parents=True, exist_ok=True
        )
    except Exception:
        logger.exception("Failed to create necessary directories.")
        sys.exit(1)

    try:
        instance_id = get_or_create_instance_id(clp_config)
        controller = DockerComposeController(clp_config, instance_id, restart_policy)
        controller.set_up_env()
        if setup_only:
            logger.info(
                "Completed setup. Services not started because `--setup-only` was specified."
            )
            sys.exit(0)
        controller.start()
    except Exception:
        logger.exception("Failed to start CLP.")
        sys.exit(1)


if "__main__" == __name__:
    main()
