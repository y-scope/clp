"""Classes used in CLP integration tests."""

import inspect
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import pytest

from tests.utils.utils import (
    validate_dir_exists,
)

_STATIC_PATH_ATTR = "_is_static_path"


def static_path(func: Callable[..., Path]) -> Callable[..., Path]:
    """
    Decorator that marks a method as a static (non-runtime) path to be validated by
    `IntegrationTestPathConfig.validate_paths()`. Must be placed below `@property`.
    """
    setattr(func, _STATIC_PATH_ATTR, True)
    return func


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

        # Create `test_cache_dir`.
        self.test_cache_dir.mkdir(parents=True, exist_ok=True)

        # Create `test_log_dir`.
        self.test_log_dir.mkdir(parents=True, exist_ok=True)

        # Validate all static path properties.
        if type(self) is IntegrationTestPathConfig:
            self.validate_static_paths()

    def validate_static_paths(self) -> None:
        """
        Iterates over all properties in this class that are tagged with `@static_path` and asserts
        that each path exists. Check extends to subclasses.

        :raise pytest.fail: If any static path does not exist.
        """
        for name, member in inspect.getmembers(type(self)):
            if not isinstance(member, property):
                continue
            if not getattr(member.fget, _STATIC_PATH_ATTR, False):
                continue

            path: Path = getattr(self, name)
            if not path.exists():
                pytest.fail(f"Expected path does not exist: '{path}'")

    @property
    def test_cache_dir(self) -> Path:
        """:return: The absolute path to the integration test cache directory."""
        return self.clp_build_dir / "integration_tests"

    @property
    @static_path
    def test_data_path(self) -> Path:
        """:return: The absolute path to the sample dataset directory."""
        return self.integration_tests_project_root / "tests" / "data"

    @property
    def test_log_dir(self) -> Path:
        """:return: The absolute path to the integration test log directory."""
        return self.test_cache_dir / "test_logs"


@dataclass
class IntegrationTestDataset:
    """Metadata for a sample dataset."""

    #: The name of the dataset (for logging purposes).
    dataset_name: str

    #: Path to the dataset logs.
    path_to_dataset_logs: Path

    #: A dictionary of metadata describing the dataset.
    metadata_dict: dict[str, Any]
