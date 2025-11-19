#!/usr/bin/env python3
import argparse
import logging
import pathlib
import sys
from contextlib import closing

from pydantic import ValidationError
from sql_adapter import SqlAdapter

from clp_py_utils.clp_config import (
    CLP_METADATA_TABLE_PREFIX,
    ClpConfig,
)
from clp_py_utils.clp_metadata_db_utils import get_aws_credentials_table_name
from clp_py_utils.core import read_yaml_config_file

logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    args_parser = argparse.ArgumentParser(
        description="Creates privileged database user for credential management."
    )
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    parsed_args = args_parser.parse_args(argv[1:])

    config_path = pathlib.Path(parsed_args.config)
    try:
        clp_config = ClpConfig.model_validate(read_yaml_config_file(config_path))
        clp_config.database.load_credentials_from_env()
    except (ValidationError, ValueError) as err:
        logger.error(err)
        return -1
    except Exception:
        logger.exception("Failed to load CLP configuration.")
        return -1

    if not clp_config.database.has_privileged_credentials():
        logger.error(
            "Privileged database credentials not configured."
            " Please set CLP_DB_PRIVILEGED_USER and CLP_DB_PRIVILEGED_PASS."
        )
        return -1

    admin_db_config = clp_config.database.model_copy()
    admin_db_config.username = "root"

    try:
        sql_adapter = SqlAdapter(admin_db_config)
        clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
        database_name = clp_db_connection_params["name"]
        table_prefix = clp_db_connection_params.get("table_prefix", CLP_METADATA_TABLE_PREFIX)

        privileged_username = clp_config.database.privileged_username
        privileged_password = clp_config.database.privileged_password
        regular_username = clp_config.database.username

        if privileged_username is None or privileged_password is None or regular_username is None:
            logger.error("Database credentials unexpectedly missing during privileged user setup.")
            return -1

        escaped_privileged_username = privileged_username.replace("'", "''")
        escaped_privileged_password = privileged_password.replace("'", "''")

        credentials_table = get_aws_credentials_table_name(table_prefix)
        with closing(sql_adapter.create_connection(True)) as db_conn, closing(
            db_conn.cursor(dictionary=True)
        ) as cursor:
            logger.info(f"Creating privileged database user '{privileged_username}'...")

            cursor.execute(
                f"CREATE USER IF NOT EXISTS '{escaped_privileged_username}'@'%'"
                f" IDENTIFIED BY '{escaped_privileged_password}'"
            )

            logger.info(f"Granting privileges on credential tables to '{privileged_username}'...")

            cursor.execute(
                f"GRANT SELECT, INSERT, UPDATE, DELETE ON `{database_name}`.`{credentials_table}`"
                f" TO '{escaped_privileged_username}'@'%'"
            )
            cursor.execute("FLUSH PRIVILEGES")
            db_conn.commit()

            logger.info("Privileged database user setup completed successfully.")

    except Exception:
        logger.exception("Failed to create privileged database user.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
