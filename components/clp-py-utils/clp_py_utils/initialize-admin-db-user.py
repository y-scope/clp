#!/usr/bin/env python3
import argparse
import logging
import pathlib
import sys
from contextlib import closing

from pydantic import ValidationError

from clp_py_utils.clp_config import (
    CLP_METADATA_TABLE_PREFIX,
    ClpConfig,
    ClpDbUserType,
)
from clp_py_utils.clp_metadata_db_utils import get_aws_credentials_table_name
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_adapter import SqlAdapter

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
        clp_config.database.load_credentials_from_env(ClpDbUserType.ROOT)
        clp_config.database.load_credentials_from_env(ClpDbUserType.ADMIN)
        clp_config.database.load_credentials_from_env(ClpDbUserType.CLP)
    except (ValidationError, ValueError) as err:
        logger.error(err)
        return -1
    except Exception:
        logger.exception("Failed to load CLP configuration.")
        return -1

    try:
        sql_adapter = SqlAdapter(clp_config.database)
        clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(
            True, ClpDbUserType.ROOT
        )
        database_name = clp_db_connection_params["name"]
        table_prefix = clp_db_connection_params.get("table_prefix", CLP_METADATA_TABLE_PREFIX)

        admin_username = clp_config.database.credentials[ClpDbUserType.ADMIN].username
        admin_password = clp_config.database.credentials[ClpDbUserType.ADMIN].password
        escaped_admin_username = admin_username.replace("'", "''")
        escaped_admin_password = admin_password.replace("'", "''")

        credentials_table = get_aws_credentials_table_name(table_prefix)
        with (
            closing(sql_adapter.create_connection(True, ClpDbUserType.ROOT)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            logger.info(f"Creating admin database user '{admin_username}'...")

            cursor.execute(
                f"CREATE USER IF NOT EXISTS '{escaped_admin_username}'@'%'"
                f" IDENTIFIED BY '{escaped_admin_password}'"
            )
            cursor.execute(
                f"ALTER USER '{escaped_admin_username}'@'%' IDENTIFIED BY '{escaped_admin_password}'"
            )

            logger.info(
                f"Granting privileges on credentials tables to admin user '{admin_username}'."
            )

            cursor.execute(
                f"GRANT SELECT, INSERT, UPDATE, DELETE ON `{database_name}`.`{credentials_table}`"
                f" TO '{escaped_admin_username}'@'%'"
            )
            cursor.execute("FLUSH PRIVILEGES")
            db_conn.commit()

            logger.info(
                "Admin database user '%s' created and permissions applied successfully.",
                admin_username,
            )

    except Exception:
        logger.exception("Failed to create admin database user.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
