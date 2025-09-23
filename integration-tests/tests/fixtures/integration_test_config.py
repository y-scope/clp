"""Define the integration test configuration fixture."""

from pathlib import Path

import pytest

from tests.utils.config import (
    CoreConfig,
    IntegrationTestConfig,
    PackageConfig,
)
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def integration_test_config() -> IntegrationTestConfig:
    """Fixture that provides an IntegrationTestConfig shared across tests."""
    core_config = CoreConfig(
        clp_core_bins_dir=Path(get_env_var("CLP_CORE_BINS_DIR")).expanduser().resolve()
    )
    package_config = PackageConfig(
        clp_package_dir=Path(get_env_var("CLP_PACKAGE_DIR")).expanduser().resolve()
    )
    test_root_dir = Path(get_env_var("CLP_BUILD_DIR")).expanduser().resolve() / "integration-tests"
    return IntegrationTestConfig(
        core_config=core_config,
        package_config=package_config,
        test_root_dir=test_root_dir,
    )
