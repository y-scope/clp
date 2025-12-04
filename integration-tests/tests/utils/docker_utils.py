"""Provide utility functions related to the use of Docker during integration tests."""

import shutil
import subprocess


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


def list_running_containers_in_compose_project(project_name: str) -> list[str]:
    """
    Lists running Docker containers that belong to the given Docker Compose project.

    :param project_name:
    :return: List of the names of the running containers that belong to the compose project.
    """
    docker_bin = get_docker_binary_path()

    compose_ps_cmd = [
        docker_bin,
        "compose",
        "--project-name",
        project_name,
        "ps",
        "--format",
        "{{.Name}}",
    ]

    compose_ps_proc = subprocess.run(compose_ps_cmd, stdout=subprocess.PIPE, text=True, check=True)

    container_names: list[str] = []
    for line in (compose_ps_proc.stdout or "").splitlines():
        container_name_candidate = line.strip()
        if container_name_candidate:
            container_names.append(container_name_candidate)

    return container_names
