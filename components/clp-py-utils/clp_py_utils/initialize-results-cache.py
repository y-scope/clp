import argparse
import logging
import sys
from typing import Tuple
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


def check_replica_set_status(client: MongoClient, netloc: str) -> Tuple[bool, bool]:
    """
    Checks the current replica set status of the MongoDB server and determines whether it needs to
    be configured (or reconfigured).

    :param client:
    :param netloc: The network location of the replica set configuration.

    :return: A tuple containing:
        - is_already_initialized: Indicates whether the replica set has already been initialized.
        - should_configure_replica_set: Indicates whether the replica set needs to be configured (or
          reconfigured).

    :raises pymongo.errors.OperationFailure: If the server returns an unexpected error during the
    replica set status check.
    """
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


def init_replica_set_for_oplog(client: MongoClient, netloc: str, force: bool):
    """
    Initializes or reconfigures a single-node MongoDB replica set for enabling oplog.

    :param client:
    :param netloc: The network location of the MongoDB instance to configure
    as a replica set member.
    :param force: Forces reconfiguration.
    """
    logger.debug("Initializing single-node replica set for oplog at %s", netloc)

    # `replSetInitiate` can be called without a config object. However, explicit host
    # specification is required, or the docker's ID would be used as the hostname.
    config = {
        "_id": "rs0",
        "members": [{"_id": 0, "host": netloc}],
        "version": 1,
    }

    if force:
        # Use `force=True` so that we do not have to increment the version
        client.admin.command("replSetReconfig", config, force=True)
    else:
        client.admin.command("replSetInitiate", config)

    logger.debug("Single-node replica set initialized successfully.")


def init_replica_set_for_oplog_if_needed(client: MongoClient, uri: str):
    """
    Initializes a MongoDB single-node replica set for enabling oplog, if not already initialized.

    :param client: The MongoDB client instance used to connect to the server.
    :param uri: The MongoDB connection URI, which includes the network location (e.g.,
    `hostname:port`) of the MongoDB instance.

    :raises ValueError: If the provided URI is invalid.
    """
    parsed_uri = urlparse(uri)
    netloc = parsed_uri.netloc
    if 0 == len(netloc):
        raise ValueError("Invalid URI: %s", uri)

    logger.debug("Replica set initialization requested for %s", netloc)

    is_already_initialized, should_configure_replica_set = check_replica_set_status(client, netloc)

    if should_configure_replica_set:
        init_replica_set_for_oplog(client, netloc=netloc, force=is_already_initialized)


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
            init_replica_set_for_oplog_if_needed(results_cache_client, results_cache_uri)

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
