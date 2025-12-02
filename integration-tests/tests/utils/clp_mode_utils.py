"""Provides utilities related to the user-level configurations of CLP's operating modes."""

from collections.abc import Callable

from clp_py_utils.clp_config import (
    ClpConfig,
    Package,
    QueryEngine,
    StorageEngine,
)

CLP_MODE_CONFIGS: dict[str, Callable[[], ClpConfig]] = {
    "clp-text": lambda: ClpConfig(
        package=Package(
            storage_engine=StorageEngine.CLP,
            query_engine=QueryEngine.CLP,
        ),
        api_server=None,
    ),
    "clp-json": lambda: ClpConfig(
        package=Package(
            storage_engine=StorageEngine.CLP_S,
            query_engine=QueryEngine.CLP_S,
        ),
    ),
}


def get_clp_config_from_mode(mode_name: str) -> ClpConfig:
    """
    Return a ClpConfig object for the given mode name.

    :param mode_name:
    :return: ClpConfig object corresponding to the mode.
    :raise ValueError: If the mode is not supported.
    """
    if mode_name not in CLP_MODE_CONFIGS:
        err_msg = f"Unsupported mode: {mode_name}"
        raise ValueError(err_msg)
    return CLP_MODE_CONFIGS[mode_name]()
