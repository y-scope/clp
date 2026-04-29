"""Utilities that start and stop the CLP package."""

import logging
from pathlib import Path

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import CmdArgs, ExternalAction, VerificationResult
from tests.utils.docker_utils import list_running_services_in_compose_project
from tests.utils.logging_utils import format_action_failure_msg

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
) -> ExternalAction:
    """
    Starts the CLP package.

    :param clp_package:
    :return: The `ExternalAction` instance that starts the package.
    """
    logger.info("Starting up the '%s' package.", clp_package.mode_name)
    args: StartStopArgs = _construct_start_clp_args(clp_package)
    return ExternalAction(cmd=args.to_cmd(), args=args)


def _construct_start_clp_args(clp_package: ClpPackage) -> StartStopArgs:
    """Construct the `StartStopArgs` object that will start the package."""
    path_config = clp_package.path_config
    return StartStopArgs(
        script_path=path_config.start_clp_path, config=clp_package.temp_config_file_path
    )


def stop_clp_package(
    clp_package: ClpPackage,
) -> ExternalAction:
    """
    Stops the CLP package.

    :param clp_package:
    :return: The `ExternalAction` instance that stops the package.
    """
    logger.info("Stopping the '%s' package.", clp_package.mode_name)
    args: StartStopArgs = _construct_stop_clp_args(clp_package)
    return ExternalAction(cmd=args.to_cmd(), args=args)


def _construct_stop_clp_args(clp_package: ClpPackage) -> StartStopArgs:
    """Construct the `StartStopArgs` object that will stop the package."""
    path_config = clp_package.path_config
    return StartStopArgs(
        script_path=path_config.stop_clp_path, config=clp_package.temp_config_file_path
    )


def verify_start_clp_action(
    start_clp_action: ExternalAction, clp_package: ClpPackage
) -> VerificationResult:
    """
    Verify the startup of the CLP package.

    :param start_clp_action:
    :param clp_package:
    :return: A `VerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying the startup of the '%s' package.", clp_package.mode_name)
    if start_clp_action.completed_proc.returncode != 0:
        return VerificationResult.fail(
            format_action_failure_msg(
                "The start-clp.sh subprocess returned a non-zero exit code.",
                start_clp_action,
            )
        )

    package_running_result = _validate_clp_package_running(clp_package)
    if package_running_result:
        return VerificationResult.ok()

    return VerificationResult.fail(
        format_action_failure_msg(package_running_result.failure_message, start_clp_action)
    )


def verify_stop_clp_action(
    stop_clp_action: ExternalAction, clp_package: ClpPackage
) -> VerificationResult:
    """
    Verify the spindown of the CLP package.

    :param stop_clp_action:
    :param clp_package:
    :return: A `VerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying the spindown of the '%s' package.", clp_package.mode_name)
    if stop_clp_action.completed_proc.returncode != 0:
        return VerificationResult.fail(
            format_action_failure_msg(
                "The stop-clp.sh subprocess returned a non-zero exit code.", stop_clp_action
            )
        )

    package_not_running_result = _validate_clp_package_not_running(clp_package)
    if package_not_running_result:
        return VerificationResult.ok()

    return VerificationResult.fail(
        format_action_failure_msg(package_not_running_result.failure_message, stop_clp_action)
    )


def _validate_clp_package_running(clp_package: ClpPackage) -> VerificationResult:
    """
    Verify that the CLP package is running.

    :param clp_package:
    :return: A `VerificationResult` indicating the success or failure of the verification.
    """
    # Get list of services currently running in the Compose project.
    instance_id = clp_package.get_clp_instance_id()
    project_name = f"clp-package-{instance_id}"
    running_services = set(list_running_services_in_compose_project(project_name))

    # Compare with list of required components.
    required_components = set(clp_package.component_list)
    if required_components == running_services:
        return VerificationResult.ok()

    # Construct failure message.
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

    return VerificationResult.fail(fail_msg)


def _validate_clp_package_not_running(clp_package: ClpPackage) -> VerificationResult:
    """
    Verify that the CLP package is not running.

    :param clp_package:
    :return: A `VerificationResult` indicating the success or failure of the verification.
    """
    # Get list of services currently running in the Compose project.
    instance_id = clp_package.get_clp_instance_id()
    project_name = f"clp-package-{instance_id}"
    running_services = set(list_running_services_in_compose_project(project_name))

    # Make sure the set is empty.
    if not running_services:
        return VerificationResult.ok()

    # Construct failure message.
    mode_name = clp_package.mode_name
    fail_msg = (
        f"'{mode_name}' package stop verification failure: there are components of the package that"
        f" are still running: '{running_services}'"
    )
    return VerificationResult.fail(fail_msg)
