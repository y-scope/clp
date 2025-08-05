from pathlib import Path

import pytest
from tests.utils.config import TestConfig
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def test_config() -> TestConfig:
    return TestConfig(
        clp_package_dir=Path(get_env_var("CLP_PACKAGE_DIR")).resolve(),
        test_root_dir=Path(get_env_var("CLP_BUILD_DIR")).resolve() / "integration-tests",
    )
