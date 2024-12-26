import argparse
import logging
import sys
from urllib.parse import urlparse

from pymongo import IndexModel, MongoClient
from pymongo.errors import OperationFailure

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def check_replica_set_status(client, netloc):
    is_already_initialized = False
    should_configure_replica_set = False

    try:
        result = client.admin.command("replSetGetStatus")
        is_already_initialized = True
        existing_netloc = result["members"][0]["name"]
        if netloc == existing_netloc:
            logger.debug("Replica set is already initialized at %s", existing_netloc)
        else:
            logger.warning(
                "Replica set is already initialized at %s, but requested at %s",
                existing_netloc,
                netloc,
            )
            should_configure_replica_set = True
    except OperationFailure as e:
        should_configure_replica_set = True

        if 94 == e.code:  # codeName: NotYetInitialized
            logger.debug("Replica set has not been previously initialized.")
        elif 93 == e.code:  # codeName: InvalidReplicaSetConfig
            logger.debug("Replica set is already initialized but reports invalid config.")
            is_already_initialized = True
        else:
            raise e

    return is_already_initialized, should_configure_replica_set


def configure_replica_set(client, is_already_initialized, netloc):
    logger.debug("Initializing replica set at %s", netloc)

    # `replSetInitiate` can be called without a config object. However, explicit host
    # specification is required, or the docker's ID would be used as the hostname.
    config = {
        "_id": "rs0",
        "members": [{"_id": 0, "host": netloc}],
        "version": 1,
    }

    if is_already_initialized:
        # Use `force=True` so that we do not have to increment the version
        client.admin.command("replSetReconfig", config, force=True)
    else:
        client.admin.command("replSetInitiate", config)

    logger.debug("Replica set initialized successfully.")


def configure_replica_set_if_needed(client, uri):
    parsed_uri = urlparse(uri)
    netloc = parsed_uri.netloc
    if 0 == len(netloc):
        raise ValueError("Invalid URI: %s", uri)

    logger.debug("Replica set initialization requested for %s", netloc)

    is_already_initialized, should_configure_replica_set = check_replica_set_status(client, netloc)

    if should_configure_replica_set:
        configure_replica_set(client, is_already_initialized, netloc)


def main(argv):
    args_parser = argparse.ArgumentParser(description="Creates results cache indices for CLP.")
    args_parser.add_argument("--uri", required=True, help="URI of the results cache.")
    args_parser.add_argument(
        "--stream-collection", required=True, help="Collection for stream metadata."
    )
    parsed_args = args_parser.parse_args(argv[1:])

    results_cache_uri = parsed_args.uri
    stream_collection_name = parsed_args.stream_collection

    try:
        with MongoClient(results_cache_uri, directConnection=True) as results_cache_client:
            configure_replica_set_if_needed(results_cache_client, results_cache_uri)

        with MongoClient(results_cache_uri) as results_cache_client:
            stream_collection = results_cache_client.get_default_database()[stream_collection_name]

            file_split_id_index = IndexModel(["file_split_id"])
            orig_file_id_index = IndexModel(["orig_file_id", "begin_msg_ix", "end_msg_ix"])
            stream_collection.create_indexes([file_split_id_index, orig_file_id_index])
    except Exception:
        logger.exception("Failed to create clp results cache indices.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
