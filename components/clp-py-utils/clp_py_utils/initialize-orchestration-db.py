#!/usr/bin/env python3
import argparse
import logging
import sys
from contextlib import closing

from pydantic import ValidationError
from sql_adapter import SQL_Adapter

from clp_py_utils.clp_config import CLPConfig
from clp_py_utils.core import read_yaml_config_file

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
    args_parser = argparse.ArgumentParser(description='Setup metadata tables for job orchestration.')
    args_parser.add_argument('--config', required=True, help='CLP package config file.')
    parsed_args = args_parser.parse_args(argv[1:])

    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(parsed_args.config))
        sql_adapter = SQL_Adapter(clp_config.database)
        with closing(sql_adapter.create_connection()) as scheduling_db, \
                closing(scheduling_db.cursor(dictionary=True)) as scheduling_db_cursor:
            scheduling_db_cursor.execute("""
                CREATE TABLE IF NOT EXISTS `compression_jobs` (
                    `job_id` INT NOT NULL AUTO_INCREMENT,
                    `job_status` VARCHAR(16) NOT NULL DEFAULT 'SCHEDULING',
                    `job_status_msg` VARCHAR(255) NOT NULL DEFAULT '',
                    `job_creation_time` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    `job_start_time` DATETIME NULL DEFAULT NULL,
                    `job_duration` INT NULL DEFAULT NULL,
                    `job_original_size` BIGINT NOT NULL DEFAULT '0',
                    `job_uncompressed_size` BIGINT NOT NULL DEFAULT '0',
                    `job_compressed_size` BIGINT NOT NULL DEFAULT '0',
                    `num_tasks` INT NOT NULL DEFAULT '0',
                    `num_tasks_completed` INT NOT NULL DEFAULT '0',
                    `clp_binary_version` INT NULL DEFAULT NULL,
                    `clp_config` VARBINARY(60000) NOT NULL,
                    PRIMARY KEY (`job_id`) USING BTREE,
                    INDEX `JOB_STATUS` (`job_status`) USING BTREE
                ) ROW_FORMAT=DYNAMIC
                ;
                """
            )

            scheduling_db_cursor.execute("""
                CREATE TABLE IF NOT EXISTS `compression_tasks` (
                    `task_id` BIGINT NOT NULL AUTO_INCREMENT,
                    `task_status` VARCHAR(16) NOT NULL DEFAULT 'SUBMITTED',
                    `task_scheduled_time` DATETIME NULL DEFAULT NULL,
                    `task_start_time` DATETIME NULL DEFAULT NULL,
                    `task_duration` SMALLINT NULL DEFAULT NULL,
                    `job_id` INT NOT NULL,
                    `clp_paths_to_compress` VARBINARY(60000) NOT NULL,
                    `partition_original_size` BIGINT NOT NULL,
                    `partition_uncompressed_size` BIGINT NULL DEFAULT NULL,
                    `partition_compressed_size` BIGINT NULL DEFAULT NULL,
                    PRIMARY KEY (`task_id`) USING BTREE,
                    INDEX `job_id` (`job_id`) USING BTREE,
                    INDEX `TASK_STATUS` (`task_status`) USING BTREE,
                    INDEX `TASK_START_TIME` (`task_start_time`) USING BTREE,
                    CONSTRAINT `compression_tasks` FOREIGN KEY (`job_id`) 
                    REFERENCES `compression_jobs` (`job_id`) ON UPDATE NO ACTION ON DELETE NO ACTION
                ) ROW_FORMAT=DYNAMIC
                ;
                """
            )

            scheduling_db.commit()
            logger.info('Successfully created compression_jobs and compression_tasks orchestration tables')

    except ValidationError as err:
        logger.error(err)
        return -1
    except Exception as ex:
        logger.error(ex)
        return -1

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
