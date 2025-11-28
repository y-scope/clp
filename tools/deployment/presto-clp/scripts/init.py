#!/usr/bin/env python3
"""Initializes Presto worker configuration based on CLP package settings."""

import argparse
import logging
import sys
from pathlib import Path
from typing import Any

import psutil
import yaml
from dotenv import dotenv_values

# Database endpoint inside the CLP Package Docker network. Must match the constants defined in
# `components/clp-py-utils/clp_py_utils/clp_config.py`.
DATABASE_COMPONENT_NAME = "database"
DATABASE_DEFAULT_PORT = 3306

# Presto worker memory configuration ratios
# Based on: https://prestodb.io/docs/current/presto_cpp/properties.html
PRESTO_QUERY_MEMORY_RATIO = 0.5
PRESTO_SYSTEM_MEMORY_RATIO = 0.9

# Set up console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter(
    "%(asctime)s.%(msecs)03d %(levelname)s [%(module)s] %(message)s", datefmt="%Y-%m-%dT%H:%M:%S"
)
logging_console_handler.setFormatter(logging_formatter)

# Set up root logger
root_logger = logging.getLogger()
root_logger.setLevel(logging.INFO)
root_logger.addHandler(logging_console_handler)

# Create logger
logger = logging.getLogger(__name__)


def main(argv: list[str] | None = None) -> int:
    """Initializes Presto worker configuration based on CLP package settings."""
    if argv is None:
        argv = sys.argv

    args_parser = argparse.ArgumentParser(
        description=(
            "Generates any necessary config files corresponding to user-configured properties."
        )
    )
    args_parser.add_argument(
        "--clp-package-dir", help="CLP package directory.", required=True, type=Path
    )
    args_parser.add_argument(
        "--output-file", help="Path for the environment variables file.", required=True, type=Path
    )

    parsed_args = args_parser.parse_args(argv[1:])
    clp_package_dir: Path = parsed_args.clp_package_dir.resolve()
    output_file: Path = parsed_args.output_file

    clp_config_file_path = clp_package_dir / "etc" / "clp-config.yaml"
    if not clp_config_file_path.exists():
        logger.error(
            "'%s' doesn't exist. Is '%s' the location of the CLP package?",
            clp_config_file_path,
            clp_package_dir.resolve(),
        )
        return 1

    with clp_config_file_path.open("r") as clp_config_file:
        clp_config = yaml.load(clp_config_file, Loader=yaml.CSafeLoader)

    env_vars: dict[str, str] = {}
    if not _add_clp_env_vars(clp_config, clp_config_file_path, clp_package_dir, env_vars):
        return 1

    if not _add_memory_env_vars(env_vars):
        return 1

    script_dir = Path(__file__).parent.resolve()
    if not _add_worker_env_vars(script_dir.parent / "coordinator-common.env", env_vars):
        return 1

    if not _generate_worker_clp_properties(
        script_dir.parent / "worker" / "config-template", env_vars
    ):
        return 1

    with output_file.open("w") as output_file_handle:
        output_file_handle.writelines(f"{key}={value}\n" for key, value in env_vars.items())

    return 0


def _add_clp_env_vars(
    clp_config: dict[str, Any],
    clp_config_file_path: Path,
    clp_package_dir: Path,
    env_vars: dict[str, str],
) -> bool:
    """
    Adds environment variables for CLP config values to `env_vars`.

    :param clp_config:
    :param clp_config_file_path: Path to the file containing `clp_config`, for logging.
    :param clp_package_dir:
    :param env_vars:
    :return: Whether the environment variables were successfully added.
    """
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_TABLE_PREFIX"] = "clp_"

    if not _add_db_env_vars(clp_config, env_vars):
        return False

    if not _add_archive_env_vars(clp_config, clp_config_file_path, clp_package_dir, env_vars):
        return False

    if not _add_credentials_env_vars(clp_package_dir, env_vars):
        return False

    return _add_instance_id_env_var(clp_config, clp_package_dir, env_vars)


