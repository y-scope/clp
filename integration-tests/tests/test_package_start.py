"""Integration tests verifying that the CLP package can be started and stopped."""

import logging
import shutil

import pytest

from tests.utils.config import (
    PackageInstance,
)
from tests.utils.docker_utils import (
    inspect_container_state,
    list_prefixed_containers,
)
from tests.utils.package_utils import (
    CLP_COMPONENT_BASENAMES,
)

package_configurations = pytest.mark.parametrize(
    "test_package_fixture",
    [
        "clp_text_package",
        "clp_json_package",
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
    assert _is_package_running(request.getfixturevalue(test_package_fixture))


def _is_package_running(package_instance: PackageInstance) -> bool:
    """Checks that the package specified in package_instance is running correctly."""
    mode = package_instance.package_instance_config_file.mode
    instance_id = package_instance.clp_instance_id

    logger.info(
        "Checking if all components of %s package with instance ID '%s' are running properly...",
        mode,
        instance_id,
    )

    docker_bin = shutil.which("docker")
    if docker_bin is None:
        err_msg = "docker not found in PATH"
        raise RuntimeError(err_msg)

    for component in CLP_COMPONENT_BASENAMES:
        # The container name may have any numeric suffix, and there may be multiple of them.
        prefix = f"clp-package-{instance_id}-{component}-"

        candidates = list_prefixed_containers(docker_bin, prefix)
        if not candidates:
            pytest.fail(f"No component container was found with the prefix '{prefix}'")

        # Inspect each matching container; require that each one is running.
        not_running = []
        for name in candidates:
            desired_state = "running"
            if not inspect_container_state(docker_bin, name, desired_state):
                not_running.append(name)

        if not_running:
            details = ", ".join(not_running)
            pytest.fail(f"Component containers not running: {details}")

    logger.info(
        "All components of the %s package with instance ID '%s' are running properly.",
        mode,
        instance_id,
    )
    return True
