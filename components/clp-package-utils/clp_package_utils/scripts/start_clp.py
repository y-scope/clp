#!/usr/bin/env python3
"""Script to start the CLP Package."""

import logging
import os
import pathlib
import sys
import tempfile

import click
from ruamel.yaml import YAML

from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH, ClpConfig
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

# Values accepted by both CLP_DISABLE_TELEMETRY and DO_NOT_TRACK to disable telemetry.
_TELEMETRY_DISABLE_VALUES = ("1", "true", "yes", "y")


TELEMETRY_NOTICE = """
================================================================================
CLP collects anonymous operational metrics to help improve the software. This
includes: CLP version, OS/architecture, deployment method, bytes ingested,
compression ratios, query volume, and more. It does NOT include: log content,
queries, hostnames, IP addresses, or any other Personally Identifiable
Information (PII).

Telemetry is sent to: https://telemetry.yscope.io
For details, see: https://docs.yscope.com/clp/main/user-docs/reference-telemetry

You can disable metrics at any time by setting the environment variable
CLP_DISABLE_TELEMETRY=true. Network admins can also block
https://telemetry.yscope.io at the firewall level.
================================================================================
"""

TELEMETRY_PROMPT = "Enable anonymous telemetry to help improve CLP? [Y/n] "


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

    _handle_telemetry_consent(clp_config, resolved_config_path)

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


def _handle_telemetry_consent(clp_config: ClpConfig, config_file_path: pathlib.Path) -> None:
    """
    Handles telemetry consent and prompts the user on first run if needed.

    Priority order for handling telemetry preference:
    1. Session-only environment variable overrides. e.g.,
       a. CLP_DISABLE_TELEMETRY=true
       b. DO_NOT_TRACK=true
    2. Config file opt-out
    3. Previously confirmed telemetry preference
    4. First run consent prompt in interactive sessions

    :param config_file_path: for persisting consent.
    """
    clp_disable_val = os.environ.get("CLP_DISABLE_TELEMETRY", "").strip().lower()
    dnt_val = os.environ.get("DO_NOT_TRACK", "").strip().lower()
    if clp_disable_val in _TELEMETRY_DISABLE_VALUES or dnt_val in _TELEMETRY_DISABLE_VALUES:
        clp_config.telemetry.disable = True
        return

    if clp_config.telemetry.disable:
        return

    # Skip prompt if not the first run. i.e., telemetry preference has been confirmed previously.
    instance_id_file = clp_config.logs_directory / "instance-id"
    if resolve_host_path_in_container(instance_id_file).exists():
        return

    if not sys.stdin.isatty():
        return

    print(TELEMETRY_NOTICE)
    try:
        response = input(TELEMETRY_PROMPT).strip().lower()
    except EOFError:
        # e.g., Ctrl+D
        response = "n"

    if response.startswith("n"):
        clp_config.telemetry.disable = True
        _persist_telemetry_disable(config_file_path)


def _persist_telemetry_disable(config_file_path: pathlib.Path) -> None:
    """
    Writes telemetry.disable = true to the config file.

    Uses ruamel.yaml round-trip editing to preserve the user's comments,
    key order, and formatting. Logs a warning on I/O failure rather than
    propagating the exception.
    """
    yaml = YAML()

    try:
        if config_file_path.exists():
            with open(config_file_path, "r") as f:
                config_data = yaml.load(f) or {}
        else:
            config_data = {}

        config_data.setdefault("telemetry", {})["disable"] = True

        # Write atomically: write to temp file, then replace
        fd, temp_path = tempfile.mkstemp(suffix=".yaml", dir=config_file_path.parent)
        try:
            with os.fdopen(fd, "w") as f:
                yaml.dump(config_data, f)
                f.flush()
                os.fsync(f.fileno())
            os.replace(temp_path, config_file_path)
        except BaseException:
            os.unlink(temp_path)
            raise
    except OSError:
        logger.warning(
            "Failed to persist telemetry preference to %s", config_file_path, exc_info=True
        )
