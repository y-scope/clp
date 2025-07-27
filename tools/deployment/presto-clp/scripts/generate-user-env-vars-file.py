import argparse
import logging
import sys
from pathlib import Path
from typing import Optional

import yaml

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
        description="Generates an environment variables file for any user-configured properties."
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
    with open(clp_config_file_path, "r") as clp_config_file:
        clp_config = yaml.safe_load(clp_config_file)

    database_host = _get_config_value(clp_config, "database.host", "localhost")
    database_port = _get_config_value(clp_config, "database.port", 3306)
    database_name = _get_config_value(clp_config, "database.name", "clp-db")

    clp_archive_output_storage_type = _get_config_value(
        clp_config, "archive_output.storage.type", "fs"
    )
    if "fs" != clp_archive_output_storage_type:
        logger.error(
            "Expected CLP's archive_output.storage.type to be fs but found '%s'. Presto currently only supports"
            " reading archives from the fs storage type.",
            clp_archive_output_storage_type,
        )

    clp_archives_dir = _get_config_value(
        clp_config,
        "archive_output.storage.directory",
        str(clp_package_dir / "var" / "data" / "archives"),
    )

    credentials_file_path = clp_package_dir / "etc" / "credentials.yml"
    with open(credentials_file_path, "r") as credentials_file:
        credentials = yaml.safe_load(credentials_file)

    database_user = _get_config_value(credentials, "database.user")
    database_password = _get_config_value(credentials, "database.password")
    if not database_user or not database_password:
        logger.error(
            "database.user and database.password must be specified in '%s'.", credentials_file_path
        )
        return 1

    with open(output_file, "w") as env_file:
        env_file.write(
            "PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_URL"
            f"=jdbc:mysql://{database_host}:{database_port}\n"
        )
        env_file.write(
            f"PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_NAME={database_name}\n"
        )
        env_file.write(
            f"PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_USER={database_user}\n"
        )
        env_file.write(
            f"PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_DATABASE_PASSWORD={database_password}\n"
        )
        env_file.write(f"PRESTO_COORDINATOR_CONFIG_CLPPROPERTIES_METADATA_TABLEPREFIX=clp_\n")
        env_file.write(f"CLP_PACKAGE_ARCHIVES={clp_archives_dir}\n")

    return 0


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
