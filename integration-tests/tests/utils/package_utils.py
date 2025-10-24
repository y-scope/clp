"""Provide utility functions related to the clp-package used across `integration-tests`."""

import shutil
import subprocess
from collections.abc import Mapping
from pathlib import Path
from typing import Any

import yaml

from tests.utils.config import (
    PackageInstance,
    PackageInstanceConfigFile,
)

CLP_TEXT_KV_DICT = {"package": {"storage_engine": "clp", "query_engine": "clp"}}
CLP_JSON_KV_DICT = {"package": {"storage_engine": "clp-s", "query_engine": "clp-s"}}

CLP_COMPONENT_BASENAMES = [
    "database",
    "queue",
    "redis",
    "results-cache",
    "compression-scheduler",
    "query-scheduler",
    "compression-worker",
    "query-worker",
    "reducer",
    "webui",
    "garbage-collector",
]


def start_clp_package(run_config: PackageInstanceConfigFile) -> None:
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
    run_config = instance.package_instance_config_file
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
    merge_original: bool,
    original_config_file_path: Path,
) -> Path:
    """
    Writes the set of key-value pairs in `mode_kv_dict` to a new yaml config file located in
    `temp_config_dir`.

    If `merge_original` is `True`, merges the kv-pairs in `original_config_file_path` together with
    `mode_kv_dict` and places them all in the new config file. If a key is present in both
    `mode_kv_dict` and the file at `original_config_file_path`, the value given in the
    `original_config_file_path` file is overridden by the value given in `mode_kv_dict`.

    Returns the path to the temp config file.
    """
    if merge_original:
        # Incorporate the current contents of the original config file.
        if not original_config_file_path.is_file():
            err_msg = f"Source config file not found: {original_config_file_path}"
            raise FileNotFoundError(err_msg)

        with original_config_file_path.open("r", encoding="utf-8") as f:
            origin_data = yaml.safe_load(f) or {}

        if not isinstance(origin_data, dict):
            err_msg = "YAML root must be a mapping."
            raise TypeError(err_msg)

        origin_kv_dict: dict[str, Any] = origin_data
        union_kv_dict = make_union_kv_dict(origin_kv_dict, mode_kv_dict)
    else:
        # Write only the kv-pairs provided in mode_kv_dict.
        if not isinstance(mode_kv_dict, dict):
            err_msg = "`mode_kv_dict` must be a mapping."
            raise TypeError(err_msg)

        union_kv_dict = dict(mode_kv_dict)

    # Make the temp config directory if it doesn't already exist.
    temp_config_dir.mkdir(parents=True, exist_ok=True)

    # Use the same filename as the original.
    temp_config_filename = original_config_file_path.name
    temp_config_file_path = temp_config_dir / temp_config_filename

    # Write the merged content to the temp config file.
    tmp_path = temp_config_file_path.with_suffix(temp_config_file_path.suffix + ".tmp")
    with tmp_path.open("w", encoding="utf-8") as f:
        yaml.safe_dump(union_kv_dict, f, sort_keys=False)
    tmp_path.replace(temp_config_file_path)

    return temp_config_file_path


def make_union_kv_dict(
    base_dict: Mapping[str, Any],
    override_dict: Mapping[str, Any],
) -> dict[str, Any]:
    """
    Finds the union of the two argument Dicts. If a key exists in both, the value from
    `override_dict` takes precedence. Nested keys will remain untouched even if its parent is an
    overridden key.

    :param base_dict:
    :param override_dict:
    :return: The merged dictionary.
    """
    return deep_merge_with_override(base_dict, override_dict)


def deep_merge_with_override(
    base_dict: Mapping[str, Any],
    override_dict: Mapping[str, Any],
) -> dict[str, Any]:
    """
    Merges the two dicts.
    The values in `override_dict` take precedence in the case of identical keys.
    """
    merged_dict: dict[str, Any] = dict(base_dict)
    for key, override_value in override_dict.items():
        if (
            key in merged_dict
            and isinstance(merged_dict[key], Mapping)
            and isinstance(override_value, Mapping)
        ):
            merged_dict[key] = deep_merge_with_override(merged_dict[key], override_value)
        else:
            merged_dict[key] = override_value
    return merged_dict


def restore_config_file(
    config_file_path: Path,
    temp_cache_file_path: Path,
) -> bool:
    """
    Replaces the clp-config file at `config_file_path` with `temp_cache_file_path`. Deletes
    `temp_cache_file_path` after it is successfully restored.

    :param config_file_path:
    :param temp_cache_file_path:
    :return: `True` if the config file was restored successfully.
    """
    if not temp_cache_file_path.is_file():
        err_msg = f"Cached copy not found: {temp_cache_file_path}"
        raise FileNotFoundError(err_msg)

    if not config_file_path.is_file():
        err_msg = f"Modified config file not found: {config_file_path}"
        raise FileNotFoundError(err_msg)

    # Replace the config file with the cached copy.
    tmp_path = config_file_path.with_suffix(config_file_path.suffix + ".tmp")
    shutil.copyfile(temp_cache_file_path, tmp_path)
    tmp_path.replace(config_file_path)

    # Remove the cached copy.
    temp_cache_file_path.unlink()

    return True


def get_dict_from_mode(mode: str) -> dict[str, Any]:
    """Returns the dict that corresponds to the mode."""
    if mode == "clp-text":
        return CLP_TEXT_KV_DICT
    if mode == "clp-json":
        return CLP_JSON_KV_DICT
    err_msg = f"Unsupported mode: {mode}"
    raise ValueError(err_msg)
