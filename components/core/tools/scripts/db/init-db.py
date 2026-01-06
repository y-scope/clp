#!/usr/bin/env python3
"""Initializes the global MySQL metadata database for CLP."""
import argparse
import logging
import os
import sys

import mariadb

# Setup logging
# Create logger
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv: list[str]) -> int:
    """
    Main.

    :param argv:
    :return: Exit code
    """
    args_parser = argparse.ArgumentParser(
        description="Setup a global MySQL metadata database for CLP."
    )
    args_parser.add_argument("--db-host", default="127.0.0.1", help="Database host")
    args_parser.add_argument("--db-port", type=int, default=3306, help="Database port")
    args_parser.add_argument("--db-name", default="clp-db", help="Database name")
    args_parser.add_argument("--db-table-prefix", default="clp_", help="Database table prefix")

    parsed_args = args_parser.parse_args(argv[1:])

    host = parsed_args.db_host
    port = parsed_args.db_port
    db_name = parsed_args.db_name
    table_prefix = parsed_args.db_table_prefix

    try:
        username = os.environ["CLP_DB_USER"]
        password = os.environ["CLP_DB_PASS"]
    except KeyError as e:
        msg = f"Environment variable {e} hasn't been set."
        raise OSError(msg) from e

    try:
        mysql_conn = mariadb.connect(host=host, port=port, username=username, password=password)
        mysql_cursor = mysql_conn.cursor()
    except mariadb.Error as err:
        logger.exception("Failed to connect - %s", err.msg)
        return -1

    try:
        # Create database
        try:
            mysql_cursor.execute(
                f"CREATE DATABASE IF NOT EXISTS `{db_name}` DEFAULT CHARACTER SET 'utf8'"
            )
        except mariadb.Error as err:
            logger.exception("Failed to create database - %s", err.msg)
            return -1

        # Use database
        try:
            mysql_cursor.execute(f"USE `{db_name}`")
        except mariadb.Error as err:
            logger.exception("Failed to use database - %s", err.msg)
            return -1

        # Create tables
        try:
            mysql_cursor.execute(
                f"""CREATE TABLE IF NOT EXISTS `{table_prefix}archives` (
                `pagination_id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
                `id` VARCHAR(64) NOT NULL,
                `begin_timestamp` BIGINT NOT NULL,
                `end_timestamp` BIGINT NOT NULL,
                `uncompressed_size` BIGINT NOT NULL,
                `size` BIGINT NOT NULL,
                `creator_id` VARCHAR(64) NOT NULL,
                `creation_ix` INT NOT NULL,
                KEY `archives_creation_order` (`creator_id`,`creation_ix`) USING BTREE,
                UNIQUE KEY `archive_id` (`id`) USING BTREE,
                PRIMARY KEY (`pagination_id`)
            )"""
            )

            mysql_cursor.execute(
                f"""CREATE TABLE IF NOT EXISTS `{table_prefix}files` (
                `id` VARCHAR(64) NOT NULL,
                `orig_file_id` VARCHAR(64) NOT NULL,
                `path` VARCHAR(12288) NOT NULL,
                `begin_timestamp` BIGINT NOT NULL,
                `end_timestamp` BIGINT NOT NULL,
                `num_uncompressed_bytes` BIGINT NOT NULL,
                `begin_message_ix` BIGINT NOT NULL,
                `num_messages` BIGINT NOT NULL,
                `archive_id` VARCHAR(64) NOT NULL,
                KEY `files_path` (`path`(768)) USING BTREE,
                KEY `files_archive_id` (`archive_id`) USING BTREE,
                PRIMARY KEY (`id`)
            ) ROW_FORMAT=DYNAMIC"""
            )
        except mariadb.Error as err:
            logger.exception("Failed to create table - %s", err.msg)
            return -1

        mysql_conn.commit()
    finally:
        mysql_cursor.close()
        mysql_conn.close()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
