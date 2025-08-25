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
    if not _add_clp_env_vars(clp_config, clp_package_dir, env_vars):
        return 1

    script_dir = Path(__file__).parent.resolve()
    if not _add_worker_env_vars(script_dir.parent / "coordinator-common.env", env_vars):
        return 1

    if not _generate_worker_clp_properties(script_dir.parent / "worker" / "config-template", env_vars):
        return 1

    with open(output_file, "w") as output_file_handle:
        for key, value in env_vars.items():
            output_file_handle.write(f"{key}={value}\n")

    return 0


def _add_clp_env_vars(
    clp_config: Dict[str, Any], clp_package_dir: Path, env_vars: Dict[str, str]
) -> bool:
    """
    Adds environment variables for CLP config values to `env_vars`.

    :param clp_config:
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
    database_port = _get_config_value(clp_config, "database.port", str(3306))
    database_name = _get_config_value(clp_config, "database.name", "clp-db")
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_URL"] = (
        f"jdbc:mysql://{database_host}:{database_port}"
    )
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_NAME"] = database_name

    clp_archive_output_storage_type = _get_config_value(
        clp_config, "archive_output.storage.type", "fs"
    )
    env_vars["CLP_ARCHIVES_DIR"] = _resolve_archives_dir(
        clp_package_dir,
        _get_config_value(clp_config, "archive_output.storage.directory"),
        clp_package_dir
        / "var"
        / "data"
        / ("archives" if clp_archive_output_storage_type == "fs" else "staged-archives"),
    )
    if "fs" == clp_archive_output_storage_type:
        pass
    elif "s3" == clp_archive_output_storage_type:
        s3_config_key_prefix = f"archive_output.storage.s3_config"
        s3_credentials_key_prefix = f"{s3_config_key_prefix}.aws_authentication.credentials"

        s3_access_key_id = _get_config_value(
            clp_config, f"{s3_credentials_key_prefix}.access_key_id"
        )

        s3_bucket = _get_config_value(clp_config, f"{s3_config_key_prefix}.bucket")
        s3_region_code = _get_config_value(clp_config, f"{s3_config_key_prefix}.region_code")

        s3_secret_access_key = _get_config_value(
            clp_config, f"{s3_credentials_key_prefix}.secret_access_key"
        )

        # Validate required S3 fields
        missing = []

        for k, v in {
            f"{s3_credentials_key_prefix}.access_key_id": s3_access_key_id,
            f"{s3_credentials_key_prefix}.secret_access_key": s3_secret_access_key,
            f"{s3_config_key_prefix}.bucket": s3_bucket,
            f"{s3_config_key_prefix}.region_code": s3_region_code,
        }.items():
            if not v:
                missing.append(k)

        if missing:
            logger.error("Missing required S3 config key(s): %s", ", ".join(missing))
            return False

        env_vars["PRESTO_WORKER_CLPPROPERTIES_STORAGE_TYPE"] = "s3"
        env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_AUTH_PROVIDER"] = "clp_package"
        env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_ACCESS_KEY_ID"] = s3_access_key_id
        s3_end_point = f"https://{s3_bucket}.s3.{s3_region_code}.amazonaws.com/"
        env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_END_POINT"] = s3_end_point
        env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_SECRET_ACCESS_KEY"] = s3_secret_access_key
    else:
        logger.error(
            "Expected CLP's archive_output.storage.type to be fs or s3 but found '%s'. Presto"
            " currently only supports reading archives from the fs or s3 storage type.",
            clp_archive_output_storage_type,
        )
        return False

    credentials_file_path = clp_package_dir / "etc" / "credentials.yml"
    if not credentials_file_path.exists():
        logger.error("'%s' doesn't exist. Did you start CLP?", credentials_file_path)
        return False

    with open(credentials_file_path, "r") as credentials_file:
        credentials = yaml.safe_load(credentials_file)

    database_user = _get_config_value(credentials, "database.user")
    database_password = _get_config_value(credentials, "database.password")
    if not database_user or not database_password:
        logger.error(
            "database.user and database.password must be specified in '%s'.", credentials_file_path
        )
        return False
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_USER"] = database_user
    env_vars["PRESTO_COORDINATOR_CLPPROPERTIES_METADATA_DATABASE_PASSWORD"] = database_password

    return True


def _resolve_archives_dir(base_dir: Path, configured: Optional[str], default: Path) -> str:
    effective = configured or str(default)
    return effective if Path(effective).is_absolute() else str(base_dir / effective)


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


def _generate_worker_clp_properties(worker_config_template_path: Path, env_vars: Dict[str, str]) -> bool:
    """
    Generates a clp.properties for worker config.

    :param worker_config_template_path:
    """
    clp_properties_path = worker_config_template_path / "clp.properties"
    config_options = ["connector.name=clp"]
    if "s3" == env_vars["PRESTO_WORKER_CLPPROPERTIES_STORAGE_TYPE"]:
        config_options.append("clp.storage-type=s3")
        config_options.append(f'clp.s3-auth-provider={env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_AUTH_PROVIDER"]}')
        config_options.append(f'clp.s3-access-key-id={env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_ACCESS_KEY_ID"]}')
        config_options.append(f'clp.s3-end-point={env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_END_POINT"]}')
        config_options.append(f'clp.s3-secret-access-key={env_vars["PRESTO_WORKER_CLPPROPERTIES_S3_SECRET_ACCESS_KEY"]}')
    with clp_properties_path.open("w", encoding="utf-8") as f:
        f.write("\n".join(config_options) + "\n")


def _get_config_value(config: dict, key: str, default_value: Optional[str] = None) -> str:
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
