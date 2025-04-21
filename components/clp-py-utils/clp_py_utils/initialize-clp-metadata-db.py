#!/usr/bin/env python3
import argparse
import logging
import sys
from contextlib import closing

from sql_adapter import SQL_Adapter

from clp_py_utils.clp_config import (
    ARCHIVE_TAGS_TABLE_SUFFIX,
    ARCHIVES_TABLE_SUFFIX,
    Database,
    DATASETS_TABLE_SUFFIX,
    FILES_TABLE_SUFFIX,
    TAGS_TABLE_SUFFIX,
)
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_table_schema_utils import create_archives_table

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
    args_parser = argparse.ArgumentParser(description="Sets up CLP's metadata tables.")
    args_parser.add_argument("--config", required=True, help="Database config file.")
    parsed_args = args_parser.parse_args(argv[1:])

    try:
        database_config = Database.parse_obj(read_yaml_config_file(parsed_args.config))
        if database_config is None:
            raise ValueError(f"Database configuration file '{parsed_args.config}' is empty.")
        sql_adapter = SQL_Adapter(database_config)
        clp_db_connection_params = database_config.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        with closing(sql_adapter.create_connection(True)) as metadata_db, closing(
            metadata_db.cursor(dictionary=True)
        ) as metadata_db_cursor:
            create_archives_table(metadata_db_cursor, f"{table_prefix}{ARCHIVES_TABLE_SUFFIX}")
            create_tags_table(metadata_db_cursor, f"{table_prefix}{TAGS_TABLE_SUFFIX}")
            create_archive_tags_table(
                metadata_db_cursor, f"{table_prefix}{ARCHIVE_TAGS_TABLE_SUFFIX}"
            )
            create_files_table(metadata_db_cursor, f"{table_prefix}{FILES_TABLE_SUFFIX}")
            metadata_db.commit()
    except:
        logger.exception("Failed to create clp metadata tables.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
