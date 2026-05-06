#!/usr/bin/env python3
"""Script to start the CLP Package."""

import logging
import os
import pathlib
import sys
import tempfile

import click
import yaml
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

TELEMETRY_PROMPT = """
================================================================================
CLP collects anonymous operational metrics to help improve the software.
This includes: CLP version, OS/architecture, deployment method, bytes
ingested, compression ratios, and query volume. It does NOT include:
log content, queries, hostnames, IP addresses, or any personally
identifiable information.

Metrics are sent via OTLP to: https://telemetry.yscope.io:4318
For details, see: https://docs.yscope.com/clp/main/user-docs/reference-telemetry

You can disable metrics at any time by setting CLP_DISABLE_TELEMETRY=true
or by blocking https://telemetry.yscope.io:4318 at the network level.

Enable anonymous telemetry to help improve CLP? [Y/n]
================================================================================
"""


def _check_telemetry_consent(clp_config, config_file_path: pathlib.Path) -> None:
    """
    Checks telemetry consent and prompts the user on first run if needed.

    Priority order for disabling telemetry:
    1. CLP_DISABLE_TELEMETRY or DO_NOT_TRACK environment variables
    2. telemetry.disable explicitly set in config file
    3. First-run interactive prompt (persists choice to config)

    :param clp_config: The loaded ClpConfig object.
    :param config_file_path: Path to the config file (for persisting consent).
    """
    # Priority 1: Environment variables
    disable_env = os.environ.get("CLP_DISABLE_TELEMETRY", "").strip().lower()
    if disable_env in ("true", "1"):
        clp_config.telemetry.disable = True
        return

    if os.environ.get("DO_NOT_TRACK", "").strip().lower() in ("1", "true", "yes"):
        clp_config.telemetry.disable = True
        return

    # Priority 2: Config file already has explicit setting
    if clp_config.telemetry.disable:
        return

    # Priority 3: First-run prompt
    instance_id_file = clp_config.logs_directory / "instance-id"
    if resolve_host_path_in_container(instance_id_file).exists():
        return  # Not first run, skip prompt

    # First run — show prompt if interactive
    if sys.stdin.isatty():
        print(TELEMETRY_PROMPT)
        try:
            response = input().strip().lower()
        except EOFError:
            response = ""

        if response.startswith("n"):
            clp_config.telemetry.disable = True
            # Persist the choice to the config file
            try:
                _update_config_file_telemetry(config_file_path, disable=True)
                logger.info(
                    "Telemetry has been disabled. You can re-enable it in %s", config_file_path
                )
            except OSError:
                logger.warning(
                    "Failed to persist telemetry preference to %s", config_file_path, exc_info=True
                )
                logger.info("Telemetry is disabled for this run but the config write failed.")

    # Non-interactive: default to enabled (no config write needed)


def _update_config_file_telemetry(config_file_path: pathlib.Path, disable: bool) -> None:
    """
    Updates the telemetry.disable setting in the config file.

    :param config_file_path: Path to the config file.
    :param disable: Whether to disable telemetry.
    """
    config_data = {}

    if config_file_path.exists():
        with open(config_file_path, "r") as f:
            config_data = yaml.safe_load(f) or {}

    telemetry = config_data.get("telemetry", {})
    telemetry["disable"] = disable
    config_data["telemetry"] = telemetry

    # Write atomically: write to temp file, then replace
    fd, temp_path = tempfile.mkstemp(suffix=".yaml", dir=config_file_path.parent)
    try:
        with os.fdopen(fd, "w") as f:
            yaml.safe_dump(config_data, f, default_flow_style=False)
            f.flush()
            os.fsync(f.fileno())
        os.replace(temp_path, config_file_path)
    except:
        os.unlink(temp_path)
        raise


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
    except Exception:
        logger.exception("Failed to load config.")
        sys.exit(1)

    # Check telemetry consent (may prompt user on first run)
    _check_telemetry_consent(clp_config, resolved_config_path)

    try:
        # Create necessary directories.
        resolve_host_path_in_container(clp_config.data_directory).mkdir(parents=True, exist_ok=True)
        resolve_host_path_in_container(clp_config.logs_directory).mkdir(parents=True, exist_ok=True)
        resolve_host_path_in_container(clp_config.tmp_directory).mkdir(parents=True, exist_ok=True)
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
