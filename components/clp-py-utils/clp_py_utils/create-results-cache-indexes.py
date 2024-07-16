import argparse
import logging
import pathlib
import sys

import pymongo

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)

from clp_py_utils.clp_config import ResultsCache
from clp_py_utils.core import read_yaml_config_file


def main(argv):
    args_parser = argparse.ArgumentParser(description="Creates results cache indexes for CLP.")
    args_parser.add_argument("--config", required=True, help="Results cache config file.")
    parsed_args = args_parser.parse_args(argv[1:])

    config_file_path = pathlib.Path(parsed_args.config)

    try:
        results_cache_config = ResultsCache.parse_obj(read_yaml_config_file(config_file_path))
        if results_cache_config is None:
            raise ValueError(f"results cache configuration file '{parsed_args.config}' is empty.")

        results_cache_uri = results_cache_config.get_uri()
        ir_collection_name = results_cache_config.ir_collection_name

        with pymongo.MongoClient(results_cache_uri) as results_cache_client:
            ir_collection = results_cache_client.get_default_database()[ir_collection_name]
            ir_collection.create_index(["file_split_id"])
            ir_collection.create_index(["orig_file_id", "begin_msg_ix", "end_msg_ix"])

    except:
        logger.exception("Failed to create clp results cache indexes.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
