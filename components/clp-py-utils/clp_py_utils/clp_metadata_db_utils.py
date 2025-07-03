from __future__ import annotations

from pathlib import Path
from typing import Set

from clp_py_utils.clp_config import (
    ArchiveOutput,
    StorageType,
)

# Constants
ARCHIVE_TAGS_TABLE_SUFFIX = "archive_tags"
ARCHIVES_TABLE_SUFFIX = "archives"
COLUMN_METADATA_TABLE_SUFFIX = "column_metadata"
DATASETS_TABLE_SUFFIX = "datasets"
FILES_TABLE_SUFFIX = "files"
TAGS_TABLE_SUFFIX = "tags"


def _create_archives_table(db_cursor, archives_table_name: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{archives_table_name}` (
            `pagination_id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
            `id` VARCHAR(64) NOT NULL,
            `begin_timestamp` BIGINT NOT NULL,
            `end_timestamp` BIGINT NOT NULL,
            `uncompressed_size` BIGINT NOT NULL,
            `size` BIGINT NOT NULL,
            `creator_id` VARCHAR(64) NOT NULL,
            `creation_ix` INT NOT NULL,
            KEY `archives_creation_order` (`creator_id`,`creation_ix`) USING BTREE,
            UNIQUE KEY `archive_id` (`id`) USING BTREE,
            PRIMARY KEY (`pagination_id`)
        )
        """
    )


def _create_tags_table(db_cursor, tags_table_name: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{tags_table_name}` (
            `tag_id` INT unsigned NOT NULL AUTO_INCREMENT,
            `tag_name` VARCHAR(255) NOT NULL,
            UNIQUE KEY (`tag_name`) USING BTREE,
            PRIMARY KEY (`tag_id`)
        )
        """
    )


def _create_archive_tags_table(
    db_cursor, archive_tags_table_name: str, archives_table_name: str, tags_table_name: str
) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{archive_tags_table_name}` (
            `archive_id` VARCHAR(64) NOT NULL,
            `tag_id` INT unsigned NOT NULL,
            PRIMARY KEY (`archive_id`,`tag_id`),
            FOREIGN KEY (`archive_id`) REFERENCES `{archives_table_name}` (`id`),
            FOREIGN KEY (`tag_id`) REFERENCES `{tags_table_name}` (`tag_id`)
        )
        """
    )


def _create_files_table(db_cursor, table_prefix: str, dataset: str | None) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{get_files_table_name(table_prefix, dataset)}` (
            `id` VARCHAR(64) NOT NULL,
            `orig_file_id` VARCHAR(64) NOT NULL,
            `path` VARCHAR(12288) NOT NULL,
            `begin_timestamp` BIGINT NOT NULL,
            `end_timestamp` BIGINT NOT NULL,
            `num_uncompressed_bytes` BIGINT NOT NULL,
            `begin_message_ix` BIGINT NOT NULL,
            `num_messages` BIGINT NOT NULL,
            `archive_id` VARCHAR(64) NOT NULL,
            KEY `files_path` (path(768)) USING BTREE,
            KEY `files_archive_id` (`archive_id`) USING BTREE,
            PRIMARY KEY (`id`)
        ) ROW_FORMAT=DYNAMIC
        """
    )


def _create_column_metadata_table(db_cursor, table_prefix: str, dataset: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{get_column_metadata_table_name(table_prefix, dataset)}` (
            `name` VARCHAR(512) NOT NULL,
            `type` TINYINT NOT NULL,
            PRIMARY KEY (`name`, `type`)
        )
        """
    )


def _get_table_name(prefix: str, suffix: str, dataset: str | None) -> str:
    """
    :param prefix:
    :param suffix:
    :param dataset:
    :return: The table name in the form of "<prefix>[<dataset>_]<suffix>".
    """
    table_name = prefix
    if dataset is not None:
        table_name += f"{dataset}_"
    table_name += suffix
    return table_name


