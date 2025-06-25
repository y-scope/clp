import asyncio
import pathlib

import pymongo
import pymongo.database
from bson import ObjectId
from clp_py_utils.clp_config import CLPConfig, ResultsCache
from clp_py_utils.clp_logging import get_logger
from job_orchestration.retention.constants import (
    MIN_TO_SECONDS,
    SEARCH_RESULTS_RETENTION_HANDLER_NAME,
)
from job_orchestration.retention.utils import (
    configure_logger,
    get_expiry_epoch_secs,
    MONGODB_ID_KEY,
    RESULTS_METADATA_COLLECTION,
)

logger = get_logger(SEARCH_RESULTS_RETENTION_HANDLER_NAME)


def _get_latest_doc_timestamp(collection: pymongo.collection.Collection) -> int:
    latest_doc = collection.find_one(sort=[(MONGODB_ID_KEY, pymongo.DESCENDING)])
    if latest_doc:
        object_id = latest_doc[MONGODB_ID_KEY]
        if isinstance(object_id, ObjectId):
            return int(object_id.generation_time.timestamp())
        else:
            raise ValueError(f"{object_id} is not a ObjectID")

    return 0


def _remove_result_metadata(database: pymongo.database.Database, collection_name: str) -> None:
    results_metadata_collection = database.get_collection(RESULTS_METADATA_COLLECTION)
    results_metadata_collection.delete_one({MONGODB_ID_KEY: collection_name})


def _handle_search_results_retention(result_cache_config: ResultsCache):
    expiry_epoch = get_expiry_epoch_secs(result_cache_config.retention_period)
    with pymongo.MongoClient(result_cache_config.get_uri()) as results_cache_client:
        results_cache_db = results_cache_client.get_default_database()
        collection_names = results_cache_db.list_collection_names()
        for collection_name in collection_names:
            if not collection_name.isdigit():
                continue

            collection = results_cache_db.get_collection(collection_name)
            collection_timestamp = _get_latest_doc_timestamp(collection)

            if collection_timestamp < expiry_epoch:
                logger.debug(f"Removing search results: {collection_name}")
                _remove_result_metadata(results_cache_db, collection_name)
                collection.drop()


async def search_results_retention(
    clp_config: CLPConfig, log_directory: pathlib, logging_level: str
) -> None:
    configure_logger(logger, logging_level, log_directory, SEARCH_RESULTS_RETENTION_HANDLER_NAME)

    job_frequency_minutes = clp_config.retention_daemon.job_frequency.search_results
    while True:
        _handle_search_results_retention(clp_config.results_cache)
        await asyncio.sleep(job_frequency_minutes * MIN_TO_SECONDS)
