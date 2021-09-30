#!/usr/bin/env python3
import argparse
import logging
import sys
from contextlib import closing

from pydantic import ValidationError

from clp_py_utils.clp_config import CLPConfig
from clp_py_utils.core import read_yaml_config_file
from sql_adapter import SQL_Adapter

# Setup logging
# Create logger
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    args_parser = argparse.ArgumentParser(
        description='Setup CLP metadata tables compression and search.')
    args_parser.add_argument('--config', required=True, help='CLP package config file.')
    parsed_args = args_parser.parse_args(argv[1:])

    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(parsed_args.config))
        sql_adapter = SQL_Adapter(clp_config.database)
        with closing(sql_adapter.create_connection()) as metadata_db, \
                closing(metadata_db.cursor(dictionary=True)) as metadata_db_cursor:
            metadata_db_cursor.execute("""
                CREATE TABLE IF NOT EXISTS `archives` (
                    `pagination_id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
                    `id` VARCHAR(64) NOT NULL,
                    `storage_id` VARCHAR(64) NOT NULL,
                    `uncompressed_size` BIGINT NOT NULL,
                    `size` BIGINT NOT NULL,
                    `creator_id` VARCHAR(64) NOT NULL,
                    `creation_ix` INT NOT NULL,
                    KEY `archives_creation_order` (`creator_id`,`creation_ix`) USING BTREE,
                    UNIQUE KEY `archive_id` (`id`) USING BTREE,
                    PRIMARY KEY (`pagination_id`)
                );
                """
            )

            metadata_db_cursor.execute("""
                CREATE TABLE IF NOT EXISTS `files` (
                    `id` VARCHAR(64) NOT NULL,
                    `orig_file_id` VARCHAR(64) NOT NULL,
                    `path` VARCHAR(12288) NOT NULL,
                    `begin_timestamp` BIGINT NOT NULL,
                    `end_timestamp` BIGINT NOT NULL,
                    `num_uncompressed_bytes` BIGINT NOT NULL,
                    `num_messages` BIGINT NOT NULL,
                    `archive_id` VARCHAR(64) NOT NULL,
                    KEY `files_path` (path(768)) USING BTREE,
                    KEY `files_archive_id` (`archive_id`) USING BTREE,
                    PRIMARY KEY (`id`)
                ) ROW_FORMAT=DYNAMIC
                ;
                """
            )

            metadata_db.commit()
            logger.info('Successfully created clp metadata tables for compression and search')

    except ValidationError as err:
        logger.error(err)
        return -1
    except Exception as ex:
        logger.error(ex)
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
