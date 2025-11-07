"""Provides utility functions related to the clp-package used across `integration-tests`."""

import shutil
import subprocess

from clp_py_utils.clp_config import (
    CLPConfig,
)
from pydantic import ValidationError

from tests.utils.clp_mode_utils import compute_mode_signature, get_clp_config_from_mode
from tests.utils.config import (
    PackageConfig,
    PackageInstance,
)
from tests.utils.docker_utils import (
    inspect_container_state,
    list_prefixed_containers,
)
from tests.utils.utils import load_yaml_to_dict


def start_clp_package(package_config: PackageConfig) -> None:
    """Start an instance of the CLP package."""
    start_script_path = package_config.start_script_path
    try:
        # fmt: off
        start_cmd = [
            str(start_script_path),
            "--config",
            str(package_config.temp_config_file_path)
        ]
        # fmt: on
        subprocess.run(start_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to start an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from e


def stop_clp_package(instance: PackageInstance) -> None:
    """Stop an instance of the CLP package."""
    package_config = instance.package_config
    stop_script_path = package_config.stop_script_path
    try:
        # fmt: off
        stop_cmd = [
            stop_script_path
        ]
        # fmt: on
        subprocess.run(stop_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to stop an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from e


def is_package_running(package_instance: PackageInstance) -> tuple[bool, str | None]:
    """
    Checks that the `package_instance` is running properly by examining each of its component
    containers. Records which containers are not found or not running.
    """
    docker_bin = shutil.which("docker")
    if docker_bin is None:
        err_msg = "docker not found in PATH"
        raise RuntimeError(err_msg)

    instance_id = package_instance.clp_instance_id
    problems: list[str] = []

    required_components = package_instance.package_config.component_list
    for component in required_components:
        prefix = f"clp-package-{instance_id}-{component}-"

        candidates = list_prefixed_containers(docker_bin, prefix)
        if not candidates:
            problems.append(f"No component container was found with the prefix '{prefix}'")
            continue

        not_running: list[str] = []
        for name in candidates:
            if not inspect_container_state(docker_bin, name, "running"):
                not_running.append(name)

        if not_running:
            details = ", ".join(not_running)
            problems.append(f"Component containers not running: {details}")

    if problems:
        return False, "; ".join(problems)

    return True, None


def is_running_mode_correct(package_instance: PackageInstance) -> tuple[bool, str | None]:
    """
    Checks if the mode described in the shared config file of `package_instance` is accurate with
    respect to its `mode_name`. Returns `True` if correct, `False` with message on mismatch.
    """
    shared_config_dict = load_yaml_to_dict(package_instance.shared_config_file_path)
    try:
        running_config = CLPConfig.model_validate(shared_config_dict)
    except ValidationError as err:
        err_msg = f"Shared config failed validation: {err}"
        raise ValueError(err_msg) from err

    intended_config = get_clp_config_from_mode(package_instance.package_config.mode_name)

    running_signature = compute_mode_signature(running_config)
    intended_signature = compute_mode_signature(intended_config)

    if running_signature != intended_signature:
        return (
            False,
            "Mode mismatch: running configuration does not match intended configuration.",
        )

    return True, None
