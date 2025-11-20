"""Fixtures that start and stop CLP package instances for integration tests."""

import logging
import subprocess
from collections.abc import Iterator

import pytest

from tests.utils.clp_mode_utils import (
    get_clp_config_from_mode,
    get_required_component_list,
)
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
    :param request:
    :return: Iterator that yields the running package instance.
    """
    mode_name = clp_config.mode_name
    instance: PackageInstance | None = None

    try:
        logger.debug("Starting up the %s package.", mode_name)

        try:
            start_clp_package(clp_config)
        except RuntimeError:
            base_port_string = request.config.getini("BASE_PORT")
            pytest.fail(
                f"Failed to start the {mode_name} package. This could mean that one of the ports"
                f" derived from BASE_PORT={base_port_string} was unavailable. Try changing"
                " BASE_PORT in .pytest.ini."
            )

        required_components = get_required_component_list(get_clp_config_from_mode(mode_name))
        instance = PackageInstance(
            package_config=clp_config,
            component_list=required_components,
        )

        yield instance

    finally:
        logger.info("Now stopping the %s package...", mode_name)
        if instance is not None:
            stop_clp_package(instance)
        else:
            # This means setup failed after start; fall back to calling stop script directly
            subprocess.run([str(clp_config.stop_script_path)], check=False)
