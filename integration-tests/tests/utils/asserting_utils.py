"""Utilities that raise pytest assertions on failure."""

import logging

import pytest

from tests.utils.config import PackageInstance
from tests.utils.docker_utils import list_running_services_in_compose_project

logger = logging.getLogger(__name__)


def validate_package_running(package_instance: PackageInstance) -> None:
    """
    Validate that the given package instance is running by checking that the set of services running
    in the Compose project exactly matches the list of required components.

    :param package_instance:
    :raise pytest.fail: if the sets of running services and required components do not match.
    """
    logger.info(
        "Validating the '%s' package.",
        package_instance.package_test_config.mode_config.mode_name,
    )

    # Get list of services currently running in the Compose project.
    instance_id = package_instance.clp_instance_id
    project_name = f"clp-package-{instance_id}"
    running_services = set(list_running_services_in_compose_project(project_name))

    # Compare with list of required components.
    required_components = set(package_instance.package_test_config.mode_config.component_list)
    if required_components == running_services:
        return

    fail_msg = "Component mismatch."

    missing_components = required_components - running_services
    if missing_components:
        fail_msg += f"\nMissing components: {missing_components}."

    unexpected_components = running_services - required_components
    if unexpected_components:
        fail_msg += f"\nUnexpected services: {unexpected_components}."

    pytest.fail(fail_msg)
