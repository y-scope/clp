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
    return IntegrationTestDataset(
        dataset_root_dir=integration_test_path_config.test_data_dir / "json_multifile",
    )


@pytest.fixture(scope="session")
def text_multifile(
    integration_test_path_config: IntegrationTestPathConfig,
) -> IntegrationTestDataset:
    """Returns an object corresponding to the `text_multifile` test dataset."""
    return IntegrationTestDataset(
        dataset_root_dir=integration_test_path_config.test_data_dir / "text_multifile",
    )
