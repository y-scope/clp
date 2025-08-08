from pathlib import Path

import pytest
from tests.utils.config import IntegrationTestConfig
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def integration_test_config() -> IntegrationTestConfig:
    return IntegrationTestConfig(
        clp_package_dir=Path(get_env_var("CLP_PACKAGE_DIR")).resolve(),
        test_root_dir=Path(get_env_var("CLP_BUILD_DIR")).resolve() / "integration-tests",
    )
