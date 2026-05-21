"""Provide general utility functions used across `integration-tests`."""

import os
import shutil
from pathlib import Path
from typing import Any

import yaml


def clear_directory(directory: Path) -> None:
    """
    Removes the contents of `directory` without removing `directory` itself.

    :param directory:
    """
    if not directory.exists():
        return

    for item in directory.iterdir():
        remove_path(item)


def get_binary_path(name: str) -> str:
    """
    :param name: Name of the program binary to locate.
    :return: Absolute path to the program binary.
    :raise RuntimeError: if the program binary is not found on $PATH.
    """
    program_binary = shutil.which(name)
    if program_binary is None:
        err_msg = f"'{name}' not found in PATH"
        raise RuntimeError(err_msg)
    return program_binary


def get_env_var(var_name: str) -> str:
    """
    :param var_name:
    :return: The string value of the specified environment variable.
    :raise: ValueError if the environment variable is not set
    """
    value = os.environ.get(var_name)
    if value is None:
        err_msg = f"Environment variable {var_name} is not set."
        raise ValueError(err_msg)
    return value


def load_yaml_to_dict(path: Path) -> dict[str, Any]:
    """
    Parses a YAML file into a dictionary.

    :param path:
    :return: Dictionary parsed from the file.
    :raise ValueError: if the file contains invalid YAML.
    :raise ValueError: if the file cannot be read.
    :raise TypeError: if the file does not have a top-level mapping.
    """
    try:
        with path.open("r", encoding="utf-8") as file:
            target_dict = yaml.safe_load(file)
    except yaml.YAMLError as err:
        err_msg = f"Invalid YAML in target file '{path}'"
        raise ValueError(err_msg) from err
    except OSError as err:
        err_msg = f"Cannot read target file '{path}'"
        raise ValueError(err_msg) from err

    if not isinstance(target_dict, dict):
        err_msg = f"Target file '{path}' must have a top-level mapping."
        raise TypeError(err_msg)

    return target_dict


def remove_path(path_to_remove: Path) -> None:
    """
    Remove a file, directory, or symlink at `path_to_remove` if it exists.

    :param path_to_remove:
    :raise: Propagates `pathlib.Path.unlink`'s exceptions.
    :raise: Propagates `shutil.rmtree`'s exceptions.
    """
    if path_to_remove.is_symlink():
        path_to_remove.unlink()
    elif not path_to_remove.exists():
        return
    elif path_to_remove.is_dir():
        shutil.rmtree(path_to_remove)
    else:
        path_to_remove.unlink()


def resolve_path_env_var(var_name: str) -> Path:
    """
    :param var_name: Name of the environment variable holding a path.
    :return: Absolute Path resolved from the input variable.
    :raise: Propagates `get_env_var`'s exceptions.
    :raise: Propagates `Path.expanduser`'s exceptions.
    :raise: Propagates `Path.resolve`'s exceptions.
    """
    return Path(get_env_var(var_name)).expanduser().resolve()


def validate_dir_exists(dir_path: Path) -> None:
    """
    :param dir_path:
    :raise: ValueError if the path does not exist or is not a directory.
    """
    if not dir_path.is_dir():
        err_msg = f"Path does not exist or is not a directory: {dir_path}"
        raise ValueError(err_msg)


def validate_file_exists(file_path: Path) -> None:
    """
    :param file_path:
    :raise ValueError: if the path does not exist or is not a file.
    """
    if not file_path.is_file():
        err_msg = f"Path does not exist or is not a file: {file_path}"
        raise ValueError(err_msg)
