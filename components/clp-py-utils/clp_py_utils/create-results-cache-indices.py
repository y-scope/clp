import argparse
import logging
import sys

from pymongo import IndexModel, MongoClient

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
    args_parser = argparse.ArgumentParser(description="Creates results cache indices for CLP.")
    args_parser.add_argument("--uri", required=True, help="URI of the results cache.")
    args_parser.add_argument("--ir-collection", required=True, help="Collection for IR metadata.")
    parsed_args = args_parser.parse_args(argv[1:])

    results_cache_uri = parsed_args.uri
    ir_collection_name = parsed_args.ir_collection

    try:
        with MongoClient(results_cache_uri) as results_cache_client:
            ir_collection = results_cache_client.get_default_database()[ir_collection_name]

            file_split_id_index = IndexModel(["file_split_id"])
            orig_file_id_index = IndexModel(["orig_file_id", "begin_msg_ix", "end_msg_ix"])
            ir_collection.create_indexes([file_split_id_index, orig_file_id_index])
    except Exception:
        logger.exception("Failed to create clp results cache indices.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
