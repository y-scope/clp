from pathlib import Path

import pytest
from tests.utils.config import TestConfig
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def test_config() -> TestConfig:
    clp_build_dir = Path(get_env_var("CLP_BUILD_DIR")).resolve()
    clp_package_dir = Path(get_env_var("CLP_PACKAGE_DIR")).resolve()

    # Check for required directories
    required_dirs = ["bin", "etc", "lib", "sbin"]
    missing_dirs = [d for d in required_dirs if not (clp_package_dir / d).is_dir()]
    if len(missing_dirs) > 0:
        raise ValueError(
            f"CLP package at {clp_package_dir} is incomplete. Missing: {', '.join(missing_dirs)}"
        )

    config = TestConfig(
        clp_package_dir=clp_package_dir,
        logs_source_dir=clp_build_dir / "integration-tests" / "downloads",
        test_root_dir=clp_build_dir / "integration-tests",
    )
    config.test_root_dir.mkdir(parents=True, exist_ok=True)
    config.logs_source_dir.mkdir(parents=True, exist_ok=True)
    return config
