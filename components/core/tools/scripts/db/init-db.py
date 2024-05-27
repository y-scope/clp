#!/usr/bin/env python3
import argparse
import logging
import mariadb
import yaml
import sys

# Setup logging
# Create logger
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    args_parser = argparse.ArgumentParser(description="Setup a global MySQL metadata database for CLP.")
    args_parser.add_argument("--config-file", required=True, help="Metadata database basic config file.")
    parsed_args = args_parser.parse_args(argv[1:])

    config_file_path = parsed_args.config_file

    with open(config_file_path, 'r') as f:
        config = yaml.safe_load(f)
    if config is None:
        raise Exception(f"Unable to parse configuration from {config_file_path}.")

    required_keys = ["host", "port", "username", "password", "name"]
    for key in required_keys:
        if key not in config:
            raise Exception(f"'{key}' missing from config file.")

    host = config["host"]
    port = config["port"]
    username = config["username"]
    password = config["password"]
    db_name = config["name"]
    table_prefix = config["table_prefix"]

    try:
        mysql_conn = mariadb.connect(host=host, port=port, username=username, password=password)
        mysql_cursor = mysql_conn.cursor()
    except mariadb.Error as err:
        logger.error("Failed to connect - {}".format(err.msg))
        return -1

    try:
        # Create database
        try:
            mysql_cursor.execute("CREATE DATABASE IF NOT EXISTS {} DEFAULT CHARACTER SET 'utf8'".format(db_name))
        except mariadb.Error as err:
            logger.error("Failed to create database - {}".format(err.msg))
            return -1

        # Use database
        try:
            mysql_cursor.execute("USE {}".format(db_name))
        except mariadb.Error as err:
            logger.error("Failed to use database - {}".format(err.msg))
            return -1

        # Create tables
        try:
            mysql_cursor.execute(f"""CREATE TABLE IF NOT EXISTS `{table_prefix}archives` (
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
            )""")

            mysql_cursor.execute(f"""CREATE TABLE IF NOT EXISTS `{table_prefix}files` (
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
            ) ROW_FORMAT=DYNAMIC""")
        except mariadb.Error as err:
            logger.error("Failed to create table - {}".format(err.msg))
            return -1

        mysql_conn.commit()
    finally:
        mysql_cursor.close()
        mysql_conn.close()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
