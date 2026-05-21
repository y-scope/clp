"""Provides definition of the clp-json operating mode."""

from clp_py_utils.clp_config import (
    ClpConfig,
    Package,
    QueryEngine,
    StorageEngine,
)

from tests.package_tests.classes import (
    ClpPackageModeConfig,
)
from tests.package_tests.utils.modes import (
    CLP_API_SERVER_COMPONENT,
    CLP_BASE_COMPONENTS,
    CLP_GARBAGE_COLLECTOR_COMPONENT,
    CLP_QUERY_COMPONENTS,
    CLP_REDUCER_COMPONENT,
)

CLP_JSON_MODE = ClpPackageModeConfig(
    mode_name="clp-json",
    clp_config=ClpConfig(
        package=Package(
            storage_engine=StorageEngine.CLP_S,
            query_engine=QueryEngine.CLP_S,
        ),
    ),
    component_list=(
        *CLP_BASE_COMPONENTS,
        CLP_REDUCER_COMPONENT,
        *CLP_QUERY_COMPONENTS,
        CLP_GARBAGE_COLLECTOR_COMPONENT,
        CLP_API_SERVER_COMPONENT,
    ),
)
