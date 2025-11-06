"""Provides utility functions related to the clp-package used across `integration-tests`."""

import shutil
import subprocess
from collections.abc import Callable
from pathlib import Path
from typing import Any

import yaml
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
from pydantic import ValidationError

from tests.utils.config import (
    PackageConfig,
    PackageInstance,
)
from tests.utils.docker_utils import (
    inspect_container_state,
    list_prefixed_containers,
)


def _to_container_basename(name: str) -> str:
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
            _to_container_basename(DB_COMPONENT_NAME),
            _to_container_basename(QUEUE_COMPONENT_NAME),
            _to_container_basename(REDIS_COMPONENT_NAME),
            _to_container_basename(REDUCER_COMPONENT_NAME),
            _to_container_basename(RESULTS_CACHE_COMPONENT_NAME),
            _to_container_basename(COMPRESSION_SCHEDULER_COMPONENT_NAME),
            _to_container_basename(QUERY_SCHEDULER_COMPONENT_NAME),
            _to_container_basename(COMPRESSION_WORKER_COMPONENT_NAME),
            _to_container_basename(QUERY_WORKER_COMPONENT_NAME),
            _to_container_basename(WEBUI_COMPONENT_NAME),
            _to_container_basename(GARBAGE_COLLECTOR_COMPONENT_NAME),
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
            _to_container_basename(DB_COMPONENT_NAME),
            _to_container_basename(QUEUE_COMPONENT_NAME),
            _to_container_basename(REDIS_COMPONENT_NAME),
            _to_container_basename(REDUCER_COMPONENT_NAME),
            _to_container_basename(RESULTS_CACHE_COMPONENT_NAME),
            _to_container_basename(COMPRESSION_SCHEDULER_COMPONENT_NAME),
            _to_container_basename(QUERY_SCHEDULER_COMPONENT_NAME),
            _to_container_basename(COMPRESSION_WORKER_COMPONENT_NAME),
            _to_container_basename(QUERY_WORKER_COMPONENT_NAME),
            _to_container_basename(WEBUI_COMPONENT_NAME),
            _to_container_basename(GARBAGE_COLLECTOR_COMPONENT_NAME),
        ],
    ),
}


def get_clp_config_from_mode(mode_name: str) -> CLPConfig:
    """Return a CLPConfig object corresponding to the given `mode_name`."""
    try:
        config = CLP_MODE_CONFIGS[mode_name][0]
    except KeyError as err:
        err_msg = f"Unsupported mode: {mode_name}"
        raise ValueError(err_msg) from err
    return config()


def _load_shared_config(path: Path) -> dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as file:
            shared_config_dict = yaml.safe_load(file)
    except yaml.YAMLError as err:
        err_msg = f"Invalid YAML in shared config {path}: {err}"
        raise ValueError(err_msg) from err
    except OSError as err:
        err_msg = f"Cannot read shared config {path}: {err}"
        raise ValueError(err_msg) from err

    if not isinstance(shared_config_dict, dict):
        err_msg = f"Shared config {path} must be a mapping at the top level."
        raise TypeError(err_msg)

    return shared_config_dict


def write_temp_config_file(
    clp_config: CLPConfig,
    temp_config_dir: Path,
    mode_name: str,
) -> Path:
    """
    Writes a temporary config file to `temp_config_dir` for a CLPConfig object. Returns the path to
    the temporary file on success.
    """
    temp_config_dir.mkdir(parents=True, exist_ok=True)
    temp_config_filename = f"clp-config-{mode_name}.yml"
    temp_config_file_path = temp_config_dir / temp_config_filename

    payload: dict[str, Any] = clp_config.dump_to_primitive_dict()

    tmp_path = temp_config_file_path.with_suffix(temp_config_file_path.suffix + ".tmp")
    with tmp_path.open("w", encoding="utf-8") as f:
        yaml.safe_dump(payload, f, sort_keys=False)
    tmp_path.replace(temp_config_file_path)

    return temp_config_file_path


def start_clp_package(package_config: PackageConfig) -> None:
    """Start an instance of the CLP package."""
    start_script_path = package_config.start_script_path
    try:
        # fmt: off
        start_cmd = [
            str(start_script_path),
            "--config",
            str(package_config.temp_config_file_path)
        ]
        # fmt: on
        subprocess.run(start_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to start an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from e


def stop_clp_package(instance: PackageInstance) -> None:
    """Stop an instance of the CLP package."""
    package_config = instance.package_config
    stop_script_path = package_config.stop_script_path
    try:
        # fmt: off
        stop_cmd = [
            stop_script_path
        ]
        # fmt: on
        subprocess.run(stop_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to stop an instance of the {package_config.mode_name} package."
        raise RuntimeError(err_msg) from e


def is_package_running(package_instance: PackageInstance) -> tuple[bool, str | None]:
    """
    Checks that the `package_instance` is running properly by examining each of its component
    containers. Records which containers are not found or not running.
    """
    docker_bin = shutil.which("docker")
    if docker_bin is None:
        err_msg = "docker not found in PATH"
        raise RuntimeError(err_msg)

    instance_id = package_instance.clp_instance_id
    problems: list[str] = []

    required_components = package_instance.package_config.component_list
    for component in required_components:
        prefix = f"clp-package-{instance_id}-{component}-"

        candidates = list_prefixed_containers(docker_bin, prefix)
        if not candidates:
            problems.append(f"No component container was found with the prefix '{prefix}'")
            continue

        not_running: list[str] = []
        for name in candidates:
            if not inspect_container_state(docker_bin, name, "running"):
                not_running.append(name)

        if not_running:
            details = ", ".join(not_running)
            problems.append(f"Component containers not running: {details}")

    if problems:
        return False, "; ".join(problems)

    return True, None


def _compute_mode_signature(config: CLPConfig) -> tuple[Any, ...]:
    """Constructs a signature that captures the mode-defining aspects of a CLPConfig object."""
    return (
        config.logs_input.type,
        config.package.storage_engine.value,
        config.package.storage_engine.value,
        config.mcp_server is not None,
        config.presto is not None,
        config.archive_output.storage.type,
        config.stream_output.storage.type,
        config.aws_config_directory is not None,
        config.get_deployment_type(),
    )


def is_running_mode_correct(package_instance: PackageInstance) -> tuple[bool, str | None]:
    """
    Checks if the mode described in the shared config file of `package_instance` is accurate with
    respect to its `mode_name`. Returns `True` if correct, `False` with message on mismatch.
    """
    shared_config_dict = _load_shared_config(package_instance.shared_config_file_path)
    try:
        running_config = CLPConfig.model_validate(shared_config_dict)
    except ValidationError as err:
        err_msg = f"Shared config failed validation: {err}"
        raise ValueError(err_msg) from err

    intended_config = get_clp_config_from_mode(package_instance.package_config.mode_name)

    running_signature = _compute_mode_signature(running_config)
    intended_signature = _compute_mode_signature(intended_config)

    if running_signature != intended_signature:
        return (
            False,
            "Mode mismatch: running configuration does not match intended configuration.",
        )

    return True, None
