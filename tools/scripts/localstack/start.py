#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""Script to start a LocalStack Docker container."""

import argparse
import logging
import subprocess
import sys

_LOCALSTACK_IMAGE: str = "localstack/localstack:latest"

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
logger = logging.getLogger(__name__)


def main() -> int:
    """Main."""
    parser = argparse.ArgumentParser(description="Start LocalStack Docker container.")
    parser.add_argument(
        "--name",
        type=str,
        default="localstack-clp-dev",
        help="The name of the started LocalStack container (default: %(default)s)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=4566,
        help="The port to expose LocalStack on (default: %(default)d)",
    )
    args = parser.parse_args()

    # Silence Ruff S607: the absolute path of the Docker binary may vary depending on the
    # installation method.
    docker_executable = "docker"

    result = subprocess.run(
        [docker_executable, "inspect", "-f", "{{.State.Running}}", args.name],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode == 0 and result.stdout.rstrip("\n") == "true":
        logger.warning("Container '%s' already exists.", args.name)
        return 1

    logger.info("Starting LocalStack container '%s' on port %d", args.name, args.port)
    logger.info("Pulling latest LocalStack image.")
    result = subprocess.run(
        [docker_executable, "pull", _LOCALSTACK_IMAGE], capture_output=True, text=True, check=False
    )
    if result.returncode != 0:
        logger.error("Failed to pull the latest LocalStack image:\n%s", result.stderr)
        return result.returncode
    logger.info("Successfully pulled latest LocalStack image.")

    localstack_start_cmd = [
        "docker",
        "run",
        "--rm",
        "--detach",
        "--name",
        args.name,
        "--publish",
        f"{args.port}:4566",
        _LOCALSTACK_IMAGE,
    ]

    result = subprocess.run(localstack_start_cmd, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        logger.error("Failed to start LocalStack container:\n%s", result.stderr)
        return result.returncode
    logger.info("LocalStack container started successfully with ID: %s", result.stdout.strip())
    return 0


if __name__ == "__main__":
    sys.exit(main())
