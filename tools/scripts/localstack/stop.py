#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""Script to stop a running LocalStack Docker container."""

import argparse
import logging
import subprocess
import sys

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
logger = logging.getLogger(__name__)


def main() -> int:
    """Main."""
    # To silence Ruff S607
    docker_executable = "docker"

    parser = argparse.ArgumentParser(description="Stop LocalStack Docker container.")
    parser.add_argument(
        "--name",
        type=str,
        default="localstack-clp-dev",
        help="The name of the started LocalStack container (default: %(default)s)",
    )
    args = parser.parse_args()

    result = subprocess.run(
        [docker_executable, "inspect", "-f", "{{.State.Running}}", args.name],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0 or result.stdout.rstrip("\n") != "true":
        logger.warning("Container '%s' doesn't exist. Exit peacefully.", args.name)
        return 0

    localstack_stop_cmd = [
        "docker",
        "stop",
        args.name,
    ]

    result = subprocess.run(localstack_stop_cmd, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        logger.error("Failed to stop LocalStack container:\n%s", result.stderr)
        return result.returncode
    logger.info("LocalStack container stopped successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
