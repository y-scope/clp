"""Provides utilities related to the user-level configurations of CLP's operating modes."""

from collections.abc import Callable

from clp_py_utils.clp_config import (
    API_SERVER_COMPONENT_NAME,
    ApiServer,
    ClpConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    Database,
    DB_COMPONENT_NAME,
    DeploymentType,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
    MCP_SERVER_COMPONENT_NAME,
    Package,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    QueryEngine,
    QueryScheduler,
    Queue,
    QUEUE_COMPONENT_NAME,
    Redis,
    REDIS_COMPONENT_NAME,
    Reducer,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    ResultsCache,
    StorageEngine,
    WebUi,
    WEBUI_COMPONENT_NAME,
)

BASE_PORT = 50000


def _make_clp_config_with_ports(
    package: Package,
    enable_api_server: bool,
    base_port: int = BASE_PORT,
) -> ClpConfig:
    """
    Build a ClpConfig where the main networked components listen on ports
    starting at base_port.
    """
    database_port = base_port + 0
    queue_port = base_port + 1
    redis_port = base_port + 2
    results_cache_port = base_port + 3
    query_scheduler_port = base_port + 4
    reducer_base_port = base_port + 5
    webui_port = base_port + 6
    api_server_port = base_port + 7

    return ClpConfig(
        package=package,
        database=Database(port=database_port),
        queue=Queue(port=queue_port),
        redis=Redis(port=redis_port),
        results_cache=ResultsCache(port=results_cache_port),
        query_scheduler=QueryScheduler(port=query_scheduler_port),
        reducer=Reducer(base_port=reducer_base_port),
        webui=WebUi(port=webui_port),
        api_server=ApiServer(port=api_server_port) if enable_api_server else None,
    )


CLP_MODE_CONFIGS: dict[str, Callable[[], ClpConfig]] = {
    "clp-text": lambda: _make_clp_config_with_ports(
        package=Package(
            storage_engine=StorageEngine.CLP,
            query_engine=QueryEngine.CLP,
        ),
        enable_api_server=False,
        base_port=50000,
    ),
    "clp-json": lambda: _make_clp_config_with_ports(
        package=Package(
            storage_engine=StorageEngine.CLP_S,
            query_engine=QueryEngine.CLP_S,
        ),
        enable_api_server=True,
        base_port=50000,
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
    Constructs a list of the components that the CLP package described in `config` needs to run
    properly.

    :param config:
    :return: List of components required by the package.
    """
    component_list: list[str] = []
    component_list.extend(CLP_BASE_COMPONENTS)

    deployment_type = config.get_deployment_type()
    if deployment_type == DeploymentType.FULL:
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
