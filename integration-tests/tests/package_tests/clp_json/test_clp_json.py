"""Tests for the clp-json package."""

import logging

import pytest

from tests.package_tests.clp_json.utils.mode import CLP_JSON_MODE
from tests.utils.asserting_utils import (
    validate_package_running,
    verify_package_compression,
)
from tests.utils.classes import SampleDataset
from tests.utils.config import PackageCompressionJob, PackageInstance
from tests.utils.package_utils import run_package_compression_script

logger = logging.getLogger(__name__)


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_json,
    pytest.mark.parametrize("fixt_package_test_config", [CLP_JSON_MODE], indirect=True),
]


@pytest.mark.startup
def test_clp_json_startup(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the `clp-json` package starts up successfully.

    :param fixt_package_instance:
    """
    logger.info("Starting test: 'test_clp_json_startup'")

    validate_package_running(fixt_package_instance)

    logger.info("Test complete: 'test_clp_json_startup'")


@pytest.mark.compression
def test_clp_json_compression_json_multifile(
    fixt_package_instance: PackageInstance,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `json-multifile` dataset.

    :param fixt_package_instance:
    :param json_multifile:
    """
    logger.info("Starting test: 'test_clp_json_compression_json_multifile'")

    validate_package_running(fixt_package_instance)

    # Clear archives before compressing.
    package_test_config = fixt_package_instance.package_test_config
    package_path_config = package_test_config.path_config
    package_path_config.clear_package_archives()

    # Compress a dataset.
    timestamp_key = json_multifile.metadata.timestamp_key
    assert timestamp_key is not None, "`json-multifile` dataset must define a `timestamp_key`."
    compression_job = PackageCompressionJob(
        path_to_original_dataset=json_multifile.logs_path,
        options=[
            "--timestamp-key",
            timestamp_key,
            "--dataset",
            json_multifile.metadata.dataset_name,
        ],
        positional_args=None,
    )
    run_package_compression_script(compression_job, package_test_config)

    # Check the correctness of compression.
    result = verify_package_compression(json_multifile.logs_path, package_test_config)
    if not result:
        pytest.fail(result.failure_message)

    # Clear archives.
    package_path_config.clear_package_archives()

    logger.info("Test complete: 'test_clp_json_compression_json_multifile'")


@pytest.mark.search
def test_clp_json_search(fixt_package_instance: PackageInstance) -> None:
    """
    Validate that the `clp-json` package successfully searches some dataset.

    :param fixt_package_instance:
    """
    logger.info("Starting test: 'test_clp_json_search'")

    validate_package_running(fixt_package_instance)

    # TODO: compress a dataset

    # TODO: check the correctness of the compression

    # TODO: search through that dataset and check the correctness of the search results.

    assert True

    logger.info("Test complete: 'test_clp_json_search'")

    # TODO: clean up clp-package/var/data, clp-package/var/log, and clp-package/var/tmp
