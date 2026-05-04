"""Provide utility functions related to the use of Docker during integration tests."""

import pytest

from tests.utils.classes import ExternalAction
from tests.utils.logging_utils import format_action_failure_msg
from tests.utils.utils import get_binary_path


def list_running_services_in_compose_project(project_name: str) -> list[str]:
    """
    Lists running Docker services that belong to the given Docker Compose project.

    :param project_name:
    :return: List of the running services that belong to the specified Docker Compose project.
    :raise pytest.fail: if `docker compose ps` returns a non-zero exit code.
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

    compose_ps_action = ExternalAction(cmd=compose_ps_cmd)
    if compose_ps_action.completed_proc.returncode != 0:
        pytest.fail(
            format_action_failure_msg(
                f"`docker compose ps` failed for project `{project_name}`.",
                compose_ps_action,
            )
        )

    service_names: list[str] = []
    for line in compose_ps_action.completed_proc.stdout.splitlines():
        service_name_candidate = line.strip()
        if service_name_candidate:
            service_names.append(service_name_candidate)

    return service_names
