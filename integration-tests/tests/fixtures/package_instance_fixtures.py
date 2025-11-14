"""Fixtures that start and stop CLP package instances for integration tests."""

import logging
import subprocess
from collections.abc import Iterator

import pytest

from tests.utils.clp_mode_utils import CLP_MODE_CONFIGS
from tests.utils.config import (
    PackageConfig,
    PackageInstance,
)
from tests.utils.package_utils import (
    start_clp_package,
    stop_clp_package,
)

logger = logging.getLogger(__name__)


@pytest.fixture
def clp_package(
    clp_config: PackageConfig,
    request: pytest.FixtureRequest,
) -> Iterator[PackageInstance]:
    """
    Starts a CLP package instance for the given configuration and stops it during teardown.

    :param clp_config:
    :return: Iterator that yields the running package instance.
    :raise: Propagates exceptions from start_clp_package.
    :raise: Propagates exceptions from stop_clp_package.
    :raise: Propagates KeyError if the mode name is not present in CLP_MODE_CONFIGS.
    :raise: Propagates subprocess.CalledProcessError if the stop script fails.
    """
    mode_name = clp_config.mode_name
    no_jobs: bool = bool(request.config.option.NO_JOBS)

    # Do not start this mode if it has no jobs, unless --no-jobs requested jobless startup.
    package_functionality_list = getattr(clp_config, "package_functionality_list", None)

    if package_functionality_list is None:
        if not no_jobs:
            pytest.fail("package_functionality_list was not set on PackageConfig by clp_config")
    elif (
        not package_functionality_list.compress_jobs
        and not package_functionality_list.search_jobs
    ):
        if not no_jobs:
            pytest.skip(f"No jobs to run for mode {mode_name} with current job filter")

    logger.info("Starting up the %s package...", mode_name)

    # Start the package using the pre-written temp config file.
    start_clp_package(clp_config)

    instance: PackageInstance | None = None
    try:
        required_components = CLP_MODE_CONFIGS[mode_name][1]
        instance = PackageInstance(package_config=clp_config, component_list=required_components)
        instance_id = instance.clp_instance_id
        logger.info(
            "An instance of the %s package was started successfully. Its instance ID is '%s'",
            mode_name,
            instance_id,
        )
        yield instance
    finally:
        logger.info("Now stopping the %s package...", mode_name)
        if instance is not None:
            stop_clp_package(instance)
        else:
            subprocess.run([str(clp_config.stop_script_path)], check=True)
        logger.info("The %s package was stopped successfully.", mode_name)
