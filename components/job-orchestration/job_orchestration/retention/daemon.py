#!/usr/bin/env python3

import argparse
import logging
import os
import pathlib
import shutil
import sys
import time
from contextlib import closing
from datetime import datetime
from pathlib import Path
from typing import List, Optional, Set, Union

import pymongo
import pymongo.database
from bson import ObjectId
from clp_py_utils.clp_config import (
    ArchiveOutput,
    CLPConfig,
    RETENTION_DAEMON_COMPONENT_NAME,
    FsStorage,
    JobFrequency,
    ResultsCache,
    StorageEngine,
    StorageType,
    StreamOutput,
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.clp_metadata_db_utils import (
    delete_archives_from_metadata_db,
    fetch_existing_datasets,
    get_archives_table_name,
)
from clp_py_utils.constants import MIN_TO_SECOND, SECOND_TO_MILLISECOND
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.s3_utils import s3_delete_objects
from clp_py_utils.sql_adapter import SQL_Adapter
from pydantic import ValidationError

logger = get_logger(RETENTION_DAEMON_COMPONENT_NAME)

MONGODB_ID_KEY = "_id"
MONGODB_STREAM_PATH_KEY = "path"
# TODO: consider to make this a shared constant between webui and package scripts
RESULTS_METADATA_COLLECTION = "results-metadata"


def get_target_time(retention_minutes: int):
    return int(time.time() - retention_minutes * MIN_TO_SECOND)


def remove_fs_target(fs_storage_config: FsStorage, relative_path: str) -> None:
    path_to_remove = fs_storage_config.directory / relative_path
    if not path_to_remove.exists():
        return

    if path_to_remove.is_dir():
        shutil.rmtree(path_to_remove)
    else:
        os.remove(path_to_remove)


# May require some redesign
class TargetsBuffer:
    def __init__(self, recovery_file_path: pathlib.Path):
        logger.info(recovery_file_path)
        self._recovery_file_path: pathlib.Path = recovery_file_path
        self._targets: Set[str] = set()
        self._targets_to_persist: List[str] = list()

        if not self._recovery_file_path.exists():
            return

        if not self._recovery_file_path.is_file():
            raise ValueError(f"{self._recovery_file_path} does not resolve to a file")

        with open(self._recovery_file_path, "r") as f:
            for line in f:
                self._targets.add(line.strip())

    def add_target(self, target: str) -> None:
        if target not in self._targets:
            self._targets.add(target)
            self._targets_to_persist.append(target)

    def get_targets(self) -> Set[str]:
        return self._targets

    def persists_new_targets(self) -> None:
        if len(self._targets_to_persist) == 0:
            return

        with open(self._recovery_file_path, "a") as f:
            for target in self._targets_to_persist:
                f.write(f"{target}\n")

        self._targets_to_persist.clear()

    def flush(self):
        self._targets.clear()
        if self._recovery_file_path.exists():
            os.unlink(self._recovery_file_path)


def archive_retention_helper(
    db_conn,
    db_cursor,
    table_prefix: str,
    archive_expiry_epoch: int,
    targets_buffer: TargetsBuffer,
    archive_output_config: ArchiveOutput,
    dataset: Optional[str],
):
    # Remove all archives
    archives_table = get_archives_table_name(table_prefix, dataset)
    db_cursor.execute(
        f"""
        SELECT id FROM `{archives_table}`
        WHERE end_timestamp <= %s
        AND end_timestamp != 0
        """,
        [archive_expiry_epoch],
    )

    results = db_cursor.fetchall()
    archive_ids: List[str] = [result["id"] for result in results]
    if len(archive_ids) != 0:
        logger.info(f"Deleting {archive_ids}")
        delete_archives_from_metadata_db(db_cursor, archive_ids, table_prefix, dataset)

        for target in archive_ids:
            if dataset is not None:
                target = f"{dataset}/{target}"
            targets_buffer.add_target(target)

        targets_buffer.persists_new_targets()
        db_conn.commit()

    remove_targets(archive_output_config, targets_buffer.get_targets())
    targets_buffer.flush()


# Let's first consider the case for only CLP.
def handle_archive_retention(clp_config: CLPConfig) -> None:
    archive_output_config = clp_config.archive_output
    storage_engine = clp_config.package.storage_engine
    archive_expiry_epoch = SECOND_TO_MILLISECOND * get_target_time(
        archive_output_config.retention_period
    )

    logger.info(f"Handler targeting all archives with end_ts < {archive_expiry_epoch}")

    logger.info("Creating SQL adapter")
    sql_adapter = SQL_Adapter(clp_config.database)
    clp_connection_param = clp_config.database.get_clp_connection_params_and_type()
    table_prefix = clp_connection_param["table_prefix"]

    recovery_file = clp_config.logs_directory / "archive_retention.tmp"
    logger.info("Creating target handle")
    targets_buffer = TargetsBuffer(recovery_file)

    logger.info("mysql cursor starts")
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        if StorageEngine.CLP_S == storage_engine:
            datasets = fetch_existing_datasets(db_cursor, table_prefix)
            for dataset in datasets:
                archive_retention_helper(
                    db_conn,
                    db_cursor,
                    table_prefix,
                    archive_expiry_epoch,
                    targets_buffer,
                    archive_output_config,
                    dataset,
                )
        elif StorageEngine.CLP == storage_engine:
            archive_retention_helper(
                db_conn,
                db_cursor,
                table_prefix,
                archive_expiry_epoch,
                targets_buffer,
                archive_output_config,
                None,
            )
        else:
            raise ValueError(f"Unsupported Storage engine: {storage_engine}")


def archive_retention_entry(clp_config: CLPConfig, job_frequency_secs: int) -> None:
    archive_retention_period = clp_config.archive_output.retention_period
    if archive_retention_period is None:
        logger.info("No archive retention period is specified, give up")
        return
    while True:
        handle_archive_retention(clp_config)
        time.sleep(job_frequency_secs)


# For empty collection, we return 0
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


def remove_targets(output_config: Union[ArchiveOutput, StreamOutput], targets: Set[str]) -> None:
    logger.debug(f"Removing: {targets}")
    storage_config = output_config.storage
    storage_type = storage_config.type

    if StorageType.S3 == storage_type:
        s3_delete_objects(storage_config.s3_config, targets)
    elif StorageType.FS == storage_type:
        for target in targets:
            remove_fs_target(storage_config, target)
    else:
        raise ValueError(f"Unsupported Storage type: {storage_type}")


def handle_stream_retention(
    logs_directory: pathlib.Path,
    stream_output_config: StreamOutput,
    results_cache_config: ResultsCache,
) -> None:
    expiry_time = get_target_time(stream_output_config.retention_period)
    expiry_oid = ObjectId.from_datetime(datetime.utcfromtimestamp(expiry_time))

    logger.info(f"Handler targeting all streams < {expiry_time}")
    logger.info(f"Translated to objectID = {expiry_oid}")

    recovery_file = logs_directory / "stream_retention.tmp"
    targets_buffer = TargetsBuffer(recovery_file)

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
            targets_buffer.add_target(stream_path)
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


def stream_retention_entry(clp_config: CLPConfig, job_frequency_secs: int) -> None:
    stream_output_config = clp_config.stream_output
    storage_type = stream_output_config.storage.type

    if StorageType.S3 == storage_type:
        storage_engine = clp_config.package.storage_engine
        if StorageEngine.CLP_S != storage_engine:
            raise ValueError(
                f"{storage_type} is not supported when using storage engine {storage_engine}"
            )
    elif StorageType.FS != storage_type:
        raise ValueError(f"Unsupported Storage type: {storage_type}")

    while True:
        handle_stream_retention(
            clp_config.logs_directory, stream_output_config, clp_config.results_cache
        )
        time.sleep(job_frequency_secs)


def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(description=f"Spin up the {RETENTION_DAEMON_COMPONENT_NAME}.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")

    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / f"{RETENTION_DAEMON_COMPONENT_NAME}.log"
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

    # fmt: off
    archive_retention_period = clp_config.archive_output.retention_period
    logger.info(f"Archive retention period: {archive_retention_period}")
    stream_retention_period = clp_config.stream_output.retention_period
    logger.info(f"Stream retention period: {stream_retention_period}")
    results_cache_retention_period = clp_config.results_cache.retention_period
    logger.info(f"Results cache retention period: {results_cache_retention_period}")

    archive_retention_entry(clp_config, 30)

    logger.info("reducer terminated")

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
