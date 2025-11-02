"""Pytest config: register fixtures, validate env, adjust paths, and other test setup."""

import sys
from pathlib import Path

from _pytest.config import Config

from tests.utils.utils import get_env_var

pytest_plugins = [
    "tests.fixtures.integration_test_config",
    "tests.fixtures.integration_test_logs",
    "tests.fixtures.package_instance_fixtures",
    "tests.fixtures.package_config_fixtures",
]


def pytest_configure(config: Config) -> None:  # noqa: ARG001
    """Adds <CLP_PACKAGE_DIR>/lib/python3/site-packages to the path."""
    # Retrieve and validate path to package directory.
    clp_package_dir = Path(get_env_var("CLP_PACKAGE_DIR")).expanduser().resolve()
    if not clp_package_dir:
        err_msg = (
            "CLP_PACKAGE_DIR is not set. Point it to the root of the CLP package that contains"
            " 'lib/python3/site-packages'."
        )
        raise RuntimeError(err_msg)

    # Construct and validate path to site_packages
    site_packages = Path(clp_package_dir) / "lib" / "python3" / "site-packages"
    if not site_packages.is_dir():
        err_msg = (
            f"Could not find the 'site-packages' directory at: '{site_packages}'. Verify"
            " CLP_PACKAGE_DIR points at the CLP package root."
        )
        raise RuntimeError(err_msg)

    # Sanity check: ensure clp_py_utils exists within site_packages.
    if not (site_packages / "clp_py_utils").exists():
        err_msg = (
            f"'clp_py_utils' not found under: '{site_packages}'. These integration tests expect the"
            " package to contain clp_py_utils."
        )
        raise RuntimeError(err_msg)

    sys.path.insert(0, str(site_packages))
