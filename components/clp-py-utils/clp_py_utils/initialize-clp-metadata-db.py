#!/usr/bin/env python3
import argparse
import logging
import pathlib
import sys
from contextlib import closing

from pydantic import ValidationError

from clp_py_utils.clp_config import ClpConfig, StorageEngine
from clp_py_utils.clp_metadata_db_utils import create_datasets_table, create_metadata_db_tables
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.sql_adapter import SqlAdapter

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
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    args_parser.add_argument(
        "--storage-engine",
        type=str,
        choices=[engine.value for engine in StorageEngine],
        required=True,
        help="Storage engine to create tables for.",
    )
    parsed_args = args_parser.parse_args(argv[1:])
    storage_engine = StorageEngine(parsed_args.storage_engine)

    # Load configuration
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

    try:
        sql_adapter = SqlAdapter(clp_config.database)
        clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        with (
            closing(sql_adapter.create_connection(True)) as metadata_db,
            closing(metadata_db.cursor(dictionary=True)) as metadata_db_cursor,
        ):
            if StorageEngine.CLP_S == storage_engine:
                create_datasets_table(metadata_db_cursor, table_prefix)
            else:
                create_metadata_db_tables(metadata_db_cursor, table_prefix)
            metadata_db.commit()
    except:
        logger.exception("Failed to create clp metadata tables.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
