from pathlib import Path

import pytest

from tests.utils.config import BaseConfig
from tests.utils.utils import get_env_var


@pytest.fixture(scope="session")
def base_config() -> BaseConfig:
    clp_build_dir = Path(get_env_var("CLP_BUILD_DIR"))
    clp_package_dir = Path(get_env_var("CLP_PACKAGE_DIR"))

    base_config = BaseConfig(
        clp_bin_dir=clp_package_dir / "bin",
        clp_package_dir=clp_package_dir,
        clp_sbin_dir=clp_package_dir / "sbin",
        test_output_dir=clp_build_dir / "var" / "logs" / "pytest",
        uncompressed_logs_dir=clp_build_dir / "var" / "data" / "pytest" / "downloads",
    )
    base_config.test_output_dir.mkdir(parents=True, exist_ok=True)
    base_config.uncompressed_logs_dir.mkdir(parents=True, exist_ok=True)
    return base_config
