from pathlib import Path

import pytest
from tests.utils.config import PackageConfig
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def package_config() -> PackageConfig:
    clp_build_dir = Path(get_env_var("CLP_BUILD_DIR")).resolve()
    clp_package_dir = Path(get_env_var("CLP_PACKAGE_DIR")).resolve()

    # Check for required directories
    required_dirs = ["bin", "etc", "lib", "sbin"]
    missing_dirs = [d for d in required_dirs if not (clp_package_dir / d).is_dir()]
    if len(missing_dirs) > 0:
        raise ValueError(
            f"CLP package at {clp_package_dir} is incomplete. Missing: {', '.join(missing_dirs)}"
        )

    config = PackageConfig(
        clp_bin_dir=clp_package_dir / "bin",
        clp_package_dir=clp_package_dir,
        clp_sbin_dir=clp_package_dir / "sbin",
        logs_source_dir=clp_build_dir / "integration-tests" / "downloads",
        test_root_dir=clp_build_dir / "integration-tests",
    )
    config.test_root_dir.mkdir(parents=True, exist_ok=True)
    config.logs_source_dir.mkdir(parents=True, exist_ok=True)
    return config
