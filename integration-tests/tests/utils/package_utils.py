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


CLP_COMPONENT_BASENAMES = [
    _to_container_basename(DB_COMPONENT_NAME),
    _to_container_basename(QUEUE_COMPONENT_NAME),
    _to_container_basename(REDIS_COMPONENT_NAME),
    _to_container_basename(RESULTS_CACHE_COMPONENT_NAME),
    _to_container_basename(COMPRESSION_SCHEDULER_COMPONENT_NAME),
    _to_container_basename(QUERY_SCHEDULER_COMPONENT_NAME),
    _to_container_basename(COMPRESSION_WORKER_COMPONENT_NAME),
    _to_container_basename(QUERY_WORKER_COMPONENT_NAME),
    _to_container_basename(REDUCER_COMPONENT_NAME),
    _to_container_basename(WEBUI_COMPONENT_NAME),
    _to_container_basename(GARBAGE_COLLECTOR_COMPONENT_NAME),
]

CLP_MODE_CONFIGS: dict[str, Callable[[], CLPConfig]] = {
    "clp-text": lambda: CLPConfig(
        package=Package(
            storage_engine=StorageEngine.CLP,
            query_engine=QueryEngine.CLP,
        ),
    ),
    "clp-json": lambda: CLPConfig(
        package=Package(
            storage_engine=StorageEngine.CLP_S,
            query_engine=QueryEngine.CLP_S,
        ),
    ),
}


def get_dict_from_mode_name(mode_name: str) -> dict[str, Any]:
    """Returns the dictionary that describes the operation of `mode_name`."""
    mode_config = CLP_MODE_CONFIGS.get(mode_name)
    if mode_config is None:
        err_msg = f"Unsupported mode: {mode_name}"
        raise ValueError(err_msg)

    clp_config = mode_config()
    ret_dict: dict[str, Any] = clp_config.dump_to_primitive_dict()
    return ret_dict


def get_mode_name_from_dict(dictionary: dict[str, Any]) -> str:
    """Returns the mode name for a parsed CLPConfig."""
    try:
        cfg = CLPConfig.model_validate(dictionary)
    except Exception as err:
        err_msg = f"Shared config failed validation: {err}"
        raise ValueError(err_msg) from err

    key = (cfg.package.storage_engine, cfg.package.query_engine)
    mode_lookup: dict[tuple[StorageEngine, QueryEngine], str] = {
        (StorageEngine.CLP, QueryEngine.CLP): "clp-text",
        (StorageEngine.CLP_S, QueryEngine.CLP_S): "clp-json",
    }
    try:
        return mode_lookup[key]
    except KeyError:
        err_msg = f"Unsupported storage/query engine pair: {key[0].value}, {key[1].value}"
        raise ValueError(err_msg) from None


def write_temp_config_file(
    mode_kv_dict: dict[str, Any],
    temp_config_dir: Path,
    mode_name: str,
) -> Path:
    """
    Writes a temporary config file to `temp_config_dir`. Returns the path to the temporary file on
    success.
    """
    if not isinstance(mode_kv_dict, dict):
        err_msg = "`mode_kv_dict` must be a mapping."
        raise TypeError(err_msg)

    temp_config_dir.mkdir(parents=True, exist_ok=True)
    temp_config_filename = f"clp-config-{mode_name}.yml"
    temp_config_file_path = temp_config_dir / temp_config_filename

    tmp_path = temp_config_file_path.with_suffix(temp_config_file_path.suffix + ".tmp")
    with tmp_path.open("w", encoding="utf-8") as f:
        yaml.safe_dump(mode_kv_dict, f, sort_keys=False)
    tmp_path.replace(temp_config_file_path)

    return temp_config_file_path


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


def start_clp_package(cfg: PackageConfig) -> None:
    """Start an instance of the CLP package."""
    start_script_path = cfg.start_script_path
    try:
        # fmt: off
        start_cmd = [
            str(start_script_path),
            "--config",
            str(cfg.temp_config_file_path)
        ]
        # fmt: on
        subprocess.run(start_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to start an instance of the {cfg.mode_name} package."
        raise RuntimeError(err_msg) from e


def stop_clp_package(instance: PackageInstance) -> None:
    """Stop an instance of the CLP package."""
    cfg = instance.package_config
    stop_script_path = cfg.stop_script_path
    try:
        # fmt: off
        stop_cmd = [
            stop_script_path
        ]
        # fmt: on
        subprocess.run(stop_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to stop an instance of the {cfg.mode_name} package."
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

    for component in CLP_COMPONENT_BASENAMES:
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


def is_running_mode_correct(package_instance: PackageInstance) -> tuple[bool, str | None]:
    """
    Checks if the mode described in the shared config file matches the mode described in
    `mode_config` of `package_instance`. Returns `True` if correct, `False` with message on
    mismatch.
    """
    running_mode = _get_running_mode(package_instance)
    intended_mode = package_instance.package_config.mode_name
    if running_mode != intended_mode:
        return (
            False,
            f"Mode mismatch: the package is running in {running_mode}, but it should be running in"
            f" {intended_mode}.",
        )

    return True, None


def _get_running_mode(package_instance: PackageInstance) -> str:
    shared_config_dict = _load_shared_config(package_instance.shared_config_file_path)
    return get_mode_name_from_dict(shared_config_dict)
