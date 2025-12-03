"""Provides utilities related to the user-level configurations of CLP's operating modes."""

from collections.abc import Callable

from clp_py_utils.clp_config import (
    API_SERVER_COMPONENT_NAME,
    ClpConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    DB_COMPONENT_NAME,
    DeploymentType,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
    MCP_SERVER_COMPONENT_NAME,
    Package,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    QueryEngine,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    StorageEngine,
    WEBUI_COMPONENT_NAME,
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


# TODO: This will eventually be replaced by a formalized mapping between component and service.
def _to_docker_compose_service_name(name: str) -> str:
    """
    Convert a component name to a Docker Compose service name.

    :param name:
    :return: Service name with underscores replaced by hyphens
    """
    return name.replace("_", "-")


# TODO: Modify these component lists when the Presto Docker Compose project is integrated with the
# CLP Docker compose project.
CLP_BASE_COMPONENTS = [
    _to_docker_compose_service_name(DB_COMPONENT_NAME),
    _to_docker_compose_service_name(QUEUE_COMPONENT_NAME),
    _to_docker_compose_service_name(REDIS_COMPONENT_NAME),
    _to_docker_compose_service_name(REDUCER_COMPONENT_NAME),
    _to_docker_compose_service_name(RESULTS_CACHE_COMPONENT_NAME),
    _to_docker_compose_service_name(COMPRESSION_SCHEDULER_COMPONENT_NAME),
    _to_docker_compose_service_name(COMPRESSION_WORKER_COMPONENT_NAME),
    _to_docker_compose_service_name(WEBUI_COMPONENT_NAME),
]

CLP_QUERY_COMPONENTS = [
    _to_docker_compose_service_name(QUERY_SCHEDULER_COMPONENT_NAME),
    _to_docker_compose_service_name(QUERY_WORKER_COMPONENT_NAME),
]

CLP_API_SERVER_COMPONENT = _to_docker_compose_service_name(API_SERVER_COMPONENT_NAME)
CLP_GARBAGE_COLLECTOR_COMPONENT = _to_docker_compose_service_name(GARBAGE_COLLECTOR_COMPONENT_NAME)
CLP_MCP_SERVER_COMPONENT = _to_docker_compose_service_name(MCP_SERVER_COMPONENT_NAME)


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


def get_required_component_list(config: ClpConfig) -> list[str]:
    """
    Constructs the list of components required for the CLP package described in `config` to run
    properly.

    :param config:
    :return: List of components required by the package.
    """
    component_list: list[str] = list(CLP_BASE_COMPONENTS)

    deployment_type = config.get_deployment_type()
    if DeploymentType.FULL == deployment_type:
        component_list.extend(CLP_QUERY_COMPONENTS)

    if config.api_server is not None:
        component_list.append(CLP_API_SERVER_COMPONENT)

    if (
        config.archive_output.retention_period is not None
        or config.results_cache.retention_period is not None
    ):
        component_list.append(CLP_GARBAGE_COLLECTOR_COMPONENT)

    if config.mcp_server is not None:
        component_list.append(CLP_MCP_SERVER_COMPONENT)

    return component_list
