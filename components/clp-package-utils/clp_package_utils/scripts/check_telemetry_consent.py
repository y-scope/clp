#!/usr/bin/env python3
"""Check telemetry consent and prompt the user on first run.

This helper is called from the host shell script (start-clp.sh) before launching
any Docker containers, so stdin is a real TTY.
"""

import argparse
import os
import sys
import tempfile
from pathlib import Path

import yaml

PROMPT = """
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

CONTAINER_LOGS_DIR_PATH = Path("var") / "log"


def _get_config_path(config: Path | None) -> Path:
    """Resolve the config file path, defaulting to etc/clp-config.yaml."""
    if config is not None:
        return config
    return Path("etc") / "clp-config.yaml"


def _get_instance_id_path(clp_home: Path | None, config_path: Path) -> Path:
    """Resolve the instance-id file path."""
    if clp_home is not None:
        return clp_home / CONTAINER_LOGS_DIR_PATH / "instance-id"
    # Infer CLP_HOME from config file location
    return config_path.parent.parent / CONTAINER_LOGS_DIR_PATH / "instance-id"


def _update_config_file_telemetry(config_path: Path, disable: bool) -> None:
    """Persist telemetry.disable to the config file."""
    config_data = {}
    if config_path.exists():
        with open(config_path, "r") as f:
            config_data = yaml.safe_load(f) or {}

    telemetry = config_data.get("telemetry", {})
    telemetry["disable"] = disable
    config_data["telemetry"] = telemetry

    # Write atomically: write to temp file, then replace
    fd, temp_path = tempfile.mkstemp(suffix=".yaml", dir=config_path.parent)
    try:
        with os.fdopen(fd, "w") as f:
            yaml.safe_dump(config_data, f, default_flow_style=False)
            f.flush()
            os.fsync(f.fileno())
        os.replace(temp_path, config_path)
    except:
        os.unlink(temp_path)
        raise


def main() -> None:
    parser = argparse.ArgumentParser(description="Check telemetry consent and prompt on first run.")
    parser.add_argument(
        "--config",
        "-c",
        type=Path,
        default=None,
        help="Path to clp-config.yaml (default: etc/clp-config.yaml).",
    )
    parser.add_argument(
        "--clp-home",
        type=Path,
        default=None,
        help="CLP home directory (used to locate var/log/instance-id).",
    )
    args = parser.parse_args()

    config_path = _get_config_path(args.config)
    instance_id_path = _get_instance_id_path(args.clp_home, config_path)

    # Priority 1: Environment variables
    disable_env = os.environ.get("CLP_DISABLE_TELEMETRY", "").strip().lower()
    if disable_env in ("true", "1"):
        return  # Already disabled via env var

    if os.environ.get("DO_NOT_TRACK", "").strip().lower() in ("1", "true", "yes"):
        return  # Already disabled via env var

    # Priority 2: Config file already has explicit setting
    if config_path.exists():
        with open(config_path, "r") as f:
            config_data = yaml.safe_load(f) or {}
        telemetry = config_data.get("telemetry", {})
        if telemetry.get("disable") is True:
            return  # Already disabled via config

    # Priority 3: First-run prompt (only if instance-id doesn't exist and stdin is a TTY)
    if instance_id_path.exists():
        return  # Not first run, skip

    if not sys.stdin.isatty():
        return  # Not interactive, default to enabled

    print(PROMPT)
    try:
        response = input().strip().lower()
    except EOFError:
        response = ""

    if response == "n":
        _update_config_file_telemetry(config_path, disable=True)
        print("Telemetry has been disabled. You can re-enable it in your config file.")


if "__main__" == __name__:
    main()
