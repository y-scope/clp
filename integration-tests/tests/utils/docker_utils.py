"""Provide utility functions related to the use of Docker during integration tests."""

import re
import shutil
import subprocess
from enum import Enum


class DockerStatus(str, Enum):
    """Enum of possible Docker Statuses."""

    created = "created"
    restarting = "restarting"
    running = "running"
    removing = "removing"
    paused = "paused"
    exited = "exited"
    dead = "dead"


def get_docker_binary_path() -> str:
    """
    Finds the absolute path to the Docker client in the current PATH.

    :return: Absolute path to the Docker binary.
    :raise RuntimeError: docker is not found on PATH.
    """
    docker_bin = shutil.which("docker")
    if docker_bin is None:
        err_msg = "docker not found in PATH"
        raise RuntimeError(err_msg)
    return docker_bin


def list_running_containers_with_prefix(prefix: str) -> list[str]:
    """
    Lists running Docker containers whose names begin with `prefix` and end with one or more digits.

    :param prefix:
    :return: List of running container names that match the pattern.
    :raise: Propagates `subprocess.run`'s errors.
    """
    docker_bin = get_docker_binary_path()

    ps_proc = subprocess.run(
        [
            docker_bin,
            "ps",
            "--format",
            "{{.Names}}",
            "--filter",
            f"name={prefix}",
            "--filter",
            "status=running",
        ],
        capture_output=True,
        text=True,
        check=True,
    )

    matches: list[str] = []
    for line in (ps_proc.stdout or "").splitlines():
        name_candidate = line.strip()
        if re.fullmatch(re.escape(prefix) + r"\d+", name_candidate):
            matches.append(name_candidate)

    return matches
