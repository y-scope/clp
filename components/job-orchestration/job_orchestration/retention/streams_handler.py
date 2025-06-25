import logging
import pathlib
import time
from typing import List

import pymongo
from bson import ObjectId
from clp_py_utils.clp_config import (
    CLPConfig,
    ResultsCache,
    StreamOutput,
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from job_orchestration.retention.utils import (
    configure_logger,
    get_expiry_epoch_secs,
    get_oid_with_expiry_time,
    MONGODB_ID_KEY,
    MONGODB_STREAM_PATH_KEY,
    remove_targets,
    TargetsBuffer,
    validate_storage_type,
)

HANDLER_NAME = "streams_retention_handler"
logger = get_logger(HANDLER_NAME)


def handle_stream_retention(
    logs_directory: pathlib.Path,
    stream_output_config: StreamOutput,
    results_cache_config: ResultsCache,
) -> None:
    expiry_epoch = get_expiry_epoch_secs(stream_output_config.retention_period)
    expiry_oid = get_oid_with_expiry_time(expiry_epoch)

    logger.info(f"Handler targeting all streams < {expiry_epoch}")
    logger.info(f"Translated to objectID = {expiry_oid}")

    recovery_file = logs_directory / f"{HANDLER_NAME}.tmp"
    targets_buffer = TargetsBuffer(recovery_file)

    with pymongo.MongoClient(results_cache_config.get_uri()) as results_cache_client:
        results_cache_db = results_cache_client.get_default_database()
        stream_collection = results_cache_db.get_collection(
            results_cache_config.stream_collection_name
        )
        # Find documents where _id (and thus creation time) is earlier
        retention_filter = {MONGODB_ID_KEY: {"$lt": expiry_oid}}
        results = stream_collection.find(retention_filter)
        object_ids_to_delete: List[ObjectId] = list()

        for stream in results:
            targets_buffer.add_target(stream.get(MONGODB_STREAM_PATH_KEY))
            object_ids_to_delete.append(stream.get(MONGODB_ID_KEY))

        targets_buffer.persists_new_targets()

        # TODO: here we don't use retention_filter again to avoid race condition between
        # targets to delete and timestamp of file.
        # This could create another race condition that
        # 1. daemon mark the entry as stale
        # 2. somehow webui access the entry and updates its last access time.
        # 3. daemon removes the file while log viewer tries to load it, race condition. boom
        # There are some strategy we can take to prevent this.
        # 1. Simply sleep for around ~10 seconds before deleting the files so most probably log viewer
        # would have already loaded it. (but how about files with other line numbers?)
        # 2. Let log viewer ignore streams outside of TTL and request creating new entries.
        if 0 != len(object_ids_to_delete):
            stream_collection.delete_many({MONGODB_ID_KEY: {"$in": object_ids_to_delete}})

    # TODO: I feel it's ok to always run this even if there's no results from pymongo
    # First, if we reached this line, it means either 1. no new targets has been added, or
    # some new target has been added, and we have already removed them from mongodb. either case,
    # the targets in the file must have been removed from the mongodb and not needed.
    remove_targets(stream_output_config, targets_buffer.get_targets())
    targets_buffer.flush()

    return


def stream_retention_entry(
    clp_config: CLPConfig, log_directory: pathlib, logging_level: str
) -> None:
    configure_logger(logger, logging_level, log_directory, HANDLER_NAME)

    job_frequency_secs = clp_config.retention_daemon.job_frequency.streams
    streams_retention_period = clp_config.stream_output.retention_period
    if streams_retention_period is None:
        logger.info("Stream retention period is not specified, terminate")
        return
    if job_frequency_secs is None:
        logger.info("Job frequency is not specified, terminate")

    stream_output_config: StreamOutput = clp_config.stream_output
    storage_engine: str = clp_config.package.storage_engine
    validate_storage_type(stream_output_config, storage_engine)

    while True:
        handle_stream_retention(
            clp_config.logs_directory, stream_output_config, clp_config.results_cache
        )
        time.sleep(job_frequency_secs)
