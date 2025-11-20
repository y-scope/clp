"""Utilities that raise pytest assertions on failure."""

import subprocess
from typing import Any

import pytest
from clp_py_utils.clp_config import ClpConfig
from pydantic import ValidationError

from tests.utils.clp_mode_utils import (
    compute_mode_signature,
    get_clp_config_from_mode,
)
from tests.utils.config import PackageInstance
from tests.utils.docker_utils import list_running_containers_with_prefix
from tests.utils.utils import load_yaml_to_dict


def run_and_assert(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess[Any]:
    """
    Runs a command with subprocess and asserts that it succeeds with pytest.

    :param cmd: Command and arguments to execute.
    :param kwargs: Additional keyword arguments passed through to the subprocess.
    :return: The completed process object, for inspection or further handling.
    :raise: pytest.fail if the command exits with a non-zero return code.
    """
    try:
        proc = subprocess.run(cmd, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        pytest.fail(f"Command failed: {' '.join(cmd)}: {e}")
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


def validate_running_mode_correct(package_instance: PackageInstance) -> None:
    """
    Validate that the mode described in the shared config of the instance matches the intended mode
    defined by the instance configuration. Calls pytest.fail if the shared config fails validation
    or if the running mode does not match the intended mode.

    :param package_instance:
    """
    shared_config_dict = load_yaml_to_dict(package_instance.shared_config_file_path)
    try:
        running_config = ClpConfig.model_validate(shared_config_dict)
    except ValidationError as err:
        pytest.fail(f"Shared config failed validation: {err}")

    intended_config = get_clp_config_from_mode(package_instance.package_config.mode_name)

    running_signature = compute_mode_signature(running_config)
    intended_signature = compute_mode_signature(intended_config)

    if running_signature != intended_signature:
        pytest.fail("Mode mismatch: running configuration does not match intended configuration.")
