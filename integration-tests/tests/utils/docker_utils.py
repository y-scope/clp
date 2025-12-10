"""Provide utility functions related to the use of Docker during integration tests."""

import re
import subprocess

from tests.utils.utils import get_binary_path


def list_running_services_in_compose_project(project_name: str) -> list[str]:
    """
    Lists running Docker services that belong to the given Docker Compose project.

    :param project_name:
    :return: List of the running services that belong to the specified Docker Compose project.
    """
    docker_bin = get_binary_path("docker")

    # fmt: off
    compose_ps_cmd = [
        docker_bin,
        "compose",
        "--project-name", project_name,
        "ps",
        "--format", "{{.Service}}",
    ]
    # fmt: on

    compose_ps_proc = subprocess.run(compose_ps_cmd, stdout=subprocess.PIPE, text=True, check=True)

    service_names: list[str] = []
    for line in (compose_ps_proc.stdout or "").splitlines():
        service_name_candidate = line.strip()
        if service_name_candidate:
            service_names.append(service_name_candidate)

    return service_names


def list_running_containers_with_prefix(prefix: str) -> list[str]:
    """
    Lists running Docker containers whose names begin with `prefix` and end with one or more digits.

    :param prefix:
    :return: List of running container names that match the pattern.
    """
    docker_bin = get_binary_path("docker")

    # fmt: off
    docker_ps_cmd = [
        docker_bin,
        "ps",
        "--format", "{{.Names}}",
        "--filter", f"name={prefix}",
    ]
    # fmt: on
    ps_proc = subprocess.run(docker_ps_cmd, stdout=subprocess.PIPE, text=True, check=True)

    matches: list[str] = []
    for line in (ps_proc.stdout or "").splitlines():
        name_candidate = line.strip()
        if re.fullmatch(re.escape(prefix) + r"\d+", name_candidate):
            matches.append(name_candidate)

    return matches
