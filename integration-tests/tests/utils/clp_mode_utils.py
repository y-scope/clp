"""Provides utilities related to the user-level configurations of CLP's operating modes."""

from collections.abc import Callable
from typing import Any

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
    Presto,
    PRESTO_COORDINATOR_COMPONENT_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    QueryEngine,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    ResultsCache,
    StorageEngine,
    WEBUI_COMPONENT_NAME,
)
from pydantic import BaseModel

CLP_MODE_CONFIGS: dict[str, Callable[[], ClpConfig]] = {
    "clp-text": lambda: ClpConfig(
        package=Package(
            storage_engine=StorageEngine.CLP,
            query_engine=QueryEngine.CLP,
        ),
        api_server=None,
        log_ingestor=None,
    ),
    "clp-json": lambda: ClpConfig(
        package=Package(
            storage_engine=StorageEngine.CLP_S,
            query_engine=QueryEngine.CLP_S,
        ),
    ),
    "clp-presto": lambda: ClpConfig(
        package=Package(
            storage_engine=StorageEngine.CLP_S,
            query_engine=QueryEngine.PRESTO,
        ),
        results_cache=ResultsCache(
            retention_period=None,
        ),
        presto=Presto(
            host="localhost",
            port=8889,
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


# These component lists should be maintained alongside the CLP_MODE_CONFIGS list.
# TODO: Modify these component lists when the Presto Docker Compose project is integrated with the
# CLP Docker compose project.
CLP_BASE_COMPONENTS = [
    _to_docker_compose_service_name(DB_COMPONENT_NAME),
    _to_docker_compose_service_name(QUEUE_COMPONENT_NAME),
    _to_docker_compose_service_name(REDIS_COMPONENT_NAME),
    _to_docker_compose_service_name(RESULTS_CACHE_COMPONENT_NAME),
    _to_docker_compose_service_name(COMPRESSION_SCHEDULER_COMPONENT_NAME),
    _to_docker_compose_service_name(COMPRESSION_WORKER_COMPONENT_NAME),
    _to_docker_compose_service_name(WEBUI_COMPONENT_NAME),
]

CLP_QUERY_COMPONENTS = [
    _to_docker_compose_service_name(QUERY_SCHEDULER_COMPONENT_NAME),
    _to_docker_compose_service_name(QUERY_WORKER_COMPONENT_NAME),
]

CLP_REDUCER_COMPONENT = _to_docker_compose_service_name(REDUCER_COMPONENT_NAME)
CLP_API_SERVER_COMPONENT = _to_docker_compose_service_name(API_SERVER_COMPONENT_NAME)
CLP_GARBAGE_COLLECTOR_COMPONENT = _to_docker_compose_service_name(GARBAGE_COLLECTOR_COMPONENT_NAME)
CLP_MCP_SERVER_COMPONENT = _to_docker_compose_service_name(MCP_SERVER_COMPONENT_NAME)

CLP_PRESTO_COMPONENTS = [
    _to_docker_compose_service_name(PRESTO_COORDINATOR_COMPONENT_NAME),
    "presto-worker",
]


def compare_mode_signatures(intended_config: ClpConfig, running_config: ClpConfig) -> bool:
    """
    Compares the running signatures of `intended_config` and `running_config` with
    `_match_objects_by_explicit_fields`.

    :param intended_config: The reference config object.
    :param running_config: The config object to compare against.
    :return: True if config objects match, False otherwise.
    """
    return _match_objects_by_explicit_fields(intended_config, running_config)


def get_clp_config_from_mode(mode_name: str) -> ClpConfig:
    """
    Return a ClpConfig object for the given mode name.

    :param mode_name:
    :return: ClpConfig object corresponding to the mode.
    :raise ValueError: If the mode is not supported.
    """
    if mode_name not in CLP_MODE_CONFIGS:
        err_msg = f"Unsupported mode: '{mode_name}'"
        raise ValueError(err_msg)
    return CLP_MODE_CONFIGS[mode_name]()


def get_required_component_list(config: ClpConfig) -> list[str]:
    """
    Constructs a list of the components that the CLP package described in `config` needs to run
    properly.

    This function should be maintained alongside the CLP_MODE_CONFIGS list.

    :param config:
    :return: List of components required by the package.
    """
    component_list: list[str] = list(CLP_BASE_COMPONENTS)

    deployment_type = config.get_deployment_type()
    if deployment_type == DeploymentType.FULL:
        component_list.extend(CLP_QUERY_COMPONENTS)
        component_list.append(CLP_REDUCER_COMPONENT)

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


def _match_objects_by_explicit_fields(intended_obj: Any, running_obj: Any) -> bool:
    """
    Compares `intended_obj` and `running_obj` using Pydantic's `model_fields_set` to recursively
    match only the fields that were explicitly set when `intended_obj` was initialized.

    When both objects are Pydantic models, the function only inspects fields that were explicitly
    set on the `intended_obj`. For other data types, the function uses standard equality.

    :param intended_obj:
    :param running_obj:
    :return: True if all explicitly set fields in intended_obj match running_obj, False otherwise.
    """
    if isinstance(intended_obj, BaseModel):
        if not isinstance(running_obj, BaseModel):
            return False

        for field_name in intended_obj.model_fields_set:
            intended_field_value = getattr(intended_obj, field_name)
            running_field_value = getattr(running_obj, field_name)

            if not _match_objects_by_explicit_fields(intended_field_value, running_field_value):
                return False

        return True

    return bool(intended_obj == running_obj)
