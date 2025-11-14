"""Provides utilities related to the user-level configurations of CLP's operating modes."""

from collections.abc import Callable
from typing import Any

from clp_py_utils.clp_config import (
    CLPConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    DB_COMPONENT_NAME,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
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


# TODO: This will eventually be replaced by a formalized mapping between component and service.
def _to_docker_compose_service_name(name: str) -> str:
    """
    Convert a component name to a Docker Compose service name.

    :param name:
    :return: Service name with underscores replaced by hyphens
    """
    return name.replace("_", "-")


CLP_MODE_CONFIGS: dict[str, tuple[Callable[[], CLPConfig], list[str]]] = {
    "clp-text": (
        lambda: CLPConfig(
            package=Package(
                storage_engine=StorageEngine.CLP,
                query_engine=QueryEngine.CLP,
            ),
        ),
        [
            _to_docker_compose_service_name(DB_COMPONENT_NAME),
            _to_docker_compose_service_name(QUEUE_COMPONENT_NAME),
            _to_docker_compose_service_name(REDIS_COMPONENT_NAME),
            _to_docker_compose_service_name(REDUCER_COMPONENT_NAME),
            _to_docker_compose_service_name(RESULTS_CACHE_COMPONENT_NAME),
            _to_docker_compose_service_name(COMPRESSION_SCHEDULER_COMPONENT_NAME),
            _to_docker_compose_service_name(QUERY_SCHEDULER_COMPONENT_NAME),
            _to_docker_compose_service_name(COMPRESSION_WORKER_COMPONENT_NAME),
            _to_docker_compose_service_name(QUERY_WORKER_COMPONENT_NAME),
            _to_docker_compose_service_name(WEBUI_COMPONENT_NAME),
            _to_docker_compose_service_name(GARBAGE_COLLECTOR_COMPONENT_NAME),
        ],
    ),
    "clp-json": (
        lambda: CLPConfig(
            package=Package(
                storage_engine=StorageEngine.CLP_S,
                query_engine=QueryEngine.CLP_S,
            ),
        ),
        [
            _to_docker_compose_service_name(DB_COMPONENT_NAME),
            _to_docker_compose_service_name(QUEUE_COMPONENT_NAME),
            _to_docker_compose_service_name(REDIS_COMPONENT_NAME),
            _to_docker_compose_service_name(REDUCER_COMPONENT_NAME),
            _to_docker_compose_service_name(RESULTS_CACHE_COMPONENT_NAME),
            _to_docker_compose_service_name(COMPRESSION_SCHEDULER_COMPONENT_NAME),
            _to_docker_compose_service_name(QUERY_SCHEDULER_COMPONENT_NAME),
            _to_docker_compose_service_name(COMPRESSION_WORKER_COMPONENT_NAME),
            _to_docker_compose_service_name(QUERY_WORKER_COMPONENT_NAME),
            _to_docker_compose_service_name(WEBUI_COMPONENT_NAME),
            _to_docker_compose_service_name(GARBAGE_COLLECTOR_COMPONENT_NAME),
        ],
    ),
}


def compute_mode_signature(config: CLPConfig) -> tuple[Any, ...]:
    """
    Constructs a signature that captures the mode-defining aspects of a CLPConfig object.

    :param config:
    :return: Tuple that encodes fields used to determine an operating mode.
    """
    return (
        config.logs_input.type,
        config.package.storage_engine.value,
        config.package.query_engine.value,
        config.mcp_server is not None,
        config.presto is not None,
        config.archive_output.storage.type,
        config.stream_output.storage.type,
        config.aws_config_directory is not None,
        config.get_deployment_type(),
    )


def get_clp_config_from_mode(mode_name: str) -> CLPConfig:
    """
    Return a CLPConfig object for the given mode name.

    :param mode_name:
    :return: CLPConfig object corresponding to the mode.
    :raise ValueError: If the mode is not supported.
    """
    try:
        config = CLP_MODE_CONFIGS[mode_name][0]
    except KeyError as err:
        err_msg = f"Unsupported mode: {mode_name}"
        raise ValueError(err_msg) from err
    return config()
