"""Utilities that raise pytest assertions on failure."""

import logging
import subprocess
from pathlib import Path
from typing import Any

import pytest
from clp_py_utils.clp_config import ClpConfig
from pydantic import ValidationError

from tests.utils.clp_mode_utils import compare_mode_signatures
from tests.utils.config import PackageInstance
from tests.utils.docker_utils import list_running_services_in_compose_project
from tests.utils.utils import load_yaml_to_dict

logger = logging.getLogger(__name__)


def run_and_assert(request: pytest.FixtureRequest, cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess[Any]:
    """
    Runs a command with subprocess and asserts that it succeeds with pytest.

    :param cmd: Command and arguments to execute.
    :param kwargs: Additional keyword arguments passed through to the subprocess.
    :return: The completed process object, for inspection or further handling.
    :raise: pytest.fail if the command exits with a non-zero return code.
    """
    log_file_path = Path(request.config.getini("log_file_path"))
    with open(log_file_path, "ab") as log_file:
        try:
            proc = subprocess.run(
                cmd,
                stdout=log_file,
                stderr=log_file,
                check=True,
                **kwargs
            )
        except subprocess.CalledProcessError:
            fail_msg = (
                "Called process failed. Check error summary at the end of the test for more information."
            )
            logger.error(fail_msg)
            pytest.fail(fail_msg)
        except subprocess.TimeoutExpired:
            fail_msg = (
                "Called process timed out. Check error summary at the end of the test for more information."
            )
            logger.error(fail_msg)
            pytest.fail(fail_msg)
        return proc


def validate_package_running(package_instance: PackageInstance) -> None:
    """
    Validate that the given package instance is running by checking that the set of services running
    in the Compose project exactly matches the list of required components.

    :param package_instance:
    :raise pytest.fail: if the sets of running services and required components do not match.
    """
    mode_name = package_instance.package_config.mode_name
    logger.info("Validating that all components of the '%s' package are running...", mode_name)

    # Get list of services currently running in the Compose project.
    instance_id = package_instance.clp_instance_id
    project_name = f"clp-package-{instance_id}"
    running_services = set(list_running_services_in_compose_project(project_name))

    # Compare with list of required components.
    required_components = set(package_instance.package_config.component_list)
    if required_components == running_services:
        return

    cmd_fail_msg = "Component mismatch."

    missing_components = required_components - running_services
    if missing_components:
        cmd_fail_msg += f"\nMissing components: {missing_components}."

    unexpected_components = running_services - required_components
    if unexpected_components:
        cmd_fail_msg += f"\nUnexpected services: {unexpected_components}."

    pytest.fail(cmd_fail_msg)


def validate_running_mode_correct(package_instance: PackageInstance) -> None:
    """
    Validate that the mode described in the shared config of the instance matches the intended mode
    defined by the instance configuration. Calls pytest.fail if the shared config fails validation
    or if the running mode does not match the intended mode.

    :param package_instance:
    :raise: Propagates `load_yaml_to_dict`'s errors.
    :raise pytest.fail: if the ClpConfig object cannot be validated.
    :raise pytest.fail: if the running ClpConfig does not match the intended ClpConfig.
    """
    mode_name = package_instance.package_config.mode_name
    logger.info("Validating that the '%s' package is running in the correct configuration...", mode_name)

    shared_config_dict = load_yaml_to_dict(package_instance.shared_config_file_path)
    try:
        running_config = ClpConfig.model_validate(shared_config_dict)
    except ValidationError as err:
        pytest.fail(f"Shared config failed validation: {err}")

    intended_config = package_instance.package_config.clp_config

    if not compare_mode_signatures(intended_config, running_config):
        pytest.fail("Mode mismatch: running configuration does not match intended configuration.")
