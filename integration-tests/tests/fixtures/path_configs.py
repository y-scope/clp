"""Session scoped path configuration fixtures shared across integration tests."""

import pytest

from tests.utils.config import (
    ClpCorePathConfig,
    IntegrationTestPathConfig,
    PackagePathConfig,
)
from tests.utils.utils import resolve_env_path


@pytest.fixture(scope="session")
def clp_core_path_config() -> ClpCorePathConfig:
    """Provides paths for the CLP core binaries."""
    return ClpCorePathConfig(clp_core_bins_dir=resolve_env_path("CLP_CORE_BINS_DIR"))


@pytest.fixture(scope="session")
def integration_test_path_config() -> IntegrationTestPathConfig:
    """Provides paths for the integration-test directory and its contents."""
    return IntegrationTestPathConfig(
        test_root_dir=resolve_env_path("CLP_BUILD_DIR") / "integration-tests"
    )


@pytest.fixture(scope="session")
def package_path_config() -> PackagePathConfig:
    """Provides paths for the clp-package directory and its contents."""
    return PackagePathConfig(clp_package_dir=resolve_env_path("CLP_PACKAGE_DIR"))
