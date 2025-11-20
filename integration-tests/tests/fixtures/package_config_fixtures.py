"""Fixtures that create and remove temporary config files for CLP packages."""

import contextlib
import logging
from collections.abc import Iterator
from pathlib import Path

import pytest

from tests.utils.clp_function_utils import (
    COMPRESS_JOBS,
    SEARCH_JOBS,
)
from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS, get_clp_config_from_mode
from tests.utils.config import (
    CompressJob,
    PackageConfig,
    PackageFunctionalityList,
    SearchJob,
)
from tests.utils.utils import get_env_var

logger = logging.getLogger(__name__)


def _build_package_config_for_mode(mode_name: str) -> PackageConfig:
    """
    Constructs a PackageConfig for the specified CLP mode.

    :param mode_name:
    :return: A PackageConfig object configured for the given mode.
    :raise: KeyError if the mode name is unknown.
    :raise: Propagates get_env_var exceptions.
    """
    if mode_name not in CLP_MODE_CONFIGS:
        err_msg = f"Unknown CLP mode '{mode_name}'."
        raise KeyError(err_msg)

    clp_package_dir = Path(get_env_var("CLP_PACKAGE_DIR")).expanduser().resolve()
    test_root_dir = Path(get_env_var("CLP_BUILD_DIR")).expanduser().resolve() / "integration-tests"

    return PackageConfig(
        clp_package_dir=clp_package_dir,
        test_root_dir=test_root_dir,
        mode_name=mode_name,
    )


def _matches_keyword(job_name: str, keyword_filter: str) -> bool:
    """Return True if this job should be included given the current -k filter."""
    if not keyword_filter:
        return True
    return keyword_filter.lower() in job_name.lower()


@pytest.fixture
def clp_config(
    request: pytest.FixtureRequest,
) -> Iterator[PackageConfig]:
    """Creates and maintains a PackageConfig object for a specific CLP mode."""
    mode_name: str = request.param
    logger.info("Creating a temporary config file for the %s package...", mode_name)

    package_config = _build_package_config_for_mode(mode_name)

    # Create the temp config file.
    clp_config_obj = get_clp_config_from_mode(mode_name)
    temp_config_file_path = PackageConfig.write_temp_config_file(
        clp_config=clp_config_obj,
        temp_config_dir=package_config.temp_config_dir,
        mode_name=mode_name,
    )

    logger.info("The temporary config file has been written for the %s package.", mode_name)

    # Build the functionality list for this mode and the current job filter,
    # unless the --no-jobs flag requests a configuration without jobs.
    no_jobs: bool = bool(request.config.option.NO_JOBS)
    job_filter: str = request.config.option.JOB_NAME or ""

    if not no_jobs:
        logger.info("Creating job list for mode %s (job filter: %s)", mode_name, job_filter)

        compress_jobs: list[CompressJob] = []
        search_jobs: list[SearchJob] = []

        for job_name, compress_job in COMPRESS_JOBS.items():
            if compress_job.mode == mode_name and _matches_keyword(job_name, job_filter):
                compress_jobs.append(compress_job)

        for job_name, search_job in SEARCH_JOBS.items():
            if search_job.mode == mode_name and _matches_keyword(job_name, job_filter):
                search_jobs.append(search_job)
                if search_job.compress_job not in compress_jobs:
                    compress_jobs.append(search_job.compress_job)

        package_functionality_list = PackageFunctionalityList(
            compress_jobs=compress_jobs,
            search_jobs=search_jobs,
        )
        object.__setattr__(package_config, "package_functionality_list", package_functionality_list)
        logger.info("The jobs list for mode %s was created successfully.", mode_name)
    else:
        logger.info(
            "The --no-jobs flag is set. Job list creation is disabled for mode %s.",
            mode_name,
        )
        # Leave package_config.package_functionality_list at its default (likely None).

    try:
        yield package_config
    finally:
        logger.info("Removing the temporary config file...")
        with contextlib.suppress(FileNotFoundError):
            temp_config_file_path.unlink()
        logger.info("The temporary config file has been removed.")
