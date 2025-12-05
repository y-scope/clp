"""Utilities that raise pytest assertions on failure."""

import logging
import shlex
import subprocess
from typing import Any

import pytest

from tests.utils.config import PackageInstance
from tests.utils.docker_utils import list_running_containers_in_compose_project
from tests.utils.utils import strip_prefix, strip_regex_suffix

logger = logging.getLogger(__name__)


def run_and_assert(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess[Any]:
    """
    Runs a command with subprocess and asserts that it succeeds with pytest.

    :param cmd: Command and arguments to execute.
    :param kwargs: Additional keyword arguments passed through to the subprocess.
    :return: The completed process object, for inspection or further handling.
    :raise: pytest.fail if the command exits with a non-zero return code.
    """
    logger.info("Running command: %s", shlex.join(cmd))

    try:
        proc = subprocess.run(cmd, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        pytest.fail(f"Command failed: {' '.join(cmd)}: {e}")
    except subprocess.TimeoutExpired as e:
        pytest.fail(f"Command timed out: {' '.join(cmd)}: {e}")
    return proc


def validate_package_running(package_instance: PackageInstance) -> None:
    """
    Validate that the given package instance is running and that its Compose project has exactly the
    expected set of component containers.

    :param package_instance:
    """
    # Get list of containers currently running in the Compose project.
    instance_id = package_instance.clp_instance_id
    project_name = f"clp-package-{instance_id}"
    running_containers = list_running_containers_in_compose_project(project_name)

    # Construct list of unique running component names and sort alphabetically.
    running_components = []
    prefix = f"{project_name}-"
    regex_suffix = r"-\d+"
    for container_name in running_containers:
        component_name = strip_prefix(container_name, prefix)
        component_name = strip_regex_suffix(component_name, regex_suffix)
        if component_name not in running_components:
            running_components.append(component_name)
    running_components = sorted(running_components)

    # Get list of required components and sort alphabetically.
    required_components = sorted(package_instance.package_config.component_list)

    if required_components != running_components:
        fail_msg = "Component mismatch."

        missing_components = set(required_components) - set(running_components)
        if missing_components:
            fail_msg += f" Missing components: {missing_components}."

        unexpected_components = set(running_components) - set(required_components)
        if unexpected_components:
            fail_msg += f" Unexpected components: {unexpected_components}."

        pytest.fail(fail_msg)
