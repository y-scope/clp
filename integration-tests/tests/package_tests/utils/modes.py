"""Provides utilities related to the user-level configurations of CLP's operating modes."""

from clp_py_utils.clp_config import (
    API_SERVER_COMPONENT_NAME,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    DB_COMPONENT_NAME,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
    LOG_INGESTOR_COMPONENT_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    WEBUI_COMPONENT_NAME,
)


# TODO: This will eventually be replaced by a formalized mapping between component and service.
def _to_docker_compose_service_name(name: str) -> str:
    """
    Convert a component name to a Docker Compose service name.

    :param name:
    :return: Service name with underscores replaced by hyphens
    """
    return name.replace("_", "-")


# Names of components that may comprise a given package mode.
CLP_BASE_COMPONENTS: tuple[str, ...] = (
    _to_docker_compose_service_name(DB_COMPONENT_NAME),
    _to_docker_compose_service_name(QUEUE_COMPONENT_NAME),
    _to_docker_compose_service_name(REDIS_COMPONENT_NAME),
    _to_docker_compose_service_name(RESULTS_CACHE_COMPONENT_NAME),
    _to_docker_compose_service_name(COMPRESSION_SCHEDULER_COMPONENT_NAME),
    _to_docker_compose_service_name(COMPRESSION_WORKER_COMPONENT_NAME),
    _to_docker_compose_service_name(WEBUI_COMPONENT_NAME),
)
CLP_REDUCER_COMPONENT = _to_docker_compose_service_name(REDUCER_COMPONENT_NAME)
CLP_QUERY_COMPONENTS: tuple[str, ...] = (
    _to_docker_compose_service_name(QUERY_SCHEDULER_COMPONENT_NAME),
    _to_docker_compose_service_name(QUERY_WORKER_COMPONENT_NAME),
)
CLP_GARBAGE_COLLECTOR_COMPONENT = _to_docker_compose_service_name(GARBAGE_COLLECTOR_COMPONENT_NAME)
CLP_API_SERVER_COMPONENT = _to_docker_compose_service_name(API_SERVER_COMPONENT_NAME)
CLP_LOG_INGESTOR_COMPONENT = _to_docker_compose_service_name(LOG_INGESTOR_COMPONENT_NAME)
