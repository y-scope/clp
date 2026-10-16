"""Session-scoped fixtures for sample datasets stored in the integration-tests data folder."""

import pytest

from tests.utils.classes import (
    IntegrationTestPathConfig,
    SampleDataset,
)


@pytest.fixture(scope="session")
def json_multifile(
    integration_test_path_config: IntegrationTestPathConfig,
) -> SampleDataset:
    """Returns an object corresponding to the `json_multifile` sample dataset."""
    return SampleDataset(
        dataset_root_dir=integration_test_path_config.test_data_dir / "json_multifile",
    )


@pytest.fixture(scope="session")
def json_clpp_synthetic(
    integration_test_path_config: IntegrationTestPathConfig,
) -> SampleDataset:
    """
    Returns an object corresponding to the `json_clpp_synthetic` sample dataset.

    This dataset holds synthetic structured log events exercising clp+ cases that do not show up
    naturally in the captured datasets (e.g. repeated parent rules with different shapes).
    """
    return SampleDataset(
        dataset_root_dir=integration_test_path_config.test_data_dir / "json_clpp_synthetic",
    )


@pytest.fixture(scope="session")
def json_multimsg(
    integration_test_path_config: IntegrationTestPathConfig,
) -> SampleDataset:
    """
    Returns an object corresponding to the `json_multimsg` sample dataset.

    This dataset has log events with multiple sibling unstructured text fields (`message` and
    `details`) so it exercises multi-`StructurizedText`-per-schema handling.
    """
    return SampleDataset(
        dataset_root_dir=integration_test_path_config.test_data_dir / "json_multimsg",
    )


@pytest.fixture(scope="session")
def text_multifile(
    integration_test_path_config: IntegrationTestPathConfig,
) -> SampleDataset:
    """Returns an object corresponding to the `text_multifile` sample dataset."""
    return SampleDataset(
        dataset_root_dir=integration_test_path_config.test_data_dir / "text_multifile",
    )


@pytest.fixture(scope="session")
def text_singlefile(
    integration_test_path_config: IntegrationTestPathConfig,
) -> SampleDataset:
    """Returns an object corresponding to the `text_singlefile` sample dataset."""
    return SampleDataset(
        dataset_root_dir=integration_test_path_config.test_data_dir / "text_singlefile",
    )
