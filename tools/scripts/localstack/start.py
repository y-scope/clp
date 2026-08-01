#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""Script to start a LocalStack Docker container."""

import argparse
import logging
import socket
import subprocess
import sys
import time
import urllib.request
from http import HTTPStatus
from pathlib import Path

# Lock `localstack` image version to 4.14 as a workaround for #2118.
_LOCALSTACK_IMAGE: str = "localstack/localstack:4.14"

# Docker creates this marker in every container it starts, so its presence means this script is
# itself running inside a container.
_DOCKER_ENV_MARKER: Path = Path("/.dockerenv")

# Silence Ruff S607: the absolute path of the Docker binary may vary depending on the installation
# method.
_DOCKER_EXECUTABLE: str = "docker"

_READINESS_TIMEOUT_SECS: float = 120
_READINESS_POLL_INTERVAL_SECS: float = 1
_READINESS_REQUEST_TIMEOUT_SECS: float = 5

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
logger = logging.getLogger(__name__)


def _get_enclosing_container_id() -> str | None:
    """
    Resolves the ID of the container this script is running in, if any.

    On a runner that mounts the host's Docker socket, containers started by this script are siblings
    on the host rather than children of the runner, so a published port lands in the host's network
    namespace instead of the runner's and is unreachable from here. Joining the runner's network
    namespace avoids that, but requires identifying the runner's container.

    :return: The container ID, or None if this script isn't running in a container that the Docker
        daemon can resolve.
    """
    if not _DOCKER_ENV_MARKER.exists():
        return None

    # Docker defaults a container's hostname to its ID, and Compose sets it to the container's name;
    # the daemon resolves either.
    hostname = socket.gethostname()
    result = subprocess.run(
        [_DOCKER_EXECUTABLE, "inspect", "-f", "{{.Id}}", hostname],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        logger.warning(
            "Running inside a container, but the Docker daemon can't resolve it from hostname"
            " '%s'. Falling back to publishing a port, which is unreachable from here if the"
            " daemon is the host's. Docker error:\n%s",
            hostname,
            result.stderr.strip(),
        )
        return None

    return result.stdout.strip()


def _wait_until_ready(port: int) -> bool:
    """
    Waits until LocalStack answers on the loopback address.

    :param port: The port LocalStack is expected to listen on.
    :return: Whether LocalStack became ready before timing out.
    """
    # `localhost` can resolve to `::1` alone, which LocalStack doesn't bind, so address it by its
    # IPv4 loopback address instead.
    health_url = f"http://127.0.0.1:{port}/_localstack/health"
    deadline = time.monotonic() + _READINESS_TIMEOUT_SECS
    while time.monotonic() < deadline:
        try:
            # Silence Ruff S310: `health_url` is built above from a literal `http` scheme.
            with urllib.request.urlopen(  # noqa: S310
                health_url, timeout=_READINESS_REQUEST_TIMEOUT_SECS
            ) as response:
                if HTTPStatus.OK == response.status:
                    return True
        except OSError as e:
            logger.debug("LocalStack isn't ready yet: %s", e)
        time.sleep(_READINESS_POLL_INTERVAL_SECS)

    return False


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

    result = subprocess.run(
        [_DOCKER_EXECUTABLE, "inspect", "-f", "{{.State.Running}}", args.name],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode == 0 and result.stdout.rstrip("\n") == "true":
        logger.warning("Container '%s' already exists.", args.name)
        return 1

    logger.info("Starting LocalStack container '%s' on port %d", args.name, args.port)
    logger.info("Pulling LocalStack image.")
    result = subprocess.run(
        [_DOCKER_EXECUTABLE, "pull", _LOCALSTACK_IMAGE], capture_output=True, text=True, check=False
    )
    if result.returncode != 0:
        logger.error("Failed to pull LocalStack image:\n%s", result.stderr)
        return result.returncode
    logger.info("Successfully pulled LocalStack image.")

    localstack_start_cmd = [
        _DOCKER_EXECUTABLE,
        "run",
        "--rm",
        "--detach",
        "--name",
        args.name,
    ]
    enclosing_container_id = _get_enclosing_container_id()
    if enclosing_container_id is None:
        localstack_start_cmd += ["--publish", f"{args.port}:4566"]
    else:
        # Publishing a port is incompatible with this network mode, so bind LocalStack's gateway
        # directly to `--port` instead.
        logger.info("Joining the network namespace of container '%s'.", enclosing_container_id)
        localstack_start_cmd += [
            "--network",
            f"container:{enclosing_container_id}",
            "--env",
            f"GATEWAY_LISTEN=0.0.0.0:{args.port}",
        ]
    localstack_start_cmd.append(_LOCALSTACK_IMAGE)

    result = subprocess.run(localstack_start_cmd, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        logger.error("Failed to start LocalStack container:\n%s", result.stderr)
        return result.returncode
    logger.info("LocalStack container started successfully with ID: %s", result.stdout.strip())

    if not _wait_until_ready(args.port):
        logger.error(
            "LocalStack didn't become ready on port %d within %g seconds.",
            args.port,
            _READINESS_TIMEOUT_SECS,
        )
        return 1
    logger.info("LocalStack is ready on port %d.", args.port)

    return 0


if __name__ == "__main__":
    sys.exit(main())
