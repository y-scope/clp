"""Provides utility functions related to the CLP package used across `integration-tests`."""

import subprocess

from tests.utils.config import (
    PackageConfig,
    PackageInstance,
)


def start_clp_package(package_config: PackageConfig) -> None:
    """
    Starts an instance of the CLP package.

    :param package_config:
    :raise RuntimeError: If the package fails to start.
    """
    path_config = package_config.path_config
    start_script_path = path_config.start_script_path
    temp_config_file_path = package_config.temp_config_file_path
    try:
        # fmt: off
        start_cmd = [
            str(start_script_path),
            "--config", str(temp_config_file_path),
        ]
        # fmt: on
        subprocess.run(start_cmd, check=True)
    except Exception as err:
        err_msg = f"Failed to start an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from err


def stop_clp_package(instance: PackageInstance) -> None:
    """
    Stops an instance of the CLP package.

    :param instance:
    :raise RuntimeError: If the package fails to stop.
    """
    package_config = instance.package_config
    path_config = package_config.path_config
    stop_script_path = path_config.stop_script_path
    try:
        # fmt: off
        stop_cmd = [
            str(stop_script_path)
        ]
        # fmt: on
        subprocess.run(stop_cmd, check=True)
    except Exception as err:
        err_msg = f"Failed to stop an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from err
