import asyncio
import pathlib
from typing import Final

import pymongo
import pymongo.database
from bson import ObjectId
from clp_py_utils.clp_config import CLPConfig, ResultsCache
from clp_py_utils.clp_logging import get_logger
from job_orchestration.garbage_collector.constants import (
    MIN_TO_SECONDS,
    SEARCH_RESULT_RETENTION_HANDLER_NAME,
)
from job_orchestration.garbage_collector.utils import (
    configure_logger,
    get_expiry_epoch_secs,
)

# Constants
MONGODB_ID_KEY: Final[str] = "_id"

logger = get_logger(SEARCH_RESULT_RETENTION_HANDLER_NAME)


def _get_latest_doc_timestamp(collection: pymongo.collection.Collection) -> int:
    latest_doc = collection.find_one(sort=[(MONGODB_ID_KEY, pymongo.DESCENDING)])
    if latest_doc:
        object_id = latest_doc[MONGODB_ID_KEY]
        if isinstance(object_id, ObjectId):
            return int(object_id.generation_time.timestamp())
        raise ValueError(f"{object_id} is not a ObjectID")

    return 0


def _remove_result_metadata(
    database: pymongo.database.Database, results_metadata_collection_name: str, job_id: str
) -> None:
    results_metadata_collection = database.get_collection(results_metadata_collection_name)
    results_metadata_collection.delete_one({MONGODB_ID_KEY: job_id})


def _handle_search_result_retention(
    result_cache_config: ResultsCache, results_metadata_collection_name: str
):
    expiry_epoch = get_expiry_epoch_secs(result_cache_config.retention_period)
    with pymongo.MongoClient(result_cache_config.get_uri()) as results_cache_client:
        results_cache_db = results_cache_client.get_default_database()
        collection_names = results_cache_db.list_collection_names()
        for job_id in collection_names:
            if not job_id.isdigit():
                continue

            job_results_collection = results_cache_db.get_collection(job_id)
            collection_timestamp = _get_latest_doc_timestamp(job_results_collection)

            if collection_timestamp < expiry_epoch:
                logger.debug(f"Removing search result for job: {job_id}")
                _remove_result_metadata(results_cache_db, results_metadata_collection_name, job_id)
                job_results_collection.drop()


async def search_result_retention(
    clp_config: CLPConfig, log_directory: pathlib.Path, logging_level: str
) -> None:
    configure_logger(logger, logging_level, log_directory, SEARCH_RESULT_RETENTION_HANDLER_NAME)

    sweep_interval_secs = clp_config.garbage_collector.sweep_interval.search_result * MIN_TO_SECONDS
    while True:
        _handle_search_result_retention(
            clp_config.results_cache, clp_config.webui.results_metadata_collection_name
        )
        await asyncio.sleep(sweep_interval_secs)
