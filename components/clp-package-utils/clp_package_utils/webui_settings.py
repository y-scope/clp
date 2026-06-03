"""Utilities for updating WebUI settings files in CLP package deployments."""

import json
import pathlib
from typing import Any, cast

from clp_py_utils.clp_config import (
    AwsAuthType,
    CLP_METADATA_TABLE_PREFIX,
    ClpConfig,
    ClpDbNameType,
    COMPRESSION_JOBS_TABLE_NAME,
    CONTAINER_INPUT_LOGS_ROOT_DIR,
    QUERY_JOBS_TABLE_NAME,
    QueryEngine,
    StorageEngine,
    StorageType,
    StreamS3Storage,
)
from clp_py_utils.clp_metadata_db_utils import (
    get_archives_table_name,
    get_datasets_table_name,
    get_files_table_name,
)


def _update_settings_object(
    parent_key_prefix: str,
    settings: dict[str, Any],
    updates: dict[str, Any],
) -> None:
    """
    Recursively updates the given settings object with the values from `updates`.

    :param parent_key_prefix: The prefix for keys at this level in the settings dictionary.
    :param settings: The settings to update.
    :param updates: The updates.
    :raise ValueError: If a key in `updates` doesn't exist in `settings`.
    """
    for key, value in updates.items():
        if key not in settings:
            error_msg = f"{parent_key_prefix}{key} is not a valid configuration key for the webui."
            raise ValueError(error_msg)
        if isinstance(value, dict):
            _update_settings_object(f"{parent_key_prefix}{key}.", settings[key], value)
        else:
            settings[key] = value


def _read_and_update_settings_json(
    settings_file_path: pathlib.Path, updates: dict[str, Any]
) -> dict[str, Any]:
    """
    Reads and updates a settings JSON file.

    :param settings_file_path:
    :param updates:
    """
    settings_object = cast("dict[str, Any]", json.loads(settings_file_path.read_text()))
    _update_settings_object("", settings_object, updates)

    return settings_object


