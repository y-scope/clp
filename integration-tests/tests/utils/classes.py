"""Classes used in CLP integration tests."""

import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import pytest

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


@dataclass
class IntegrationTestDataset:
    """Metadata for a sample dataset."""

    #: The name of the dataset (for logging purposes).
    dataset_name: str

    #: Path to the dataset logs.
    path_to_dataset_logs: Path

    #: A dictionary of metadata describing the dataset.
    metadata_dict: dict[str, Any]


@dataclass
class IntegrationTestExternalAction:
    """Metadata for an external action executed during an integration test."""

    #: Command to pass to `subprocess.run()`.
    cmd: list[str]

    #: The completed process returned from `subprocess.run()`.
    completed_proc: subprocess.CompletedProcess[str] = field(init=False)

    #: Path to the file where this action's subprocess output was logged.
    log_file_path: Path = field(init=False)
