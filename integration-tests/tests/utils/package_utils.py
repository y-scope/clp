"""Provides utility functions related to the CLP package used across `integration-tests`."""

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import PackageTestConfig

DEFAULT_CMD_TIMEOUT_SECONDS = 120.0


def start_clp_package(package_test_config: PackageTestConfig) -> None:
    """
    Starts an instance of the CLP package.

    :param package_test_config:
    :raise: Propagates `run_and_assert`'s errors.
    """
    path_config = package_test_config.path_config
    start_script_path = path_config.start_script_path
    temp_config_file_path = package_test_config.temp_config_file_path

    # fmt: off
    start_cmd = [
        str(start_script_path),
        "--config", str(temp_config_file_path),
    ]
    # fmt: on
    run_and_assert(start_cmd, timeout=DEFAULT_CMD_TIMEOUT_SECONDS)


def stop_clp_package(package_test_config: PackageTestConfig) -> None:
    """
    Stops the running instance of the CLP package.

    :param package_test_config:
    :raise: Propagates `run_and_assert`'s errors.
    """
    path_config = package_test_config.path_config
    stop_script_path = path_config.stop_script_path
    temp_config_file_path = package_test_config.temp_config_file_path

    # fmt: off
    stop_cmd = [
        str(stop_script_path),
        "--config", str(temp_config_file_path),
    ]
    # fmt: on
    run_and_assert(stop_cmd, timeout=DEFAULT_CMD_TIMEOUT_SECONDS)
