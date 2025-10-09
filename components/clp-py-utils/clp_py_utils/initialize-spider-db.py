#!/usr/bin/env python3
import argparse
import logging
import pathlib
import re
import sys
from contextlib import closing

from pydantic import ValidationError
from sql_adapter import SQL_Adapter

from clp_py_utils.clp_config import CLPConfig
from clp_py_utils.core import read_yaml_config_file

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    args_parser = argparse.ArgumentParser(description="Sets up Spider database.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    parsed_args = args_parser.parse_args(argv[1:])

    config_path = pathlib.Path(parsed_args.config)
    try:
        clp_config = CLPConfig.model_validate(read_yaml_config_file(config_path))
        clp_config.database.load_credentials_from_env()
        if clp_config.spider_db is None:
            return 0
        clp_config.spider_db.load_credentials_from_env()
    except (ValidationError, ValueError) as err:
        logger.error(err)
        return -1
    except:
        logger.exception("Failed to load CLP configuration.")
        return -1

    spider_db_config = clp_config.spider_db
    if not spider_db_config:
        logger.error("Spider database configuration not found in CLP configuration.")
        return -1

    try:
        sql_adapter = SQL_Adapter(clp_config.database)
        with closing(sql_adapter.create_root_mariadb_connection()) as db_conn, closing(
            db_conn.cursor()
        ) as db_cursor:
            db_name = spider_db_config.name
            db_user = spider_db_config.username
            db_password = spider_db_config.password
            clp_user = clp_config.database.username
            if not _validate_name(db_name):
                logger.error(f"Invalid database name: {db_name}")
                return -1
            if not _validate_name(db_user):
                logger.error(f"Invalid database user name: {db_user}")
                return -1
            if not _validate_name(clp_user):
                logger.error(f"Invalid CLP database user name: {clp_user}")
                return -1
            if not _validate_name(db_password):
                logger.error(f"Invalid database user password")
                return -1

            db_cursor.execute(f"""CREATE DATABASE IF NOT EXISTS `{db_name}`""")
            if db_password is not None:
                db_cursor.execute(
                    f"""CREATE USER IF NOT EXISTS '{db_user}'@'%' IDENTIFIED BY '{db_password}'"""
                )
            else:
                db_cursor.execute(f"""CREATE USER IF NOT EXISTS '{db_user}'@'%' IDENTIFIED BY ''""")
            db_cursor.execute(f"""GRANT ALL PRIVILEGES ON `{db_name}`.* TO '{db_user}'@'%'""")
            db_cursor.execute(f"""GRANT ALL PRIVILEGES ON `{db_name}`.* TO '{clp_user}'@'%'""")

    except:
        logger.exception("Failed to setup Spider database.")
        return -1

    return 0


_name_pattern = re.compile(r"^[A-Za-z0-9_-]+$")


def _validate_name(name: str) -> bool:
    """
    Validates that the input string contains only alphanumeric characters, underscores, or hyphens.
    :param name: The input string to validate.
    :return: If the input string is valid.
    """
    return _name_pattern.match(name) is not None


if "__main__" == __name__:
    sys.exit(main(sys.argv))
