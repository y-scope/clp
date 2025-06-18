#!/usr/bin/env python3

import argparse
import logging
import os
import pathlib
import shutil
import sys
import time
import typing
from contextlib import closing
from datetime import datetime
from pathlib import Path
from typing import List, Set, Callable

import pymongo
import pymongo.database
from bson import ObjectId
from clp_py_utils.clp_config import (
    ARCHIVES_TABLE_SUFFIX,
    CLPConfig,
    DAEMON_COMPONENT_NAME,
    JobFrequency,
    ResultsCache,
    FsStorage,
    S3Storage,
    StorageEngine,
    StorageType,
    StreamOutput
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.s3_utils import s3_try_removing_object
from clp_py_utils.sql_adapter import SQL_Adapter
from pydantic import ValidationError
from pymongo.collection import Collection

logger = get_logger(DAEMON_COMPONENT_NAME)

MIN_TO_SECOND = 60
MONGODB_ID_KEY = "_id"

def get_target_time(retention_minutes: int):
    return int(time.time() - retention_minutes * MIN_TO_SECOND)


def try_removing_fs_file(fs_storage_config: FsStorage, relative_path: str) -> bool:
    file_path = fs_storage_config.directory / relative_path
    if not file_path.exists():
        return False

    if file_path.is_dir():
        raise ValueError(f"Path {file_path} resolves to a directory")

    os.remove(file_path)
    return True


# # Let's first not consider recovery from interrupt
# def handle_archive_retention(clp_config: CLPConfig) -> None:
#     archive_retention_period = clp_config.archive_output.retention_period
#     expiry_epoch = time.time() - archive_retention_period
#
#     logger.info("Creating SQL adapter")
#     sql_adapter = SQL_Adapter(clp_config.database)
#     clp_connection_param = clp_config.database.get_clp_connection_params_and_type()
#     table_prefix = clp_connection_param["table_prefix"]
#
#     with closing(sql_adapter.create_connection(True)) as db_conn, closing(
#         db_conn.cursor(dictionary=True)
#     ) as db_cursor:
#
#         # Remove all archives
#         db_cursor.execute(
#             f"""
#             DELETE FROM `{table_prefix}{ARCHIVES_TABLE_SUFFIX}`
#             WHERE end_timestamp <= %s
#             RETURNING id
#             """,
#             [expiry_epoch],
#         )
#         results = db_cursor.fetchall()
#
#         archive_ids: typing.List[str] = [result["id"] for result in results]
#         ids_list_string: str = ", ".join(["'%s'"] * len(archive_ids))
#
#         db_cursor.execute(
#             f"""
#             DELETE FROM `{table_prefix}{FILES_TABLE_SUFFIX}`
#             WHERE archive_id in ({ids_list_string})
#             """
#         )
#
#         db_cursor.execute(
#             f"""
#             DELETE FROM `{table_prefix}{ARCHIVE_TAGS_TABLE_SUFFIX}`
#             WHERE archive_id in ({ids_list_string})
#             """
#         )
#         for archive_id in archive_ids:
#             logger.info(f"Deleted archive {archive_id} from the database.")
#
#         if dry_run:
#             logger.info("Dry-run finished.")
#             db_conn.rollback()
#             return 0
#
#         db_conn.commit()


def archive_retention_entry(clp_config: CLPConfig, job_frequency: int) -> None:
    job_frequency_secs: int = job_frequency * MIN_TO_SECOND
    while True:
        time.sleep(job_frequency_secs)


def find_collection_stub(collection: pymongo.collection.Collection) -> typing.Optional[int]:
    latest_doc = collection.find().sort(MONGODB_ID_KEY, -1).limit(1)
    if latest_doc:
        latest_doc = list(latest_doc)
        object_id = latest_doc[0][MONGODB_ID_KEY]
        if isinstance(object_id, ObjectId):
            return int(object_id.generation_time.timestamp())
        else:
            return None


def try_removing_results_metadata(
    database: pymongo.database.Database, collection_name: str
) -> None:
    results_metadata_collection = database.get_collection("results-metadata")
    results_metadata_collection.delete_one({MONGODB_ID_KEY: str(collection_name)})


def handle_search_results_retention(clp_config: CLPConfig):
    result_cache_config: ResultsCache = clp_config.results_cache
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
            if collection_stub is None:
                # TODO: how do we handle this?
                logger.warning("Unexpected empty collection")
                continue

            logger.info(f"Collection stub: {collection_name}, {collection_stub}")
            if collection_stub < expiry_ts:
                logger.info(f"Removing collection: {collection_name}")
                # TODO
                try_removing_results_metadata(results_cache_db, collection_name)
                collection.drop()


def search_results_retention_entry(clp_config: CLPConfig, job_frequency_secs: int) -> None:
    while True:
        handle_search_results_retention(clp_config)
        time.sleep(job_frequency_secs)


# May require some redesign
class RecoveryFile:
    def __init__(self, path: pathlib.Path):
        self.file_path = path
    def load(self) -> Set[str]:
        res = set()
        with open(self.file_path, "r") as f:
            for line in f.readlines():
                res.add(line)
        return res

    def append(self, lines: Set[str]) -> None:
        with open(self.file_path, "a") as f:
            for line in lines:
                f.write(line + "\n")


def handle_removal(stream_output_config: StreamOutput, stream_path: str) -> bool:
    stream_storage_config = stream_output_config.storage
    stream_storage_type = stream_storage_config.type

    if StorageType.S3 == stream_storage_type:
        stream_s3_config = stream_storage_config.s3_config
        return s3_try_removing_object(stream_storage_config.s3_config, stream_path)
    elif StorageType.FS == stream_storage_type:
        return try_removing_fs_path(stream_storage_config, stream_path)
    else:
        raise ValueError(
            f"Stream storage type {stream_storage_type} is not supported"
        )


def handle_stream_retention(stream_output_config: StreamOutput, results_cache_config: ResultsCache) -> None:
    expiry_time = get_target_time(stream_output_config.retention_period)
    expiry_oid = ObjectId.from_datetime(datetime.utcfromtimestamp(expiry_time))

    logger.info(f"Handler targeting all streams < {expiry_time}")
    logger.info(f"Translated to objectID = {expiry_oid}")

    results_cache_uri = results_cache_config.get_uri()
    with pymongo.MongoClient(results_cache_uri) as results_cache_client:
        results_cache_db = results_cache_client.get_default_database()
        stream_collection = results_cache_db.get_collection(
            results_cache_config.stream_collection_name
        )
        # Find documents where _id (and thus creation time) is earlier
        retention_filter = {MONGODB_ID_KEY: {"$lt": expiry_oid}}
        results = stream_collection.find(retention_filter)
        for stream in results:
            stream_path = stream["path"]
            logger.info(f"Plan to remove {stream_path}")
            res = handle_removal(stream_output_config, stream_path)
            if not res:
                logger.error(f"failed to remove {stream_path}")

        stream_collection.delete_many(retention_filter)
    return


def stream_retention_entry(clp_config: CLPConfig, job_frequency_secs: int) -> None:
    stream_output_config = clp_config.stream_output
    storage_type = stream_output_config.storage.type

    if StorageType.S3 == storage_type:
        storage_engine = clp_config.package.storage_engine
        if StorageEngine.CLP_S != storage_engine:
            # TODO: update error message
            raise ValueError(f"Stream storage type {storage_type} is not supported when using storage engine {storage_engine}")
    elif StorageType.FS != storage_type:
        raise ValueError(
            f"Stream storage type {storage_type} is not supported"
        )

    while True:
        handle_stream_retention(stream_output_config, clp_config.results_cache)
        time.sleep(job_frequency_secs)


def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(description=f"Spin up the {DAEMON_COMPONENT_NAME}.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")

    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / f"{DAEMON_COMPONENT_NAME}.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    set_logging_level(logger, os.getenv("CLP_LOGGING_LEVEL"))

    # Load configuration
    config_path = Path(parsed_args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
        return 1
    except Exception as ex:
        logger.error(ex)
        return 1

    clp_home = Path(os.getenv("CLP_HOME"))
    # fmt: off
    archive_retention_period = clp_config.archive_output.retention_period
    logger.info(f"Archive retention period: {archive_retention_period}")
    stream_retention_period = clp_config.stream_output.retention_period
    logger.info(f"Stream retention period: {stream_retention_period}")
    results_cache_retention_period = clp_config.results_cache.retention_period
    logger.info(f"Results cache retention period: {results_cache_retention_period}")

    stream_retention_entry(clp_config, 30)

    logger.info("reducer terminated")

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