def _add_db_env_vars(clp_config: dict[str, Any], env_vars: dict[str, str]) -> bool:
    """
    Adds environment variables for CLP database config values to `env_vars`.

    :param clp_config:
    :param env_vars:
    :return: Whether the environment variables were successfully added.
    """
    database_type = _get_config_value(clp_config, "database.type", "mariadb")
    if database_type not in {"mariadb", "mysql"}:
        logger.error(
            "CLP's database.type must be either mariadb or mysql but found '%s'. Presto"
            " currently only supports reading metadata from a mariadb or mysql database.",
            database_type,
        )
        return False

    database_name = _get_config_value(clp_config, "database.name", "clp-db")
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_URL"] = (
        f"jdbc:mysql://{DATABASE_COMPONENT_NAME}:{DATABASE_DEFAULT_PORT}"
    )
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_NAME"] = database_name

    return True


def _add_archive_env_vars(
    clp_config: dict[str, Any],
    clp_config_file_path: Path,
    clp_package_dir: Path,
    env_vars: dict[str, str],
) -> bool:
    """
    Adds environment variables for CLP archive storage config values to `env_vars`.

    :param clp_config:
    :param clp_config_file_path: Path to the file containing `clp_config`, for logging.
    :param clp_package_dir:
    :param env_vars:
    :return: Whether the environment variables were successfully added.
    """
    clp_archive_output_storage_type = _get_config_value(
        clp_config, "archive_output.storage.type", "fs"
    )
    archive_output_storage_key = "archive_output.storage"
    env_vars["PRESTO_WORKER_CLPPROPERTIES_STORAGE_TYPE"] = clp_archive_output_storage_type
    if "fs" == clp_archive_output_storage_type:
        env_vars["CLP_ARCHIVES_DIR"] = str(
            _get_path_clp_config_value(
                clp_config,
                f"{archive_output_storage_key}.directory",
                Path("var") / "data" / "archives",
                clp_package_dir,
            )
        )
        return True
    if "s3" == clp_archive_output_storage_type:
        env_vars["CLP_STAGED_ARCHIVES_DIR"] = str(
            _get_path_clp_config_value(
                clp_config,
                f"{archive_output_storage_key}.staging_directory",
                Path("var") / "data" / "staged-archives",
                clp_package_dir,
            )
        )
        return _add_clp_s3_env_vars(clp_config, clp_config_file_path, env_vars)

    logger.error(
        "'%s' for %s is unsupported.",
        clp_archive_output_storage_type,
        archive_output_storage_key,
    )
    return False


def _add_credentials_env_vars(clp_package_dir: Path, env_vars: dict[str, str]) -> bool:
    """
    Adds environment variables for CLP database credentials to `env_vars`.

    :param clp_package_dir:
    :param env_vars:
    :return: Whether the environment variables were successfully added.
    """
    credentials_file_path = clp_package_dir / "etc" / "credentials.yaml"
    if not credentials_file_path.exists():
        logger.error("'%s' doesn't exist. Did you start CLP?", credentials_file_path)
        return False

    with credentials_file_path.open("r") as credentials_file:
        credentials = yaml.load(credentials_file, Loader=yaml.CSafeLoader)

    try:
        database_username = _get_required_config_value(
            credentials, "database.username", credentials_file_path
        )
        database_password = _get_required_config_value(
            credentials, "database.password", credentials_file_path
        )
    except KeyError:
        return False
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_USER"] = database_username
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_PASSWORD"] = database_password
    return True


def _add_instance_id_env_var(
    clp_config: dict[str, Any], clp_package_dir: Path, env_vars: dict[str, str]
) -> bool:
    """
    Adds environment variable for CLP package instance ID to `env_vars`.

    :param clp_config:
    :param clp_package_dir:
    :param env_vars:
    :return: Whether the environment variable was successfully added.
    """
    instance_id = _get_clp_package_instance_id(clp_config, clp_package_dir)
    if instance_id is None:
        return False
    env_vars["CLP_PACKAGE_NETWORK_NAME"] = f"clp-package-{instance_id}_default"
    return True


