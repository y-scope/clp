"""Session-scoped test log fixtures shared across integration tests."""

import pytest

from tests.utils.classes import (
    IntegrationTestDataset,
    IntegrationTestPathConfig,
)
from tests.utils.utils import load_json_to_dict


@pytest.fixture(scope="session")
def json_multifile(
    integration_test_path_config: IntegrationTestPathConfig,
) -> IntegrationTestDataset:
    """Returns an object corresponding to the `json_multifile` test dataset."""
    path_to_dataset = integration_test_path_config.test_data_path / "json_multifile"
    metadata_dict = load_json_to_dict(path_to_dataset / "metadata.json")
    return IntegrationTestDataset(
        dataset_name="json_multifile",
        path_to_dataset_logs=path_to_dataset / metadata_dict["file_structure"]["logs_subdir"],
        metadata_dict=metadata_dict,
    )


@pytest.fixture(scope="session")
def text_multifile(
    integration_test_path_config: IntegrationTestPathConfig,
) -> IntegrationTestDataset:
    """Returns an object corresponding to the `text_multifile` test dataset."""
    path_to_dataset = integration_test_path_config.test_data_path / "text_multifile"
    metadata_dict = load_json_to_dict(path_to_dataset / "metadata.json")
    return IntegrationTestDataset(
        dataset_name="text_multifile",
        path_to_dataset_logs=path_to_dataset / metadata_dict["file_structure"]["logs_subdir"],
        metadata_dict=metadata_dict,
    )
