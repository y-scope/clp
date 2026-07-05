"""Tests for the clp-json package."""

import logging

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_json.utils.mode import CLP_JSON_MODE
from tests.package_tests.clp_json.verification.compress import (
    verify_compress_structured_clp_json,
    verify_compress_unstructured_clp_json,
)
from tests.package_tests.utils.compress import (
    CompressArgs,
)
from tests.utils.classes import (
    ClpAction,
    SampleDataset,
    SampleDatasetMetadata,
)

logger = logging.getLogger(__name__)


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_json,
    pytest.mark.parametrize(
        "clp_package", [CLP_JSON_MODE], indirect=True, ids=[CLP_JSON_MODE.mode_name]
    ),
]


@pytest.mark.startstop
def test_clp_json_startstop(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package starts up successfully.

    :param clp_package:
    """
    assert clp_package


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_compression_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `json-multifile` dataset.

    :param clp_package:
    :param json_multifile:
    """
    metadata: SampleDatasetMetadata = json_multifile.metadata
    args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[json_multifile.logs_path],
    )

    logger.info("Compressing the 'json_multifile' dataset with the 'clp-json' package.")
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the compression of the 'json_multifile' dataset.")
    result = verify_compress_structured_clp_json(action, clp_package, json_multifile)
    assert result, result.failure_message


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_compression_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `text-multifile` dataset as
    unstructured text.

    :param clp_package:
    :param text_multifile:
    """
    metadata: SampleDatasetMetadata = text_multifile.metadata
    args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[text_multifile.logs_path],
    )

    logger.info("Compressing the 'text_multifile' dataset with the 'clp-json' package.")
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the compression of the 'text_multifile' dataset.")
    result = verify_compress_unstructured_clp_json(action, clp_package, text_multifile)
    assert result, result.failure_message
