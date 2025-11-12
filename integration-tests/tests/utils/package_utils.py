"""Provides utility functions related to the clp-package used across `integration-tests`."""

import subprocess

from clp_py_utils.clp_config import CLPConfig
from pydantic import ValidationError

from tests.utils.clp_mode_utils import (
    compute_mode_signature,
    get_clp_config_from_mode,
)
from tests.utils.config import (
    PackageConfig,
    PackageInstance,
)
from tests.utils.docker_utils import (
    list_running_containers_with_prefix,
)
from tests.utils.utils import load_yaml_to_dict


def start_clp_package(package_config: PackageConfig) -> None:
    """
    Starts an instance of the CLP package.

    :param package_config:
    :raise RuntimeError: if the package fails to start.
    """
    start_script_path = package_config.start_script_path

    # Use the deterministic temp config file path.
    temp_config_file_path = (
        package_config.temp_config_dir / f"clp-config-{package_config.mode_name}.yml"
    )
    try:
        # fmt: off
        start_cmd = [
            str(start_script_path),
            "--config", str(temp_config_file_path),
        ]
        # fmt: on
        subprocess.run(start_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to start an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from e


def stop_clp_package(instance: PackageInstance) -> None:
    """
    Stops an instance of the CLP package.

    :param instance:
    :raise RuntimeError: if the package fails to stop.
    """
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


def validate_package_running(package_instance: PackageInstance) -> tuple[bool, str | None]:
    """
    Determines whether the given package instance is running. The check confirms that
    each required component has at least one running container whose name matches the
    expected prefix. The check stops at the first missing component.

    :param package_instance:
    :return: (True, None) if all required components are running, otherwise (False, message).
    """
    instance_id = package_instance.clp_instance_id
    required_components = package_instance.component_list

    for component in required_components:
        prefix = f"clp-package-{instance_id}-{component}-"
        running_matches = list_running_containers_with_prefix(prefix)
        if len(running_matches) == 0:
            return (
                False,
                f"No running container found for component '{component}' "
                f"(expected name prefix '{prefix}').",
            )

    return True, None


def validate_running_mode_correct(package_instance: PackageInstance) -> tuple[bool, str | None]:
    """
    Determines whether the mode described in the shared config of the instance matches
    the intended mode defined by the instance configuration.

    :param package_instance:
    :return: (True, None) if the running mode matches the intended mode, otherwise (False, message).
    :raise ValueError: if the shared config fails validation.
    """
    shared_config_dict = load_yaml_to_dict(package_instance.shared_config_file_path)
    try:
        running_config = CLPConfig.model_validate(shared_config_dict)
    except ValidationError as err:
        err_msg = "Shared config failed validation"
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
