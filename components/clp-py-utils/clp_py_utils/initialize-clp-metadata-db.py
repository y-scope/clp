#!/usr/bin/env python3
import argparse
import logging
import sys
from contextlib import closing
from pathlib import Path

from sql_adapter import SQL_Adapter

from clp_py_utils.clp_config import (
    Database,
    StorageEngine,
)
from clp_py_utils.clp_metadata_db_utils import (
    create_datasets_table,
    create_metadata_db_tables,
)
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
    args_parser = argparse.ArgumentParser(description="Sets up CLP's metadata tables.")
    args_parser.add_argument("--config", required=True, help="Database config file.")
    args_parser.add_argument(
        "--storage-engine",
        type=str,
        choices=[engine.value for engine in StorageEngine],
        required=True,
        help="Storage engine to create tables for.",
    )
    parsed_args = args_parser.parse_args(argv[1:])

    config_file_path = Path(parsed_args.config)
    storage_engine = StorageEngine(parsed_args.storage_engine)

    try:
        database_config = Database.parse_obj(read_yaml_config_file(config_file_path))
        if database_config is None:
            raise ValueError(f"Database configuration file '{config_file_path}' is empty.")
        sql_adapter = SQL_Adapter(database_config)
        clp_db_connection_params = database_config.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        with closing(sql_adapter.create_connection(True)) as metadata_db, closing(
            metadata_db.cursor(dictionary=True)
        ) as metadata_db_cursor:
            # TODO: After the dataset feature is fully implemented, for clp-json:
            # 1. Populate the datasets table with the name and path for the "default" dataset.
            # 2. Change the metadata tables to be specific to the "default" dataset.
            if StorageEngine.CLP_S == storage_engine:
                create_datasets_table(metadata_db_cursor, table_prefix)
            create_metadata_db_tables(metadata_db_cursor, table_prefix)
            metadata_db.commit()
    except:
        logger.exception("Failed to create clp metadata tables.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
