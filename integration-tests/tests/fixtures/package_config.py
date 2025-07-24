from pathlib import Path

import pytest
from tests.utils.config import PackageConfig
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def package_config() -> PackageConfig:
    clp_build_dir = Path(get_env_var("CLP_BUILD_DIR")).resolve()
    clp_package_dir = Path(get_env_var("CLP_PACKAGE_DIR")).resolve()

    config = PackageConfig(
        clp_bin_dir=clp_package_dir / "bin",
        clp_package_dir=clp_package_dir,
        clp_sbin_dir=clp_package_dir / "sbin",
        test_output_dir=clp_build_dir / "integration-tests",
        uncompressed_logs_dir=clp_build_dir / "integration-tests" / "downloads",
    )
    config.test_output_dir.mkdir(parents=True, exist_ok=True)
    config.uncompressed_logs_dir.mkdir(parents=True, exist_ok=True)
    return config