def update_webui_settings(
    clp_config: ClpConfig,
    container_clp_config: ClpConfig,
    client_settings_json_path: pathlib.Path,
    server_settings_json_path: pathlib.Path,
    container_webui_dir: pathlib.Path,
) -> None:
    """
    Updates the WebUI client and server settings files for a package deployment.

    :param clp_config:
    :param container_clp_config:
    :param client_settings_json_path:
    :param server_settings_json_path:
    :param container_webui_dir:
    """
    table_prefix = CLP_METADATA_TABLE_PREFIX
    if StorageEngine.CLP_S == clp_config.package.storage_engine:
        archives_table_name = ""
        files_table_name = ""
    else:
        archives_table_name = get_archives_table_name(table_prefix, None)
        files_table_name = get_files_table_name(table_prefix, None)

    client_settings_json_updates = {
        "ClpStorageEngine": clp_config.package.storage_engine,
        "ClpQueryEngine": clp_config.package.query_engine,
        "LogsInputType": clp_config.logs_input.type,
        "MaxDatasetsPerQuery": clp_config.query_scheduler.max_datasets_per_query,
        "MongoDbSearchResultsMetadataCollectionName": (
            clp_config.webui.results_metadata_collection_name
        ),
        "SqlDbClpArchivesTableName": archives_table_name,
        "SqlDbClpDatasetsTableName": get_datasets_table_name(table_prefix),
        "SqlDbClpFilesTableName": files_table_name,
        "SqlDbClpTablePrefix": table_prefix,
        "SqlDbCompressionJobsTableName": COMPRESSION_JOBS_TABLE_NAME,
    }

    server_settings_json_updates = {
        "SqlDbHost": container_clp_config.database.host,
        "SqlDbPort": container_clp_config.database.port,
        "SqlDbName": clp_config.database.names[ClpDbNameType.CLP],
        "SqlDbQueryJobsTableName": QUERY_JOBS_TABLE_NAME,
        "SqlDbCompressionJobsTableName": COMPRESSION_JOBS_TABLE_NAME,
        "MongoDbHost": container_clp_config.results_cache.host,
        "MongoDbPort": container_clp_config.results_cache.port,
        "MongoDbName": clp_config.results_cache.db_name,
        "MongoDbSearchResultsMetadataCollectionName": (
            clp_config.webui.results_metadata_collection_name
        ),
        "MongoDbStreamFilesCollectionName": clp_config.results_cache.stream_collection_name,
        "ClientDir": str(container_webui_dir / "client"),
        "LogViewerDir": str(container_webui_dir / "yscope-log-viewer"),
        "StreamTargetUncompressedSize": clp_config.stream_output.target_uncompressed_size,
        "ArchiveOutputCompressionLevel": clp_config.archive_output.compression_level,
        "ArchiveOutputTargetArchiveSize": clp_config.archive_output.target_archive_size,
        "ArchiveOutputTargetDictionariesSize": (clp_config.archive_output.target_dictionaries_size),
        "ArchiveOutputTargetEncodedFileSize": clp_config.archive_output.target_encoded_file_size,
        "ArchiveOutputTargetSegmentSize": clp_config.archive_output.target_segment_size,
        "ClpQueryEngine": clp_config.package.query_engine,
        "ClpStorageEngine": clp_config.package.storage_engine,
    }

    stream_storage = clp_config.stream_output.storage
    if StorageType.S3 == stream_storage.type:
        stream_storage = cast("StreamS3Storage", stream_storage)
        s3_config = stream_storage.s3_config
        server_settings_json_updates["StreamFilesDir"] = None
        server_settings_json_updates["StreamFilesS3Region"] = s3_config.region_code
        server_settings_json_updates["StreamFilesS3PathPrefix"] = (
            f"{s3_config.bucket}/{s3_config.key_prefix}"
        )
        auth = s3_config.aws_authentication
        if AwsAuthType.profile == auth.type:
            server_settings_json_updates["StreamFilesS3Profile"] = auth.profile
        else:
            server_settings_json_updates["StreamFilesS3Profile"] = None
    elif StorageType.FS == stream_storage.type:
        server_settings_json_updates["StreamFilesDir"] = str(
            container_clp_config.stream_output.get_directory()
        )
        server_settings_json_updates["StreamFilesS3Region"] = None
        server_settings_json_updates["StreamFilesS3PathPrefix"] = None
        server_settings_json_updates["StreamFilesS3Profile"] = None

    query_engine = clp_config.package.query_engine
    if QueryEngine.PRESTO == query_engine:
        if container_clp_config.presto is None:
            msg = "presto config is required when query_engine is presto."
            raise ValueError(msg)
        server_settings_json_updates["PrestoHost"] = container_clp_config.presto.host
        server_settings_json_updates["PrestoPort"] = container_clp_config.presto.port
    else:
        server_settings_json_updates["PrestoHost"] = None
        server_settings_json_updates["PrestoPort"] = None

    if StorageType.FS == clp_config.logs_input.type:
        client_settings_json_updates["LogsInputRootDir"] = str(CONTAINER_INPUT_LOGS_ROOT_DIR)
        server_settings_json_updates["LogsInputRootDir"] = str(CONTAINER_INPUT_LOGS_ROOT_DIR)
    else:
        client_settings_json_updates["LogsInputRootDir"] = None
        server_settings_json_updates["LogsInputRootDir"] = None

    client_settings_json = _read_and_update_settings_json(
        client_settings_json_path, client_settings_json_updates
    )
    client_settings_json_path.write_text(json.dumps(client_settings_json))

    server_settings_json = _read_and_update_settings_json(
        server_settings_json_path, server_settings_json_updates
    )
    server_settings_json_path.write_text(json.dumps(server_settings_json))
