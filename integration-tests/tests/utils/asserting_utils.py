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
from tests.utils.logging_utils import construct_log_err_msg
from tests.utils.utils import load_yaml_to_dict

logger = logging.getLogger(__name__)


def run_and_assert(request: pytest.FixtureRequest, cmd: list[str], **kwargs: Any) -> None:
    """
    Runs a command with subprocess.

    :param request: Pytest fixture request.
    :param cmd: Command and arguments to execute.
    :param kwargs: Additional keyword arguments passed through to the subprocess.
    :raise: Propagates `subprocess.run`'s errors.
    """
    log_file_path = Path(request.config.getini("log_file_path"))
    with log_file_path.open("ab") as log_file:
        log_debug_msg = f"Now running command: {cmd}"
        logger.debug(log_debug_msg)
        subprocess.run(cmd, stdout=log_file, stderr=log_file, check=True, **kwargs)


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

    err_msg = f"Component validation failed for the {mode_name} package test."

    missing_components = required_components - running_services
    if missing_components:
        err_msg += f" Missing components: {missing_components}."

    unexpected_components = running_services - required_components
    if unexpected_components:
        err_msg += f" Unexpected services: {unexpected_components}."

    logger.error(construct_log_err_msg(err_msg))
    pytest.fail(err_msg)


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
    logger.info(
        "Validating that the '%s' package is running in the correct configuration...", mode_name
    )

    shared_config_dict = load_yaml_to_dict(package_instance.shared_config_file_path)
    try:
        running_config = ClpConfig.model_validate(shared_config_dict)
    except ValidationError as err:
        err_msg = f"The shared config file could not be validated: {err}"
        logger.error(construct_log_err_msg(err_msg))
        pytest.fail(err_msg)

    intended_config = package_instance.package_config.clp_config

    if not compare_mode_signatures(intended_config, running_config):
        err_msg = f"Mode validation failed for the {mode_name} package test."
        logger.error(construct_log_err_msg(err_msg))
        pytest.fail(err_msg)
