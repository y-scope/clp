"""Integration tests verifying that the CLP package can be started and stopped."""

import logging
import shutil
import subprocess

import pytest

from tests.utils.config import (
    PackageRun,
)

pytestmark = pytest.mark.package


package_configurations = pytest.mark.parametrize(
    "test_package_fixture",
    [
        "clp_package",
    ],
)

logger = logging.getLogger(__name__)


@pytest.mark.package
@package_configurations
def test_clp_package(
    request: pytest.FixtureRequest,
    test_package_fixture: str,
) -> None:
    """
    Validate that all of the components of the clp package start up successfully. The package is
    started up in whatever configuration is currently described in clp-config.yml; default is
    clp-text.

    :param request:
    :param test_package_fixture:
    """
    package_run: PackageRun = request.getfixturevalue(test_package_fixture)

    assert _is_package_running(package_run)


def _is_package_running(package_run: PackageRun) -> bool:
    """Checks that the package specified in package_run is running correctly."""
    mode = package_run.mode
    instance_id = package_run.clp_instance_id

    component_basenames = [
        "clp-database",
        "clp-queue",
        "clp-redis",
        "clp-results_cache",
        "clp-compression_scheduler",
        "clp-query_scheduler",
        "clp-compression_worker",
        "clp-query_worker",
        "clp-reducer",
        "clp-webui",
        "clp-garbage_collector",
    ]

    logger.info(
        "Checking if all components of %s package with instance ID '%s' are running properly.",
        mode,
        instance_id,
    )

    docker_bin = shutil.which("docker")
    if docker_bin is None:
        error_msg = "docker not found in PATH"
        raise RuntimeError(error_msg)

    for component in component_basenames:
        name = f"{component}-{instance_id}"

        proc = subprocess.run(
            [docker_bin, "inspect", "-f", "{{.State.Running}}", name],
            capture_output=True,
            text=True,
            check=False,
        )

        if proc.returncode != 0:
            err = (proc.stderr or proc.stdout or "").strip()
            if "No such object" in err:
                logger.error("Component container not found: %s", name)
                return False
            error_msg = f"Error inspecting container {name}: {err}"
            raise RuntimeError(error_msg)

        status = (proc.stdout or "").strip().lower()
        if status != "true":
            logger.error("Component container not running: %s (status=%s)", name, status)
            return False

    logger.info(
        "All components of the %s package with instance ID '%s' are running properly.",
        mode,
        instance_id,
    )
    return True
