"""Utilities that raise pytest assertions on failure."""

import logging
from pathlib import Path

import pytest
from clp_package_utils.general import EXTRACT_FILE_CMD

from tests.utils.classes import ClpAction, VerificationResult
from tests.utils.config import PackageInstance, PackageTestConfig
from tests.utils.docker_utils import list_running_services_in_compose_project
from tests.utils.fs_validation import is_dir_tree_content_equal
from tests.utils.utils import clear_directory

logger = logging.getLogger(__name__)


def validate_package_running(package_instance: PackageInstance) -> None:
    """
    Validate that the given package instance is running by checking that the set of services running
    in the Compose project exactly matches the list of required components.

    :param package_instance:
    :raise pytest.fail: if the sets of running services and required components do not
        match.
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


def verify_package_compression(
    path_to_original_dataset: Path,
    package_test_config: PackageTestConfig,
) -> VerificationResult:
    """
    Verify that compression has been executed correctly by decompressing the contents of
    `clp-package/var/data/archives` and comparing the decompressed logs to the originals stored at
    `path_to_original_dataset`.

    :param path_to_original_dataset:
    :param package_test_config:
    :return: A `VerificationResult` describing the outcome.
    """
    mode = package_test_config.mode_config.mode_name
    log_msg = f"Verifying {mode} package compression."
    logger.info(log_msg)

    if mode == "clp-json":
        # TODO: Waiting for PR 1299 to be merged.
        return VerificationResult.ok()
    if mode == "clp-text":
        # Decompress the contents of `clp-package/var/data/archives`.
        path_config = package_test_config.path_config
        decompress_script_path = path_config.decompress_script_path
        decompression_dir = path_config.package_decompression_dir
        temp_config_file_path = package_test_config.temp_config_file_path

        clear_directory(decompression_dir)

        decompress_cmd = [
            str(decompress_script_path),
            "--config",
            str(temp_config_file_path),
            EXTRACT_FILE_CMD,
            "--extraction-dir",
            str(decompression_dir),
        ]

        # Run decompression command and check that it succeeds.
        decompress_action = ClpAction.from_cmd(decompress_cmd)
        decompress_result = decompress_action.verify_returncode()
        if not decompress_result:
            clear_directory(decompression_dir)
            return decompress_result

        # Verify content equality.
        output_path = decompression_dir / path_to_original_dataset.relative_to(
            path_to_original_dataset.anchor
        )

        try:
            if not is_dir_tree_content_equal(path_to_original_dataset, output_path):
                return VerificationResult.fail(
                    f"Mismatch between clp input {path_to_original_dataset} and output"
                    f" {output_path}."
                )
        finally:
            clear_directory(decompression_dir)

    return VerificationResult.ok()
