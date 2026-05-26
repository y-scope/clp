"""Utilities that start and stop the CLP package."""

import logging
from pathlib import Path

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import ClpAction, ClpVerificationResult, CmdArgs

logger = logging.getLogger(__name__)


class StartStopArgs(CmdArgs):
    """Command argument model for starting and stopping the CLP package."""

    script_path: Path
    config: Path

    def to_cmd(self) -> list[str]:
        """Convert the model attributes to a command list."""
        return [
            str(self.script_path),
            "--config",
            str(self.config),
        ]


def start_clp_package(
    clp_package: ClpPackage,
) -> ClpAction:
    """
    Starts the CLP package.

    :param clp_package:
    :return: The `ClpAction` instance that starts the package.
    """
    logger.info("Starting up the '%s' package.", clp_package.mode_name)

    args = StartStopArgs(
        script_path=clp_package.path_config.start_clp_path,
        config=clp_package.temp_config_file_path,
    )
    return ClpAction.from_args(args)


def stop_clp_package(
    clp_package: ClpPackage,
) -> ClpAction:
    """
    Stops the CLP package.

    :param clp_package:
    :return: The `ClpAction` instance that stops the package.
    """
    logger.info("Stopping the '%s' package.", clp_package.mode_name)

    args = StartStopArgs(
        script_path=clp_package.path_config.stop_clp_path,
        config=clp_package.temp_config_file_path,
    )
    return ClpAction.from_args(args)


def verify_start_clp_action(
    start_clp_action: ClpAction, clp_package: ClpPackage
) -> ClpVerificationResult:
    """
    Verifies the startup of the CLP package by checking that the start command exited with a
    successful return code and that the set of running services exactly matches the package's
    required components.

    :param start_clp_action:
    :param clp_package:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying the startup of the '%s' package.", clp_package.mode_name)

    returncode_result = start_clp_action.verify_returncode()
    if not returncode_result:
        return returncode_result

    running_services = clp_package.get_running_services()
    required_components = set(clp_package.component_list)
    if required_components == running_services:
        return start_clp_action.pass_verification()

    mode_name = clp_package.mode_name
    fail_msg = (
        f"'{mode_name}' package start up verification failure: components could not be validated."
    )
    missing_components = required_components - running_services
    if missing_components:
        fail_msg += f" Missing components: {missing_components}."
    unexpected_components = running_services - required_components
    if unexpected_components:
        fail_msg += f" Unexpected services: {unexpected_components}."

    return start_clp_action.fail_verification(fail_msg)


def verify_stop_clp_action(
    stop_clp_action: ClpAction, clp_package: ClpPackage
) -> ClpVerificationResult:
    """
    Verifies the spindown of the CLP package by checking that the stop command exited with a
    successful return code and that no package services remain running.

    :param stop_clp_action:
    :param clp_package:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying the spindown of the '%s' package.", clp_package.mode_name)
    returncode_result = stop_clp_action.verify_returncode()
    if not returncode_result:
        return returncode_result

    running_services = clp_package.get_running_services()
    if not running_services:
        return stop_clp_action.pass_verification()

    mode_name = clp_package.mode_name
    fail_msg = (
        f"'{mode_name}' package stop verification failure: there are components of the package that"
        f" are still running: '{running_services}'"
    )
    return stop_clp_action.fail_verification(fail_msg)
