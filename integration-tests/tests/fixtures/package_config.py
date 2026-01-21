"""Fixtures that create and remove temporary config files for CLP packages."""

import logging
from collections.abc import Iterator

import pytest
from clp_py_utils.clp_config import (
    CLP_DEFAULT_DATA_DIRECTORY_PATH,
    CLP_DEFAULT_TMP_DIRECTORY_PATH,
)

from tests.utils.clp_job_utils import (
    build_package_job_list,
)
from tests.utils.clp_mode_utils import (
    get_clp_config_from_mode,
    get_required_component_list,
)
from tests.utils.config import PackageConfig, PackagePathConfig
from tests.utils.port_utils import assign_ports_from_base
from tests.utils.utils import unlink

logger = logging.getLogger(__name__)


@pytest.fixture
def fixt_package_config(
    request: pytest.FixtureRequest,
    fixt_package_path_config: PackagePathConfig,
) -> Iterator[PackageConfig]:
    """
    Creates and maintains a PackageConfig object for a specific CLP mode.

    :param request:
    :return: An iterator that yields the PackageConfig object for the specified mode.
    :raise ValueError: if the CLP base port's value is invalid.
    """
    mode_name: str = request.param
    logger.info("Setting up the '%s' package...", mode_name)

    # Get the ClpConfig object for this mode.
    clp_config_obj = get_clp_config_from_mode(mode_name)

    # Assign ports based on the `--base-port` CLI option.
    base_port_string = request.config.getoption("--base-port")
    try:
        base_port = int(base_port_string)
    except ValueError as err:
        err_msg = f"Invalid value '{base_port_string}' for '--base-port'; expected an integer."
        raise ValueError(err_msg) from err
    assign_ports_from_base(base_port, clp_config_obj)

    # Get list of required components.
    required_components = get_required_component_list(clp_config_obj)

    # Build the job list for this mode and the current job filter.
    no_jobs: bool = bool(request.config.option.NO_JOBS)
    job_filter: str = request.config.option.JOB_NAME_CONTAINS or ""
    package_job_list = None if no_jobs else build_package_job_list(mode_name, job_filter)

    # Construct PackageConfig.
    package_config = PackageConfig(
        path_config=fixt_package_path_config,
        mode_name=mode_name,
        component_list=required_components,
        clp_config=clp_config_obj,
        base_port=base_port,
        package_job_list=package_job_list,
    )

    try:
        yield package_config
    finally:
        logger.info("Cleaning up the '%s' package...", mode_name)
        package_config.temp_config_file_path.unlink(missing_ok=True)

        data_dir = package_config.path_config.clp_package_dir / CLP_DEFAULT_DATA_DIRECTORY_PATH
        tmp_dir = package_config.path_config.clp_package_dir / CLP_DEFAULT_TMP_DIRECTORY_PATH
        for directory_path in (data_dir, tmp_dir):
            unlink(directory_path)
