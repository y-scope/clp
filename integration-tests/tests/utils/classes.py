"""Classes used in CLP integration tests."""

from dataclasses import dataclass, field
from pathlib import Path

import pytest
from pydantic import BaseModel

from tests.utils.utils import (
    validate_dir_exists,
)


@dataclass
class IntegrationTestPathConfig:
    """Path configuration for CLP integration tests."""

    #: Default CLP build directory.
    clp_build_dir: Path

    #: Default integration test project root directory.
    integration_tests_project_root: Path

    def __post_init__(self) -> None:
        """Create and validate directories."""
        # Validate that init directories exist.
        validate_dir_exists(self.clp_build_dir)
        validate_dir_exists(self.integration_tests_project_root)

        # Validate all static paths.
        for path in self._static_paths():
            if not path.exists():
                pytest.fail(f"Expected path does not exist: '{path}'")

        # Create `test_cache_dir`.
        self.test_cache_dir.mkdir(parents=True, exist_ok=True)

        # Create `test_log_dir`.
        self.test_log_dir.mkdir(parents=True, exist_ok=True)

    @property
    def test_cache_dir(self) -> Path:
        """:return: The absolute path to the integration test cache directory."""
        return self.clp_build_dir / "integration_tests"

    @property
    def test_data_path(self) -> Path:
        """:return: The absolute path to the sample dataset directory."""
        return self.integration_tests_project_root / "tests" / "data"

    @property
    def test_log_dir(self) -> Path:
        """:return: The absolute path to the integration test log directory."""
        return self.test_cache_dir / "test_logs"

    def _static_paths(self) -> list[Path]:
        """:return: List of paths that must exist on disk at construction time."""
        return [self.test_data_path]


class IntegrationTestDatasetMetadata(BaseModel):
    """
    Metadata for a sample dataset. All `<dataset_name>/metadata.json` files must conform to this
    schema.
    """

    dataset_name: str
    unstructured: bool
    timestamp_key: str | None
    begin_ts: int
    end_ts: int
    logs_subdir: str
    file_names: list[str]
    single_match_wildcard_query: str
    columns_file_name: str


@dataclass
class IntegrationTestDataset:
    """Path layout and metadata storage for a sample dataset."""

    #: The name of the dataset (for logging purposes).
    dataset_name: str

    #: Absolute path to the dataset root.
    path_to_dataset_root: Path

    #: Pydantic model of metadata describing the dataset.
    metadata: IntegrationTestDatasetMetadata = field(init=False)

    def __post_init__(self) -> None:
        """Validate data members and load metadata."""
        validate_dir_exists(self.path_to_dataset_root)

        # Load metadata.
        validate_dir_exists(self.metadata_file_path)
        raw_metadata = self.metadata_file_path.read_text()
        self.metadata = IntegrationTestDatasetMetadata.model_validate_json(raw_metadata)

        # Validate metadata properties.
        validate_dir_exists(self.logs_path)

        if self.columns_file_path is not None:
            validate_dir_exists(self.columns_file_path)

        if self.metadata.dataset_name != self.dataset_name:
            pytest.fail(
                f"Dataset '{self.dataset_name}' carries the incorrect name in its metadata:"
                f" '{self.metadata.dataset_name}'"
            )

        if self.metadata.begin_ts > self.metadata.end_ts:
            pytest.fail(
                f"Dataset metadata failure: `begin_ts` '{self.metadata.begin_ts}' is larger than"
                f" `end_ts` '{self.metadata.end_ts}'"
            )

        for file_path in self.metadata.file_names:
            file_path_abs = self.logs_path / file_path
            if not file_path_abs.is_file():
                pytest.fail(
                    "Dataset metadata failure: log file specified in `file_names` does not exist:"
                    f" '{file_path_abs}'"
                )

    @property
    def metadata_file_path(self) -> Path:
        """:return: The absolute path to the file containing metadata for the dataset."""
        return self.path_to_dataset_root / "metadata.json"

    @property
    def logs_path(self) -> Path:
        """:return: The absolute path to the subdirectory containing logs."""
        return self.path_to_dataset_root / self.metadata.logs_subdir

    @property
    def columns_file_path(self) -> Path | None:
        """
        :return: The absolute path to the file containing a description of the dataset columns, or
        `None` if there is no such file.
        """
        if self.metadata.columns_file_name is not None:
            return self.path_to_dataset_root / self.metadata.columns_file_name
        return None
