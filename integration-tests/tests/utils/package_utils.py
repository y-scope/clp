"""Provides utility functions related to the CLP package used across `integration-tests`."""

import logging
from subprocess import SubprocessError

import pytest

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import PackageConfig
from tests.utils.logging_utils import construct_log_err_msg

logger = logging.getLogger(__name__)


DEFAULT_CMD_TIMEOUT_SECONDS = 120.0


def start_clp_package(request: pytest.FixtureRequest, package_config: PackageConfig) -> None:
    """
    Starts an instance of the CLP package.

    :param package_config:
    :raise: Propagates `run_and_assert`'s errors.
    """
    path_config = package_config.path_config
    start_script_path = path_config.start_script_path
    temp_config_file_path = package_config.temp_config_file_path

    # fmt: off
    start_cmd = [
        str(start_script_path),
        "--config", str(temp_config_file_path),
    ]
    # fmt: on

    try:
        run_and_assert(request, start_cmd, timeout=DEFAULT_CMD_TIMEOUT_SECONDS)
    except SubprocessError:
        mode_name = package_config.mode_name
        err_msg = f"The {mode_name} package failed to start."
        logger.error(construct_log_err_msg(err_msg))
        pytest.fail(err_msg)


def stop_clp_package(request: pytest.FixtureRequest, package_config: PackageConfig) -> None:
    """
    Stops the running instance of the CLP package.

    :param package_config:
    :raise: Propagates `run_and_assert`'s errors.
    """
    path_config = package_config.path_config
    stop_script_path = path_config.stop_script_path
    temp_config_file_path = package_config.temp_config_file_path

    # fmt: off
    stop_cmd = [
        str(stop_script_path),
        "--config", str(temp_config_file_path),
    ]
    # fmt: on

    try:
        run_and_assert(request, stop_cmd, timeout=DEFAULT_CMD_TIMEOUT_SECONDS)
    except SubprocessError:
        mode_name = package_config.mode_name
        err_msg = f"The {mode_name} package failed to stop."
        logger.error(construct_log_err_msg(err_msg))
        pytest.fail(err_msg)
