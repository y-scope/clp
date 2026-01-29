"""Utilities that raise pytest assertions on failure."""

import logging
import shlex
import subprocess
from typing import Any

import pytest
from clp_py_utils.clp_config import ClpConfig
from pydantic import ValidationError

from tests.utils.clp_mode_utils import compare_mode_signatures
from tests.utils.config import PackageCompressionJob, PackageInstance, PackageTestConfig
from tests.utils.docker_utils import list_running_services_in_compose_project
from tests.utils.utils import is_dir_tree_content_equal, load_yaml_to_dict, unlink

logger = logging.getLogger(__name__)


def run_and_assert(cmd: list[str], **kwargs: Any) -> subprocess.CompletedProcess[Any]:
    """
    Runs a command with subprocess and asserts that it succeeds with pytest.

    :param cmd: Command and arguments to execute.
    :param kwargs: Additional keyword arguments passed through to the subprocess.
    :return: The completed process object, for inspection or further handling.
    :raise: pytest.fail if the command exits with a non-zero return code.
    """
    logger.info("Running command: %s", shlex.join(cmd))

    try:
        proc = subprocess.run(cmd, check=True, **kwargs)
    except subprocess.CalledProcessError as e:
        pytest.fail(f"Command failed: {' '.join(cmd)}: {e}")
    except subprocess.TimeoutExpired as e:
        pytest.fail(f"Command timed out: {' '.join(cmd)}: {e}")
    return proc


def validate_package_instance(package_instance: PackageInstance) -> None:
    """
    Validate that the given package instance is running by performing two checks: validate that the
    instance has exactly the set of running components that it should have, and validate that the
    instance is running in the correct mode.

    :param package_instance:
    """
    # Ensure that all package components are running.
    _validate_package_running(package_instance)

    # Ensure that the package is running in the correct mode.
    _validate_running_mode_correct(package_instance)


def _validate_package_running(package_instance: PackageInstance) -> None:
    """
    Validate that the given package instance is running by checking that the set of services running
    in the Compose project exactly matches the list of required components.

    :param package_instance:
    :raise pytest.fail: if the sets of running services and required components do not match.
    """
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


def _validate_running_mode_correct(package_instance: PackageInstance) -> None:
    """
    Validate that the mode described in the shared config of the instance matches the intended mode
    defined by the instance configuration. Calls pytest.fail if the shared config fails validation
    or if the running mode does not match the intended mode.

    :param package_instance:
    :raise: Propagates `load_yaml_to_dict`'s errors.
    :raise pytest.fail: if the ClpConfig object cannot be validated.
    :raise pytest.fail: if the running ClpConfig does not match the intended ClpConfig.
    """
    shared_config_dict = load_yaml_to_dict(package_instance.shared_config_file_path)
    try:
        running_config = ClpConfig.model_validate(shared_config_dict)
    except ValidationError as err:
        pytest.fail(f"Shared config failed validation: {err}")

    intended_config = package_instance.package_test_config.mode_config.clp_config

    if not compare_mode_signatures(intended_config, running_config):
        pytest.fail("Mode mismatch: running configuration does not match intended configuration.")


def verify_compression(
    compression_job: PackageCompressionJob,
    package_test_config: PackageTestConfig,
) -> None:
    """
    Verify that the compression_job has been executed correctly by decompressing the contents of
    `clp-package/var/data/archives` and comparing the decompressed logs to the originals.

    :param compression_job:
    :param package_test_config:
    """
    mode = package_test_config.mode_config.mode_name

    if mode in ("clp-json", "clp-presto"):
        # TODO: Waiting for PR 1299 to be merged.
        assert True
    elif mode == "clp-text":
        # Decompress the contents of `clp-package/var/data/archives`.
        path_config = package_test_config.path_config
        decompress_script_path = path_config.decompress_script_path
        decompression_dir = path_config.package_decompression_dir
        temp_config_file_path = package_test_config.temp_config_file_path
        decompress_cmd = [
            str(decompress_script_path),
            "--config",
            str(temp_config_file_path),
            "x",
            "--extraction-dir",
            str(decompression_dir),
        ]

        # Run decompression command and assert that it succeeds.
        run_and_assert(decompress_cmd)

        # Verify content equality.
        path_to_dataset = compression_job.path_to_dataset
        output_path = decompression_dir / path_to_dataset.relative_to(path_to_dataset.anchor)

        if not is_dir_tree_content_equal(path_to_dataset, output_path):
            err_msg = f"Mismatch between clp input {path_to_dataset} and output {output_path}."
            pytest.fail(err_msg)

        unlink(decompression_dir)
