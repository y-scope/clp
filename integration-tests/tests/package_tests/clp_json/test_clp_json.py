"""Tests for the clp-json package."""

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_json.utils.mode import CLP_JSON_MODE
from tests.package_tests.utils.compress import (
    compress_clp_package,
    verify_compress_action,
)
from tests.utils.classes import SampleDataset

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
    compress_action = compress_clp_package(clp_package, json_multifile)
    result = verify_compress_action(compress_action, clp_package, json_multifile)
    assert result, result.failure_message


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package successfully searches some dataset.

    :param clp_package:
    """
    # TODO: compress a dataset

    # TODO: check the correctness of the compression

    # TODO: search through that dataset and check the correctness of the search results.

    assert clp_package
