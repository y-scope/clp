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
