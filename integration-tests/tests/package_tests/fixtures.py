"""Session-scoped path configuration fixtures shared across package integration tests."""

import logging
from collections.abc import Iterator

import pytest

from tests.package_tests.classes import (
    ClpPackage,
    ClpPackageModeConfig,
    ClpPackageTestPathConfig,
)
from tests.package_tests.utils.start_stop import (
    start_clp_package,
    stop_clp_package,
    verify_start_clp_action,
    verify_stop_clp_action,
)
from tests.utils.port_utils import assign_ports_from_base
from tests.utils.utils import resolve_path_env_var, write_dict_to_yaml

logger = logging.getLogger(__name__)


@pytest.fixture(scope="session")
def clp_package_test_path_config() -> ClpPackageTestPathConfig:
    """Provides paths relevant to all package integration tests."""
    return ClpPackageTestPathConfig(
        clp_build_dir=resolve_path_env_var("CLP_BUILD_DIR"),
        integration_tests_project_root=resolve_path_env_var("INTEGRATION_TESTS_PROJECT_ROOT"),
        clp_package_dir=resolve_path_env_var("CLP_PACKAGE_DIR"),
    )


@pytest.fixture(scope="module")
def clp_package(
    request: pytest.FixtureRequest,
    clp_package_test_path_config: ClpPackageTestPathConfig,
) -> Iterator[ClpPackage]:
    """Docstring for clp_package fixture."""
    mode_config: ClpPackageModeConfig = request.param
    mode_name = mode_config.mode_name
    clp_config = mode_config.clp_config
    component_list = mode_config.component_list

    # Assign ports to the `ClpConfig` pydantic object.
    logger.info("Assigning ports to the '%s' package.", mode_name)
    base_port_string = request.config.getoption("--base-port")
    try:
        base_port = int(base_port_string)
    except ValueError as err:
        err_msg = f"Invalid value '{base_port_string}' for '--base-port'; expected an integer."
        raise ValueError(err_msg) from err
    assign_ports_from_base(base_port, clp_config)

    # Write the temporary config file.
    logger.info("Writing the temporary config file for the '%s' package.", mode_name)
    temp_config_file_path = (
        clp_package_test_path_config.temp_config_dir / f"clp-config-{mode_name}.yaml"
    )
    write_dict_to_yaml(
        clp_config.dump_to_primitive_dict(),  # type: ignore[no-untyped-call]
        temp_config_file_path,
    )

    # Construct `ClpPackage` object.
    clp_package = ClpPackage(
        path_config=clp_package_test_path_config,
        mode_name=mode_name,
        clp_config=clp_config,
        component_list=component_list,
    )

    try:
        start_clp_action = start_clp_package(clp_package)
        start_result = verify_start_clp_action(start_clp_action, clp_package)
        assert start_result, start_result.failure_message
        yield clp_package
    finally:
        stop_clp_action = stop_clp_package(clp_package)
        stop_result = verify_stop_clp_action(stop_clp_action, clp_package)
        assert stop_result, stop_result.failure_message

        clp_package.temp_config_file_path.unlink(missing_ok=True)
