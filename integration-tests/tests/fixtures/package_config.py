"""Fixtures that create and remove temporary config files for CLP packages."""

import contextlib
import logging
from collections.abc import Iterator

import pytest

from tests.utils.clp_job_utils import (
    PACKAGE_COMPRESS_JOBS,
    PACKAGE_SEARCH_JOBS,
)
from tests.utils.clp_mode_utils import (
    get_clp_config_from_mode,
    get_required_component_list,
)
from tests.utils.config import (
    PackageCompressJob,
    PackageConfig,
    PackageJobList,
    PackagePathConfig,
    PackageSearchJob,
)
from tests.utils.port_utils import assign_ports_from_base

logger = logging.getLogger(__name__)


def _matches_keyword(job_name: str, keyword_filter: str) -> bool:
    """Return True if this job should be included given the current -k filter."""
    if not keyword_filter:
        return True
    return keyword_filter.lower() in job_name.lower()


def _build_package_job_list(mode_name: str, job_filter: str) -> PackageJobList | None:
    logger.info("Creating job list for mode %s (job filter: %s)", mode_name, job_filter)

    package_compress_jobs: list[PackageCompressJob] = []
    package_search_jobs: list[PackageSearchJob] = []

    for job_name, package_compress_job in PACKAGE_COMPRESS_JOBS.items():
        if package_compress_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_compress_jobs.append(package_compress_job)

    for job_name, package_search_job in PACKAGE_SEARCH_JOBS.items():
        if package_search_job.mode == mode_name and _matches_keyword(job_name, job_filter):
            package_search_jobs.append(package_search_job)
            if package_search_job.package_compress_job not in package_compress_jobs:
                package_compress_jobs.append(package_search_job.package_compress_job)

    if not package_compress_jobs and not package_search_jobs:
        return None
    return PackageJobList(
        package_compress_jobs=package_compress_jobs,
        package_search_jobs=package_search_jobs,
    )


@pytest.fixture
def fixt_package_config(
    fixt_package_path_config: PackagePathConfig,
    request: pytest.FixtureRequest,
) -> Iterator[PackageConfig]:
    """
    Creates and maintains a PackageConfig object for a specific CLP mode.

    :param request:
    :return: An iterator that yields the PackageConfig object for the specified mode.
    """
    mode_name: str = request.param
    logger.debug("Creating a temporary config file for the %s package.", mode_name)

    # Get the ClpConfig for this mode.
    clp_config_obj = get_clp_config_from_mode(mode_name)

    # Assign ports based on BASE_PORT from ini.
    base_port_string = request.config.getini("BASE_PORT")
    try:
        base_port = int(base_port_string)
    except ValueError as err:
        err_msg = (
            f"Invalid BASE_PORT value '{base_port_string}' in pytest.ini; expected an integer."
        )
        raise ValueError(err_msg) from err

    assign_ports_from_base(base_port, clp_config_obj)

    # Compute the list of required components for this mode.
    required_components = get_required_component_list(clp_config_obj)

    # Build the job list for this mode and the current job filter.
    no_jobs: bool = bool(request.config.option.NO_JOBS)
    job_filter: str = request.config.option.JOB_NAME or ""
    package_job_list = None if no_jobs else _build_package_job_list(mode_name, job_filter)

    # Construct PackageConfig.
    package_config = PackageConfig(
        path_config=fixt_package_path_config,
        mode_name=mode_name,
        component_list=required_components,
        clp_config=clp_config_obj,
        package_job_list=package_job_list,
    )

    try:
        yield package_config
    finally:
        logger.debug("Removing the temporary config file.")
        with contextlib.suppress(FileNotFoundError):
            package_config.temp_config_file_path.unlink()
