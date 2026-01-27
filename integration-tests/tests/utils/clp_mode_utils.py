"""Provides utilities related to the user-level configurations of CLP's operating modes."""

from typing import Any

from clp_py_utils.clp_config import (
    API_SERVER_COMPONENT_NAME,
    ClpConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    DB_COMPONENT_NAME,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    WEBUI_COMPONENT_NAME,
)
from pydantic import BaseModel


# TODO: This will eventually be replaced by a formalized mapping between component and service.
def _to_docker_compose_service_name(name: str) -> str:
    """
    Convert a component name to a Docker Compose service name.

    :param name:
    :return: Service name with underscores replaced by hyphens
    """
    return name.replace("_", "-")


# Names of components that may comprise a given package mode. Test modules use these lists to
# assemble mode-specific component lists (see tests/package_tests/*/test_*.py).
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
    _to_docker_compose_service_name(QUERY_SCHEDULER_COMPONENT_NAME),
    _to_docker_compose_service_name(QUERY_WORKER_COMPONENT_NAME),
    _to_docker_compose_service_name(GARBAGE_COLLECTOR_COMPONENT_NAME),
]

CLP_API_SERVER_COMPONENT = _to_docker_compose_service_name(API_SERVER_COMPONENT_NAME)


def compare_mode_signatures(intended_config: ClpConfig, running_config: ClpConfig) -> bool:
    """
    Compares the running signatures of `intended_config` and `running_config` with
    `_match_objects_by_explicit_fields`.

    :param intended_config: The reference config object.
    :param running_config: The config object to compare against.
    :return: True if config objects match, False otherwise.
    """
    return _match_objects_by_explicit_fields(intended_config, running_config)


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