def _add_clp_s3_env_vars(
    clp_config: dict[str, Any], clp_config_file_path: Path, env_vars: dict[str, str]
) -> bool:
    """
    Adds environment variables for CLP S3 config values to `env_vars`.

    :param clp_config:
    :param clp_config_file_path: Path to the file containing `clp_config`, for logging.
    :param env_vars:
    :return: Whether the environment variables were successfully added.
    """
    try:
        s3_config_key = "archive_output.storage.s3_config"
        s3_bucket = _get_required_config_value(
            clp_config, f"{s3_config_key}.bucket", clp_config_file_path
        )
        s3_region_code = _get_required_config_value(
            clp_config, f"{s3_config_key}.region_code", clp_config_file_path
        )

        aws_auth_key = f"{s3_config_key}.aws_authentication"
        aws_auth_type_key = f"{aws_auth_key}.type"
        aws_auth_type = _get_required_config_value(
            clp_config, aws_auth_type_key, clp_config_file_path
        )
        credentials_auth_type_str = "credentials"
        if credentials_auth_type_str != aws_auth_type:
            logger.error("'%s' for %s is unsupported.", aws_auth_type, aws_auth_type_key)
            return False

        s3_credentials_key = f"{aws_auth_key}.{credentials_auth_type_str}"
        s3_access_key_id = _get_required_config_value(
            clp_config, f"{s3_credentials_key}.access_key_id", clp_config_file_path
        )
        s3_secret_access_key = _get_required_config_value(
            clp_config, f"{s3_credentials_key}.secret_access_key", clp_config_file_path
        )
    except KeyError:
        return False

    env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_AUTH_PROVIDER"] = "clp_package"
    env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_ACCESS_KEY_ID"] = s3_access_key_id
    s3_end_point = f"https://{s3_bucket}.s3.{s3_region_code}.amazonaws.com/"
    env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_END_POINT"] = s3_end_point
    env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_SECRET_ACCESS_KEY"] = s3_secret_access_key

    return True


def _add_memory_env_vars(env_vars: dict[str, str]) -> bool:
    """
    Adds memory-related environment variables based on Presto guidelines.

    :param env_vars: Dictionary to populate with environment variables.
    :return: Whether the environment variables were successfully added.
    """
    total_memory_gb = psutil.virtual_memory().total / (1024**3)

    query_memory_gb = max(1, int(total_memory_gb * PRESTO_QUERY_MEMORY_RATIO))
    system_memory_gb = max(query_memory_gb, int(total_memory_gb * PRESTO_SYSTEM_MEMORY_RATIO))

    logger.info(
        "Computed Presto worker memory settings from %.2f GB total RAM: "
        "query-memory-gb=%d, system-memory-gb=%d",
        total_memory_gb,
        query_memory_gb,
        system_memory_gb,
    )

    env_vars["PRESTO_WORKER_CONFIGPROPERTIES_QUERY_MEMORY_GB"] = str(query_memory_gb)
    env_vars["PRESTO_WORKER_CONFIGPROPERTIES_SYSTEM_MEMORY_GB"] = str(system_memory_gb)

    return True


def _add_worker_env_vars(coordinator_common_env_file_path: Path, env_vars: dict[str, str]) -> bool:
    """
    Adds environment variables for worker config values to `env_vars`.

    :param coordinator_common_env_file_path:
    :param env_vars:
    :return: Whether the environment variables were successfully added.
    """
    config = dotenv_values(coordinator_common_env_file_path)

    try:
        env_vars["PRESTO_COORDINATOR_CONFIGPROPERTIES_DISCOVERY_URI"] = (
            f"http://{config['PRESTO_COORDINATOR_SERVICENAME']}"
            f":{config['PRESTO_COORDINATOR_HTTPPORT']}"
        )
    except KeyError as e:
        logger.exception(
            "Missing required key in '%s'.", coordinator_common_env_file_path, exc_info=e
        )
        return False

    return True