def create_datasets_table(db_cursor, table_prefix: str) -> None:
    """
    Creates the datasets information table.

    :param db_cursor: The database cursor to execute the table creation.
    :param table_prefix: A string to prepend to the table name.
    """

    # For a description of the table, see
    # `../../../docs/src/dev-guide/design-metadata-db.md`
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{get_datasets_table_name(table_prefix)}` (
            `name` VARCHAR(255) NOT NULL,
            `archive_storage_directory` VARCHAR(4096) NOT NULL,
            PRIMARY KEY (`name`)
        )
        """
    )


def add_dataset(
    db_conn,
    db_cursor,
    table_prefix: str,
    dataset_name: str,
    archive_output: ArchiveOutput,
) -> None:
    """
    Inserts a new dataset into the `datasets` table and creates the corresponding standard set of
    tables for CLP's metadata.

    :param db_conn:
    :param db_cursor: The database cursor to execute the table row insertion.
    :param table_prefix: A string to prepend to the table name.
    :param dataset_name:
    :param archive_output:
    """
    archive_storage_directory: Path
    if StorageType.S3 == archive_output.storage.type:
        s3_config = archive_output.storage.s3_config
        archive_storage_directory = Path(s3_config.key_prefix)
    else:
        archive_storage_directory = archive_output.get_directory()

    query = f"""INSERT INTO `{get_datasets_table_name(table_prefix)}`
                (name, archive_storage_directory)
                VALUES (%s, %s)
                """
    db_cursor.execute(
        query,
        (dataset_name, str(archive_storage_directory / dataset_name)),
    )
    create_metadata_db_tables(db_cursor, table_prefix, dataset_name)
    db_conn.commit()


def fetch_existing_datasets(
    db_cursor,
    table_prefix: str,
) -> Set[str]:
    """
    Gets the names of all existing datasets.

    :param db_cursor:
    :param table_prefix:
    """
    db_cursor.execute(f"SELECT name FROM `{get_datasets_table_name(table_prefix)}`")
    rows = db_cursor.fetchall()
    return {row["name"] for row in rows}


def create_metadata_db_tables(db_cursor, table_prefix: str, dataset: str | None = None) -> None:
    """
    Creates the standard set of tables for CLP's metadata.

    :param db_cursor: The database cursor to execute the table creations.
    :param table_prefix: A string to prepend to all table names.
    :param dataset: If set, all tables will be named in a dataset-specific manner.
    """
    if dataset is not None:
        _create_column_metadata_table(db_cursor, table_prefix, dataset)

    archives_table_name = get_archives_table_name(table_prefix, dataset)
    tags_table_name = get_tags_table_name(table_prefix, dataset)
    archive_tags_table_name = get_archive_tags_table_name(table_prefix, dataset)

    _create_archives_table(db_cursor, archives_table_name)
    _create_tags_table(db_cursor, tags_table_name)
    _create_archive_tags_table(
        db_cursor, archive_tags_table_name, archives_table_name, tags_table_name
    )
    _create_files_table(db_cursor, table_prefix, dataset)


def get_archive_tags_table_name(table_prefix: str, dataset: str | None) -> str:
    return _get_table_name(table_prefix, ARCHIVE_TAGS_TABLE_SUFFIX, dataset)


def get_archives_table_name(table_prefix: str, dataset: str | None) -> str:
    return _get_table_name(table_prefix, ARCHIVES_TABLE_SUFFIX, dataset)


def get_column_metadata_table_name(table_prefix: str, dataset: str | None) -> str:
    return _get_table_name(table_prefix, COLUMN_METADATA_TABLE_SUFFIX, dataset)


def get_datasets_table_name(table_prefix: str) -> str:
    return _get_table_name(table_prefix, DATASETS_TABLE_SUFFIX, None)


def get_files_table_name(table_prefix: str, dataset: str | None) -> str:
    return _get_table_name(table_prefix, FILES_TABLE_SUFFIX, dataset)


def get_tags_table_name(table_prefix: str, dataset: str | None) -> str:
    return _get_table_name(table_prefix, TAGS_TABLE_SUFFIX, dataset)
