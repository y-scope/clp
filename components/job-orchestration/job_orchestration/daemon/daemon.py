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
from typing import List, Set, Optional

import pymongo
import pymongo.database
from bson import ObjectId
from clp_py_utils.clp_config import (
    ARCHIVES_TABLE_SUFFIX,
    CLPConfig,
    DAEMON_COMPONENT_NAME,
    FsStorage,
    JobFrequency,
    ResultsCache,
    S3Storage,
    StorageEngine,
    StorageType,
    StreamOutput,
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
MONGODB_STREAM_PATH_KEY = "path"
# TODO: consider to make this a shared constant between webui and package scripts
RESULTS_METADATA_COLLECTION = "results-metadata"


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
#
#
#         db_conn.commit()


def archive_retention_entry(clp_config: CLPConfig, job_frequency: int) -> None:
    job_frequency_secs: int = job_frequency * MIN_TO_SECOND
    while True:
        time.sleep(job_frequency_secs)


# For empty collection, we return 0
def find_collection_stub(collection: pymongo.collection.Collection) -> int:
    latest_doc = collection.find_one(sort=[(MONGODB_ID_KEY, pymongo.DESCENDING)])
    if latest_doc:
        object_id = latest_doc[MONGODB_ID_KEY]
        if isinstance(object_id, ObjectId):
            return int(object_id.generation_time.timestamp())
        else:
            raise ValueError(f"{object_id} is not in the form of ObjectID")

    return 0


def try_removing_results_metadata(
    database: pymongo.database.Database, collection_name: str
) -> None:
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
                logger.info(f"Removing collection: {collection_name}")
                try_removing_results_metadata(results_cache_db, collection_name)
                collection.drop()


def search_results_retention_entry(clp_config: CLPConfig, job_frequency_secs: int) -> None:
    while True:
        handle_search_results_retention(clp_config.results_cache)
        time.sleep(job_frequency_secs)


# May require some redesign
class TargetsHandle:
    def __init__(self, recovery_file_path: pathlib.Path):
        self._recovery_file_path: Optional[pathlib.Path] = recovery_file_path
        self._targets: Set[str] = set()
        self._new_targets: List[str] = list()

        if not self._recovery_file_path.exists():
            return

        with open(self._recovery_file_path, "r") as f:
            for line in f:
                self._targets.add(line.strip())

    def add_target(self, target: str) -> None:
        if target not in self._targets:
            self._targets.add(target)
            self._new_targets.append(target)

    def get_targets(self) -> Set[str]:
        return self._targets

    def flush_new_targets(self) -> None:
        if len(self._new_targets) == 0:
            return

        with open(self._recovery_file_path, "a") as f:
            for target in self._new_targets:
                f.write(f"{target}\n")
    def clean(self):
        if self._recovery_file_path.exists():
            os.unlink(self._recovery_file_path)

def handle_removal(stream_output_config: StreamOutput, stream_path: str) -> bool:
    stream_storage_config = stream_output_config.storage
    stream_storage_type = stream_storage_config.type

    if StorageType.S3 == stream_storage_type:
        return s3_try_removing_object(stream_storage_config.s3_config, stream_path)
    elif StorageType.FS == stream_storage_type:
        return try_removing_fs_file(stream_storage_config, stream_path)
    else:
        raise ValueError(f"Stream storage type {stream_storage_type} is not supported")


def handle_stream_retention(
    logs_directory: pathlib.Path,
    stream_output_config: StreamOutput,
    results_cache_config: ResultsCache
) -> None:
    expiry_time = get_target_time(stream_output_config.retention_period)
    expiry_oid = ObjectId.from_datetime(datetime.utcfromtimestamp(expiry_time))

    logger.info(f"Handler targeting all streams < {expiry_time}")
    logger.info(f"Translated to objectID = {expiry_oid}")

    recovery_file = logs_directory / "stream_retention.tmp"
    targets_handle = TargetsHandle(recovery_file)

    results_cache_uri = results_cache_config.get_uri()
    with pymongo.MongoClient(results_cache_uri) as results_cache_client:
        results_cache_db = results_cache_client.get_default_database()
        stream_collection = results_cache_db.get_collection(
            results_cache_config.stream_collection_name
        )
        # Find documents where _id (and thus creation time) is earlier
        retention_filter = {MONGODB_ID_KEY: {"$lt": expiry_oid}}
        results = stream_collection.find(retention_filter)
        object_ids_to_delete: List[ObjectId] = list()

        for stream in results:
            stream_path = stream.get(MONGODB_STREAM_PATH_KEY)
            targets_handle.add_target(stream_path)
            object_ids_to_delete.append(stream.get(MONGODB_ID_KEY))

        targets_handle.flush_new_targets()

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
            stream_collection.delete_many({MONGODB_ID_KEY: {'$in': object_ids_to_delete}})


    # TODO: I feel it's ok to always run this even if there's no results from pymongo
    # First, if we reached this line, it means either 1. no new targets has been added, or
    # some new target has been added, and we have already removed them from mongodb. either case,
    # the targets in the file must have been removed from the mongodb and not needed.
    for target in targets_handle.get_targets():
        logger.info(f"removing {target}")
        _ = handle_removal(stream_output_config, target)

    targets_handle.clean()
    return


def stream_retention_entry(clp_config: CLPConfig, job_frequency_secs: int) -> None:
    stream_output_config = clp_config.stream_output
    storage_type = stream_output_config.storage.type

    if StorageType.S3 == storage_type:
        storage_engine = clp_config.package.storage_engine
        if StorageEngine.CLP_S != storage_engine:
            # TODO: update error message
            raise ValueError(
                f"Stream storage type {storage_type} is not supported when using storage engine {storage_engine}"
            )
    elif StorageType.FS != storage_type:
        raise ValueError(f"Stream storage type {storage_type} is not supported")

    while True:
        handle_stream_retention(clp_config.logs_directory, stream_output_config, clp_config.results_cache)
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
