"""Utilities that raise pytest assertions on failure."""

import logging
import shlex
import subprocess
from typing import Any

import pytest

from tests.utils.config import PackageInstance
from tests.utils.docker_utils import list_running_services_in_compose_project

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
    Validate that the given package instance is running by checking that the set of services running
    in the Compose project exactly matches the list of required components.

    :param package_instance:
    """
    # Get list of services currently running in the Compose project.
    instance_id = package_instance.clp_instance_id
    project_name = f"clp-package-{instance_id}"
    running_services = list_running_services_in_compose_project(project_name)

    # Compare with list of required components.
    required_components = package_instance.package_config.component_list
    if set(required_components) == set(running_services):
        return

    fail_msg = "Component mismatch."

    missing_components = set(required_components) - set(running_services)
    if missing_components:
        fail_msg += f" Missing components: {missing_components}."

    unexpected_components = set(running_services) - set(required_components)
    if unexpected_components:
        fail_msg += f" Unexpected services: {unexpected_components}."

    pytest.fail(fail_msg)
