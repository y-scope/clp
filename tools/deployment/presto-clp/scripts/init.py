import argparse
import logging
import sys
from pathlib import Path
from typing import Any, Dict, Optional

import yaml
from dotenv import dotenv_values

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


def main(argv=None) -> int:
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

    clp_config_file_path = clp_package_dir / "etc" / "clp-config.yml"
    if not clp_config_file_path.exists():
        logger.error(
            "'%s' doesn't exist. Is '%s' the location of the CLP package?",
            clp_config_file_path,
            clp_package_dir.resolve(),
        )
        return 1

    with open(clp_config_file_path, "r") as clp_config_file:
        clp_config = yaml.safe_load(clp_config_file)

    env_vars: Dict[str, str] = {}
    if not _add_clp_env_vars(clp_config, clp_config_file_path, clp_package_dir, env_vars):
        return 1

    script_dir = Path(__file__).parent.resolve()
    if not _add_worker_env_vars(script_dir.parent / "coordinator-common.env", env_vars):
        return 1

    if not _generate_worker_clp_properties(
        script_dir.parent / "worker" / "config-template", env_vars
    ):
        return 1

    with open(output_file, "w") as output_file_handle:
        for key, value in env_vars.items():
            output_file_handle.write(f"{key}={value}\n")

    return 0


def _add_clp_env_vars(
    clp_config: Dict[str, Any],
    clp_config_file_path: Path,
    clp_package_dir: Path,
    env_vars: Dict[str, str],
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

    database_type = _get_config_value(clp_config, "database.type", "mariadb")
    if "mariadb" != database_type and "mysql" != database_type:
        logger.error(
            "CLP's database.type must be either mariadb or mysql but found '%s'. Presto"
            " currently only supports reading metadata from a mariadb or mysql database.",
            database_type,
        )
        return False

    database_host = _get_config_value(clp_config, "database.host", "localhost")
    database_port = _get_config_value(clp_config, "database.port", 3306)
    database_name = _get_config_value(clp_config, "database.name", "clp-db")
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_URL"] = (
        f"jdbc:mysql://{database_host}:{database_port}"
    )
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_NAME"] = database_name

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
    elif "s3" == clp_archive_output_storage_type:
        env_vars["CLP_ARCHIVES_DIR"] = str(
            _get_path_clp_config_value(
                clp_config,
                f"{archive_output_storage_key}.staging_directory",
                Path("var") / "data" / "staged-archives",
                clp_package_dir,
            )
        )

        if not _add_clp_s3_env_vars(clp_config, clp_config_file_path, env_vars):
            return False
    else:
        logger.error(
            "'%s' for %s is unsupported.",
            clp_archive_output_storage_type,
            archive_output_storage_key,
        )
        return False

    credentials_file_path = clp_package_dir / "etc" / "credentials.yml"
    if not credentials_file_path.exists():
        logger.error("'%s' doesn't exist. Did you start CLP?", credentials_file_path)
        return False

    with open(credentials_file_path, "r") as credentials_file:
        credentials = yaml.safe_load(credentials_file)

    try:
        database_user = _get_required_config_value(
            credentials, "database.user", credentials_file_path
        )
        database_password = _get_required_config_value(
            credentials, "database.password", credentials_file_path
        )
    except KeyError:
        return False
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_USER"] = database_user
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_PASSWORD"] = database_password

    return True


def _add_clp_s3_env_vars(
    clp_config: Dict[str, Any], clp_config_file_path: Path, env_vars: Dict[str, str]
) -> bool:
    """
    Adds environment variables for CLP S3 config values to `env_vars`.

    :param clp_config:
    :param clp_config_file_path: Path to the file containing `clp_config`, for logging.
    :param env_vars:
    :return: Whether the environment variables were successfully added.
    """
    try:
        s3_config_key = f"archive_output.storage.s3_config"
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


def _add_worker_env_vars(coordinator_common_env_file_path: Path, env_vars: Dict[str, str]) -> bool:
    """
    Adds environment variables for worker config values to `env_vars`.

    :param coordinator_common_env_file_path:
    :param env_vars:
    :return: Whether the environment variables were successfully added.
    """
    config = dotenv_values(coordinator_common_env_file_path)

    try:
        env_vars["PRESTO_COORDINATOR_CONFIGPROPERTIES_DISCOVERY_URI"] = (
            f'http://{config["PRESTO_COORDINATOR_SERVICENAME"]}'
            f':{config["PRESTO_COORDINATOR_HTTPPORT"]}'
        )
    except KeyError as e:
        logger.error(
            "Missing required key '%s' in '%s'",
            e,
            coordinator_common_env_file_path,
        )
        return False

    return True


def _generate_worker_clp_properties(
    worker_config_template_path: Path, env_vars: Dict[str, str]
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

    with open(worker_config_template_path / "clp.properties", "w", encoding="utf-8") as f:
        f.write("\n".join(properties) + "\n")

    return True


def _get_path_clp_config_value(
    clp_config: Dict[str, Any], key: str, default_value: Path, clp_package_dir: Path
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
    else:
        return clp_package_dir / value_as_path


def _get_required_config_value(config: Dict[str, Any], key: str, config_file_path: Path) -> str:
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
    config: Dict[str, Any], key: str, default_value: Optional[Any] = None
) -> Optional[Any]:
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
