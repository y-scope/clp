"""Integration tests verifying that the CLP package can be started and stopped."""

import logging
import shutil
from pathlib import Path
from typing import Any

import pytest
import yaml

from tests.utils.config import (
    PackageInstance,
)
from tests.utils.docker_utils import (
    inspect_container_state,
    list_prefixed_containers,
)
from tests.utils.package_utils import (
    CLP_COMPONENT_BASENAMES,
    get_mode_from_dict,
)

package_configurations = pytest.mark.parametrize(
    "test_package_fixture",
    [
        "clp_text_package",
        "clp_json_package",
    ],
)

logger = logging.getLogger(__name__)


@pytest.mark.package
@package_configurations
def test_clp_package(
    request: pytest.FixtureRequest,
    test_package_fixture: str,
) -> None:
    """
    Validate that all of the components of the clp package start up successfully. The package is
    started up in whatever configuration is currently described in clp-config.yml.

    :param request:
    :param test_package_fixture:
    """
    package_instance = request.getfixturevalue(test_package_fixture)
    assert _is_package_running(package_instance)
    assert _is_running_mode_correct(package_instance)


def _is_package_running(package_instance: PackageInstance) -> bool:
    """Ensures the components of `package_instance` are running correctly."""
    mode = package_instance.package_instance_config.mode
    instance_id = package_instance.clp_instance_id

    logger.info(
        "Checking if all components of %s package with instance ID '%s' are running properly...",
        mode,
        instance_id,
    )

    docker_bin = shutil.which("docker")
    if docker_bin is None:
        err_msg = "docker not found in PATH"
        raise RuntimeError(err_msg)

    for component in CLP_COMPONENT_BASENAMES:
        # The container name may have any numeric suffix, and there may be multiple of them.
        prefix = f"clp-package-{instance_id}-{component}-"

        candidates = list_prefixed_containers(docker_bin, prefix)
        if not candidates:
            pytest.fail(f"No component container was found with the prefix '{prefix}'")

        # Inspect each matching container; require that each one is running.
        not_running = []
        for name in candidates:
            desired_state = "running"
            if not inspect_container_state(docker_bin, name, desired_state):
                not_running.append(name)

        if not_running:
            details = ", ".join(not_running)
            pytest.fail(f"Component containers not running: {details}")

    logger.info(
        "All components of the %s package with instance ID '%s' are running properly.",
        mode,
        instance_id,
    )
    return True


def _is_running_mode_correct(package_instance: PackageInstance) -> bool:
    """
    Ensures that the mode intended for the package specified in package_instance matches the mode of
    operation described by the package's shared config file.
    """
    mode = package_instance.package_instance_config.mode
    instance_id = package_instance.clp_instance_id

    logger.info(
        "Checking that the %s package with instance ID '%s' is running in the correct mode...",
        mode,
        instance_id,
    )

    running_mode = _get_running_mode(package_instance)
    intended_mode = package_instance.package_instance_config.mode
    if running_mode != intended_mode:
        err_msg = (
            f"Mode mismatch: the package is running in {running_mode},"
            f" but it should be running in {intended_mode}."
        )
        raise ValueError(err_msg)

    logger.info(
        "The %s package with instance ID '%s' is running in the correct mode.",
        mode,
        instance_id,
    )

    return True


def _get_running_mode(package_instance: PackageInstance) -> str:
    """Gets the current running mode of the clp package."""
    shared_config_dict = _load_shared_config(package_instance.shared_config_file_path)
    return get_mode_from_dict(shared_config_dict)


def _load_shared_config(path: Path) -> dict[str, Any]:
    """Load the content of the shared config file into a dictionary."""
    try:
        with path.open("r", encoding="utf-8") as file:
            shared_config_dict = yaml.safe_load(file)
    except yaml.YAMLError as err:
        err_msg = f"Invalid YAML in shared config {path}: {err}"
        raise ValueError(err_msg) from err
    except OSError as err:
        err_msg = f"Cannot read shared config {path}: {err}"
        raise ValueError(err_msg) from err

    if not isinstance(shared_config_dict, dict):
        err_msg = f"Shared config {path} must be a mapping at the top level."
        raise TypeError(err_msg)

    return shared_config_dict