def _generate_worker_clp_properties(
    worker_config_template_path: Path, env_vars: dict[str, str]
) -> bool:
    """
    Generates a clp.properties file for the worker.

    :param worker_config_template_path:
    :param env_vars:
    :return: Whether the clp.properties file was successfully generated.
    """
    properties = ["connector.name=clp"]
    if "s3" == env_vars.get("PRESTO_WORKER_CLPPROPERTIES_STORAGE_TYPE"):
        property_name_to_env_var_name = {
            "clp.storage-type": "PRESTO_WORKER_CLPPROPERTIES_STORAGE_TYPE",
            "clp.s3-auth-provider": "PRESTO_WORKER_CLPPROPERTIES_S3_AUTH_PROVIDER",
            "clp.s3-access-key-id": "PRESTO_WORKER_CLPPROPERTIES_S3_ACCESS_KEY_ID",
            "clp.s3-end-point": "PRESTO_WORKER_CLPPROPERTIES_S3_END_POINT",
            "clp.s3-secret-access-key": "PRESTO_WORKER_CLPPROPERTIES_S3_SECRET_ACCESS_KEY",
        }

        for property_name, env_var_name in property_name_to_env_var_name.items():
            env_var_value = env_vars.get(env_var_name)
            if not env_var_value:
                logger.error("Internal error: Missing required env var '%s'", env_var_name)
                return False
            properties.append(f"{property_name}={env_var_value}")

    with (worker_config_template_path / "clp.properties").open("w", encoding="utf-8") as f:
        f.write("\n".join(properties) + "\n")

    return True


def _get_clp_package_instance_id(clp_config: dict[str, Any], clp_package_dir: Path) -> str | None:
    """
    Retrieves the CLP package instance ID from the logs directory.

    :param clp_config:
    :param clp_package_dir:
    :return: The instance ID if it could be read, otherwise `None`.
    """
    logs_directory = _get_path_clp_config_value(
        clp_config, "logs_directory", Path("var") / "log", clp_package_dir
    )
    instance_id_path = logs_directory / "instance-id"
    if not instance_id_path.exists():
        logger.error(
            "Cannot determine the CLP package Docker network because '%s' does not exist."
            " Start the CLP package at least once before configuring Presto.",
            instance_id_path,
        )
        return None

    try:
        instance_id = instance_id_path.read_text(encoding="utf-8").strip()
    except OSError:
        logger.exception("Failed to read the CLP package instance ID from '%s'.", instance_id_path)
        return None

    if not instance_id:
        logger.error(
            "Instance ID file '%s' is empty. Restart the CLP package to regenerate the instance"
            " ID.",
            instance_id_path,
        )
        return None

    return instance_id


def _get_path_clp_config_value(
    clp_config: dict[str, Any], key: str, default_value: Path, clp_package_dir: Path
) -> Path:
    """
    Gets the value corresponding to `key` from `clp_config` as a `Path`.

    :param clp_config:
    :param key: The key to look for in the config, in dot notation (e.g., "database.host").
    :param default_value: Value to use if the key is missing.
    :param clp_package_dir: Base directory for resolving relative paths in `clp_config`.
    :return: The value as a `Path`; relative values resolve to `clp_package_dir` / <path>.
    """
    value = _get_config_value(clp_config, key, str(default_value))
    value_as_path = Path(value)
    if value_as_path.is_absolute():
        return value_as_path
    return clp_package_dir / value_as_path


def _get_required_config_value(config: dict[str, Any], key: str, config_file_path: Path) -> str:
    """
    Gets the value corresponding to `key` from `config`. Logs an error on failure.

    :param config: The config.
    :param key: The key to look for in the config, in dot notation (e.g., "database.host").
    :param config_file_path: The path to the config file, for logging.
    :return: The value corresponding to `key`.
    :raises KeyError: If `key` doesn't exist in `config` or its value is `None`.
    """
    value = _get_config_value(config, key)
    if value is None:
        logger.error("Required config '%s' is missing or null in '%s'.", key, config_file_path)
        raise KeyError(key)
    return value


def _get_config_value(
    config: dict[str, Any], key: str, default_value: Any | None = None
) -> Any | None:
    """
    Gets the value corresponding to `key` from `config` if it exists.

    :param config: The config.
    :param key: The key to look for in the config, in dot notation (e.g., "database.host").
    :param default_value: The value to return if `key` doesn't exist in `config`.
    :return: The value corresponding to `key` if it exists, otherwise `default_value`.
    """
    keys = key.split(".")
    value = config
    for k in keys:
        if isinstance(value, dict) and k in value:
            value = value[k]
        else:
            return default_value
    return value


if "__main__" == __name__:
    sys.exit(main(sys.argv))
