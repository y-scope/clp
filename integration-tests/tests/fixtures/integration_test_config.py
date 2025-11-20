"""Define the integration test configuration fixture."""

from pathlib import Path

import pytest

from tests.utils.config import (
    CoreConfig,
    IntegrationTestConfig,
    PackagePathConfig,
)
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def core_config() -> CoreConfig:
    """Fixture that provides a CoreConfig shared across tests."""
    return CoreConfig(
        clp_core_bins_dir=Path(get_env_var("CLP_CORE_BINS_DIR")).expanduser().resolve()
    )


@pytest.fixture(scope="session")
def package_path_config(integration_test_config: IntegrationTestConfig) -> PackagePathConfig:
    """Fixture that provides a PackagePathConfig shared across tests."""
    return PackagePathConfig(
        clp_package_dir=Path(get_env_var("CLP_PACKAGE_DIR")).expanduser().resolve(),
        test_root_dir=integration_test_config.test_root_dir,
    )


@pytest.fixture(scope="session")
def integration_test_config() -> IntegrationTestConfig:
    """Fixture that provides an IntegrationTestConfig shared across tests."""
    return IntegrationTestConfig(
        test_root_dir=Path(get_env_var("CLP_BUILD_DIR")).expanduser().resolve()
        / "integration-tests",
    )
