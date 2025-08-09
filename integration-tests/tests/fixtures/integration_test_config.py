from pathlib import Path

import pytest
from tests.utils.config import IntegrationTestConfig, PackageConfig
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def integration_test_config() -> IntegrationTestConfig:
    package_config = PackageConfig(clp_package_dir=Path(get_env_var("CLP_PACKAGE_DIR")).resolve())
    return IntegrationTestConfig(
        package_config=package_config,
        test_root_dir=Path(get_env_var("CLP_BUILD_DIR")).resolve() / "integration-tests",
    )
