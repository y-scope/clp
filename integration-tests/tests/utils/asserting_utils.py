"""Utilities that raise pytest assertions on failure."""

import logging
import shlex
import subprocess
from typing import Any

import pytest

from tests.utils.config import PackageInstance
from tests.utils.docker_utils import list_running_containers_with_prefix

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
    Validate that the given package instance is running. Each required component must have at least
    one running container whose name matches the expected prefix. Calls pytest.fail on the first
    missing component.

    :param package_instance:
    """
    instance_id = package_instance.clp_instance_id
    required_components = package_instance.package_config.component_list

    for component in required_components:
        prefix = f"clp-package-{instance_id}-{component}-"
        running_matches = list_running_containers_with_prefix(prefix)
        if len(running_matches) == 0:
            pytest.fail(
                f"No running container found for component '{component}' "
                f"(expected name prefix '{prefix}')."
            )
