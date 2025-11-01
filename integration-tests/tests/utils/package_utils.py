"""Provides utility functions related to the clp-package used across `integration-tests`."""

import subprocess
from pathlib import Path
from typing import Any

import yaml
from clp_py_utils.clp_config import (
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
    PackageInstance,
    PackageInstanceConfig,
    PackageModeConfig,
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

CLP_MODES: dict[str, PackageModeConfig] = {
    "clp-text": PackageModeConfig(
        name="clp-text",
        storage_engine=StorageEngine.CLP,
        query_engine=QueryEngine.CLP,
    ),
    "clp-json": PackageModeConfig(
        name="clp-json",
        storage_engine=StorageEngine.CLP_S,
        query_engine=QueryEngine.CLP_S,
    ),
}


def start_clp_package(run_config: PackageInstanceConfig) -> None:
    """Starts an instance of the clp package."""
    start_script_path = run_config.package_config.start_script_path
    try:
        # fmt: off
        start_cmd = [
            str(start_script_path),
            "--config",
            str(run_config.temp_config_file_path)
        ]
        # fmt: on
        subprocess.run(start_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to start an instance of the {run_config.mode} package."
        raise RuntimeError(err_msg) from e


def stop_clp_package(
    instance: PackageInstance,
) -> None:
    """Stops an instance of the clp package."""
    run_config = instance.package_instance_config
    stop_script_path = run_config.package_config.stop_script_path
    try:
        # fmt: off
        stop_cmd = [
            stop_script_path
        ]
        # fmt: on
        subprocess.run(stop_cmd, check=True)
    except Exception as e:
        err_msg = f"Failed to start an instance of the {run_config.mode} package."
        raise RuntimeError(err_msg) from e


def write_temp_config_file(
    mode_kv_dict: dict[str, Any],
    temp_config_dir: Path,
    mode: str,
) -> Path:
    """
    Writes the set of key-value pairs in `mode_kv_dict` to a new yaml config file called
    "clp-config-`{mode}`.yml" located in `temp_config_dir`. Returns the path to the temp config
    file.
    """
    if not isinstance(mode_kv_dict, dict):
        err_msg = "`mode_kv_dict` must be a mapping."
        raise TypeError(err_msg)

    temp_config_dir.mkdir(parents=True, exist_ok=True)
    temp_config_filename = f"clp-config-{mode}.yml"
    temp_config_file_path = temp_config_dir / temp_config_filename

    tmp_path = temp_config_file_path.with_suffix(temp_config_file_path.suffix + ".tmp")
    with tmp_path.open("w", encoding="utf-8") as f:
        yaml.safe_dump(mode_kv_dict, f, sort_keys=False)
    tmp_path.replace(temp_config_file_path)

    return temp_config_file_path


def build_dict_from_config(mode_config: PackageModeConfig) -> dict[str, Any]:
    """
    Return {"package": {...}} where the inner mapping is produced from clp_config.Package
    with proper enum serialization.
    """
    storage_engine = mode_config.storage_engine
    query_engine = mode_config.query_engine
    package_model = Package(storage_engine=storage_engine, query_engine=query_engine)
    return {"package": package_model.model_dump()}


def get_dict_from_mode(mode: str) -> dict[str, Any]:
    """Returns the dict that corresponds to the mode."""
    mode_config = CLP_MODES.get(mode)
    if mode_config is None:
        err_msg = f"Unsupported mode: {mode}"
        raise ValueError(err_msg)

    return build_dict_from_config(mode_config)


def get_mode_from_dict(dictionary: dict[str, Any]) -> str:
    """Returns the mode that corresponds to dict."""
    package_dict = dictionary.get("package")
    if not isinstance(package_dict, dict):
        err_msg = "`dictionary` does not carry any mapping for 'package'."
        raise TypeError(err_msg)

    dict_query_engine = package_dict.get("query_engine")
    dict_storage_engine = package_dict.get("storage_engine")
    if dict_query_engine is None or dict_storage_engine is None:
        err_msg = (
            "`dictionary` must specify both 'package.query_engine' and 'package.storage_engine'."
        )
        raise ValueError(err_msg)

    for mode_name, mode_config in CLP_MODES.items():
        if str(mode_config.query_engine.value) == str(dict_query_engine) and str(
            mode_config.storage_engine.value
        ) == str(dict_storage_engine):
            return mode_name

    err_msg = (
        "The set of kv-pairs described in `dictionary` does not correspond to any mode of operation"
        " for which integration testing is supported."
    )
    raise ValueError(err_msg)
