"""Classes used in CLP integration tests."""

from dataclasses import dataclass, field
from pathlib import Path

import pytest
from pydantic import BaseModel

from tests.utils.utils import (
    validate_dir_exists,
    validate_file_exists,
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
    def test_data_dir(self) -> Path:
        """:return: The absolute path to the sample dataset directory."""
        return self.integration_tests_project_root / "tests" / "data"

    @property
    def test_log_dir(self) -> Path:
        """:return: The absolute path to the integration test log directory."""
        return self.test_cache_dir / "test_logs"

    def _static_paths(self) -> list[Path]:
        """:return: List of paths that must exist on disk at construction time."""
        return [self.test_data_dir]


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


@dataclass
class IntegrationTestDataset:
    """Path layout and metadata storage for a sample dataset."""

    #: Absolute path to the dataset root directory.
    dataset_root_dir: Path

    #: Pydantic model of metadata describing the dataset.
    metadata: IntegrationTestDatasetMetadata = field(init=False)

    #: The name of the dataset (for logging purposes).
    dataset_name: str = field(init=False)

    def __post_init__(self) -> None:
        """Validate data members and load metadata."""
        validate_dir_exists(self.dataset_root_dir)

        # Load metadata.
        validate_file_exists(self.metadata_file_path)
        raw_metadata = self.metadata_file_path.read_text()
        self.metadata = IntegrationTestDatasetMetadata.model_validate_json(raw_metadata)

        # Set dataset name from metadata.
        self.dataset_name = self.metadata.dataset_name

        # Validate metadata properties.
        validate_dir_exists(self.logs_path)

        if self.metadata.begin_ts > self.metadata.end_ts:
            err_msg = (
                f"Dataset metadata failure: `begin_ts` '{self.metadata.begin_ts}' is larger than"
                f" `end_ts` '{self.metadata.end_ts}'"
            )
            raise ValueError(err_msg)

        for file_path in self.metadata.file_names:
            file_path_abs = self.logs_path / file_path
            validate_file_exists(file_path_abs)

    @property
    def metadata_file_path(self) -> Path:
        """:return: The absolute path to the file containing metadata for the dataset."""
        return self.dataset_root_dir / "metadata.json"

    @property
    def logs_path(self) -> Path:
        """:return: The absolute path to the logs directory."""
        return self.dataset_root_dir / self.metadata.logs_subdir
