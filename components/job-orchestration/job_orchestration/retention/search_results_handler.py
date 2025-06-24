import time
from typing import List

import pymongo
import pymongo.database
from bson import ObjectId
from clp_py_utils.clp_config import CLPConfig, ResultsCache
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from job_orchestration.retention.utils import (
    get_target_time,
    MONGODB_ID_KEY,
    RESULTS_METADATA_COLLECTION,
)

logger = get_logger("SEARCH_RESULTS_RETENTION_HANDLER")


def find_collection_stub(collection: pymongo.collection.Collection) -> int:
    latest_doc = collection.find_one(sort=[(MONGODB_ID_KEY, pymongo.DESCENDING)])
    if latest_doc:
        object_id = latest_doc[MONGODB_ID_KEY]
        if isinstance(object_id, ObjectId):
            return int(object_id.generation_time.timestamp())
        else:
            raise ValueError(f"{object_id} is not a ObjectID")

    return 0


def remove_result_metadata(database: pymongo.database.Database, collection_name: str) -> None:
    results_metadata_collection = database.get_collection(RESULTS_METADATA_COLLECTION)
    results_metadata_collection.delete_one({MONGODB_ID_KEY: collection_name})


def handle_search_results_retention(result_cache_config: ResultsCache):
    expiry_ts = get_target_time(result_cache_config.retention_period)
    logger.info(f"Handler targeting all search results < {expiry_ts}")

    results_cache_uri = result_cache_config.get_uri()
    with pymongo.MongoClient(results_cache_uri) as results_cache_client:
        results_cache_db = results_cache_client.get_default_database()
        collection_names: List[str] = results_cache_db.list_collection_names()
        for collection_name in collection_names:
            if not collection_name.isdigit():
                continue

            collection = results_cache_db.get_collection(collection_name)
            collection_stub = find_collection_stub(collection)

            logger.info(f"Collection stub: {collection_name}, {collection_stub}")
            if collection_stub < expiry_ts:
                logger.debug(f"Removing collection: {collection_name}")
                remove_result_metadata(results_cache_db, collection_name)
                collection.drop()


def search_results_retention_entry(clp_config: CLPConfig, job_frequency_secs: int) -> None:
    while True:
        handle_search_results_retention(clp_config.results_cache)
        time.sleep(job_frequency_secs)
