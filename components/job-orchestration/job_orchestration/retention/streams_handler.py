import asyncio
import pathlib
from typing import List

import pymongo
from bson import ObjectId
from clp_py_utils.clp_config import (
    CLPConfig,
    ResultsCache,
    StreamOutput,
)
from clp_py_utils.clp_logging import get_logger
from job_orchestration.retention.constants import MIN_TO_SECONDS, STREAMS_RETENTION_HANDLER_NAME
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

logger = get_logger(STREAMS_RETENTION_HANDLER_NAME)


def _handle_stream_retention(
    logs_directory: pathlib.Path,
    stream_output_config: StreamOutput,
    results_cache_config: ResultsCache,
) -> None:
    expiry_epoch_secs = get_expiry_epoch_secs(stream_output_config.retention_period)
    expiry_oid = get_oid_with_expiry_time(expiry_epoch_secs)

    recovery_file = logs_directory / f"{STREAMS_RETENTION_HANDLER_NAME}.tmp"
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
            logger.debug(f"Deleting {stream.get(MONGODB_STREAM_PATH_KEY)}")
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


async def stream_retention(
    clp_config: CLPConfig, log_directory: pathlib, logging_level: str
) -> None:
    configure_logger(logger, logging_level, log_directory, STREAMS_RETENTION_HANDLER_NAME)

    stream_output_config = clp_config.stream_output
    storage_engine = clp_config.package.storage_engine
    validate_storage_type(stream_output_config, storage_engine)

    job_frequency_minutes = clp_config.retention_cleaner.job_frequency.streams
    while True:
        _handle_stream_retention(
            clp_config.logs_directory, stream_output_config, clp_config.results_cache
        )
        await asyncio.sleep(job_frequency_minutes * MIN_TO_SECONDS)
