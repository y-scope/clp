from pathlib import Path

import pytest

from .fixture_types import PackageTestConfig
from .utils import get_env_var


@pytest.fixture(scope="session")
def config() -> PackageTestConfig:
    clp_build_dir = Path(get_env_var("CLP_BUILD_DIR"))
    clp_package_dir = Path(get_env_var("CLP_PACKAGE_BUILD_DIR"))

    config = PackageTestConfig(
        clp_bin_dir=clp_package_dir / "bin",
        clp_package_dir=clp_package_dir,
        clp_sbin_dir=clp_package_dir / "sbin",
        test_output_dir=clp_build_dir / "var" / "logs" / "pytest",
        uncompressed_logs_dir=clp_build_dir / "var" / "data" / "pytest" / "downloads",
    )
    config.test_output_dir.mkdir(parents=True, exist_ok=True)
    config.uncompressed_logs_dir.mkdir(parents=True, exist_ok=True)
    return config
