"""Session-scoped test log fixtures shared across integration tests."""

import pytest

from tests.utils.classes import (
    IntegrationTestDataset,
    IntegrationTestPathConfig,
)


@pytest.fixture(scope="session")
def json_multifile(
    integration_test_path_config: IntegrationTestPathConfig,
) -> IntegrationTestDataset:
    """Returns an object corresponding to the `json_multifile` test dataset."""
    dataset_name = "json_multifile"
    return IntegrationTestDataset(
        dataset_name=dataset_name,
        path_to_dataset_root=integration_test_path_config.test_data_path / dataset_name,
    )


@pytest.fixture(scope="session")
def text_multifile(
    integration_test_path_config: IntegrationTestPathConfig,
) -> IntegrationTestDataset:
    """Returns an object corresponding to the `text_multifile` test dataset."""
    dataset_name = "text_multifile"
    return IntegrationTestDataset(
        dataset_name=dataset_name,
        path_to_dataset_root=integration_test_path_config.test_data_path / dataset_name,
    )
