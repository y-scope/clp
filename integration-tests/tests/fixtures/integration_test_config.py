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
    core_config = CoreConfig(clp_core_bins_dir=Path(get_env_var("CLP_CORE_BINS_DIR")).resolve())
    package_config = PackageConfig(clp_package_dir=Path(get_env_var("CLP_PACKAGE_DIR")).resolve())
    return IntegrationTestConfig(
        core_config=core_config,
        package_config=package_config,
        test_root_dir=Path(get_env_var("CLP_BUILD_DIR")).resolve() / "integration-tests",
    )
