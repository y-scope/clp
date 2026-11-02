from __future__ import annotations

from pathlib import Path

from clp_py_utils.clp_config import ArchiveOutput, StorageType

# Constants
DATASET_NAME_MAX_LEN = 255

# An archive whose timestamp range is wider than this is also recorded in the long-span archives
# table, so that time-ranged queries can still find it after the archives table is partitioned.
# TODO: Make this configurable.
LONG_SPAN_THRESHOLD_MILLIS = 24 * 60 * 60 * 1000

ARCHIVES_TABLE_SUFFIX = "archives"
COLUMN_METADATA_TABLE_SUFFIX = "column_metadata"
DATASETS_TABLE_SUFFIX = "datasets"
FILES_TABLE_SUFFIX = "files"
LONG_SPAN_ARCHIVES_TABLE_SUFFIX = "long_span_archives"


def _create_archives_table(db_cursor, archives_table_name: str) -> None:
    # NOTE: `archive_uuid` can't be a unique key, since MySQL requires every unique key of a
    # partitioned table to contain the partitioning column.
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{archives_table_name}` (
            `id` BIGINT unsigned NOT NULL AUTO_INCREMENT,
            `dataset_id` SMALLINT unsigned NOT NULL,
            `uuid` BINARY(16) NOT NULL,
            `timestamp_range_begin_millis` BIGINT NOT NULL,
            `timestamp_range_end_millis` BIGINT NOT NULL,
            `num_uncompressed_bytes` BIGINT unsigned NOT NULL,
            `num_compressed_bytes` BIGINT unsigned NOT NULL,
            `creation_time_millis` BIGINT NOT NULL,
            `pack_id` BIGINT unsigned DEFAULT NULL,
            `is_deleted` BOOLEAN NOT NULL DEFAULT FALSE,
            PRIMARY KEY (`dataset_id`, `timestamp_range_begin_millis`, `id`),
            KEY `auto_inc` (`id`),
            KEY `archive_uuid` (`dataset_id`, `uuid`),
            KEY `archives_gc_order` (`dataset_id`, `creation_time_millis`)
        ) ENGINE=InnoDB ROW_FORMAT=COMPRESSED KEY_BLOCK_SIZE=8
        PARTITION BY RANGE (`timestamp_range_begin_millis`) (
            PARTITION `p_future` VALUES LESS THAN MAXVALUE
        )
        """
    )


def _create_long_span_archives_table(db_cursor, table_prefix: str) -> None:
    # NOTE: This table holds a second reference to every archive whose timestamp range exceeds the
    # span contract, so it's deliberately unpartitioned.
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{get_long_span_archives_table_name(table_prefix)}` (
            `dataset_id` SMALLINT unsigned NOT NULL,
            `timestamp_range_begin_millis` BIGINT NOT NULL,
            `timestamp_range_end_millis` BIGINT NOT NULL,
            `archive_id` BIGINT unsigned NOT NULL,
            PRIMARY KEY (`dataset_id`, `timestamp_range_end_millis`, `archive_id`),
            KEY `by_begin` (`dataset_id`, `timestamp_range_begin_millis`)
        ) ENGINE=InnoDB
        """
    )


def _create_column_metadata_table(db_cursor, table_prefix: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{get_column_metadata_table_name(table_prefix)}` (
            `dataset_id` SMALLINT unsigned NOT NULL,
            `name` VARCHAR(512) NOT NULL,
            `type` TINYINT NOT NULL,
            PRIMARY KEY (`dataset_id`, `name`, `type`)
        )
        """
    )


def _get_table_name(prefix: str, suffix: str) -> str:
    """
    :param prefix:
    :param suffix:
    :return: The table name in the form of "<prefix><suffix>".
    """
    return prefix + suffix


def create_datasets_table(db_cursor, table_prefix: str) -> None:
    """
    Creates the datasets information table.

    :param db_cursor: The database cursor to execute the table creation.
    :param table_prefix: A string to prepend to the table name.
    """
    # For a description of the table, see
    # `../../../docs/src/dev-docs/design-metadata-db.md`
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{get_datasets_table_name(table_prefix)}` (
            `id` SMALLINT unsigned NOT NULL AUTO_INCREMENT,
            `name` VARCHAR(255) NOT NULL,
            `archive_storage_path` VARCHAR(4096) NOT NULL,
            `retention_period_minutes` INT unsigned DEFAULT NULL,
            `is_deleted` BOOLEAN NOT NULL DEFAULT FALSE,
            UNIQUE KEY `dataset_name` (`name`) USING BTREE,
            PRIMARY KEY (`id`)
        )
        """
    )


def add_dataset(
    db_conn,
    db_cursor,
    table_prefix: str,
    dataset_name: str,
    archive_output: ArchiveOutput,
) -> int:
    """
    Inserts a new dataset into the `datasets` table.

    :param db_conn:
    :param db_cursor: The database cursor to execute the table row insertion.
    :param table_prefix: A string to prepend to the table name.
    :param dataset_name:
    :param archive_output:
    :return: The ID of the newly inserted dataset.
    """
    archive_storage_directory: Path
    if StorageType.S3 == archive_output.storage.type:
        s3_config = archive_output.storage.s3_config
        archive_storage_directory = Path(s3_config.key_prefix)
    else:
        archive_storage_directory = archive_output.get_directory()

    query = f"""INSERT INTO `{get_datasets_table_name(table_prefix)}`
                (name, archive_storage_path)
                VALUES (%s, %s)
                """
    db_cursor.execute(
        query,
        (dataset_name, str(archive_storage_directory / dataset_name)),
    )
    # NOTE: `lastrowid` is only valid until the next statement runs on this cursor.
    dataset_id = int(db_cursor.lastrowid)
    db_conn.commit()

    return dataset_id


def fetch_existing_datasets(
    db_cursor,
    table_prefix: str,
) -> dict[str, int]:
    """
    Gets the names and IDs of all existing datasets.

    :param db_cursor:
    :param table_prefix:
    :return: A map of each dataset's name to its ID.
    """
    db_cursor.execute(
        f"""
        SELECT id, name FROM `{get_datasets_table_name(table_prefix)}`
        WHERE is_deleted = FALSE
        """
    )
    rows = db_cursor.fetchall()
    return {row["name"]: row["id"] for row in rows}


def create_metadata_db_tables(db_cursor, table_prefix: str) -> None:
    """
    Creates the standard set of tables for CLP's metadata.

    The tables are shared by every dataset, so they only need to be created once.

    :param db_cursor: The database cursor to execute the table creations.
    :param table_prefix: A string to prepend to all table names.
    """
    archives_table_name = get_archives_table_name(table_prefix)

    _create_archives_table(db_cursor, archives_table_name)
    _create_long_span_archives_table(db_cursor, table_prefix)
    _create_column_metadata_table(db_cursor, table_prefix)


def delete_archives_from_metadata_db(
    db_cursor, archive_ids: list[int], table_prefix: str, dataset_id: int
) -> None:
    """
    Deletes archives from the metadata database specified by a list of IDs. It also deletes the
    associated entries from the `long_span_archives` table that reference these archives.

    The order of deletion follows the foreign key constraints, ensuring no violations occur during
    the process.

    :param db_cursor:
    :param archive_ids: The list of archive to delete.
    :param table_prefix:
    :param dataset_id:
    """
    if 0 == len(archive_ids):
        return

    ids_list_string = ", ".join(["%s"] * len(archive_ids))
    params = [dataset_id, *archive_ids]

    db_cursor.execute(
        f"""
        DELETE FROM `{get_long_span_archives_table_name(table_prefix)}`
        WHERE dataset_id = %s AND archive_id in ({ids_list_string})
        """,
        params,
    )

    db_cursor.execute(
        f"""
        DELETE FROM `{get_archives_table_name(table_prefix)}`
        WHERE dataset_id = %s AND id in ({ids_list_string})
        """,
        params,
    )


def delete_dataset_from_metadata_db(db_cursor, table_prefix: str, dataset: str) -> None:
    """
    Marks `dataset` as deleted in the metadata database.

    The dataset's rows in the other metadata tables are left for the garbage collector to reclaim,
    and the dataset continues to occupy its name.

    :param db_cursor:
    :param table_prefix:
    :param dataset:
    """
    db_cursor.execute(
        f"""
        UPDATE `{get_datasets_table_name(table_prefix)}`
        SET is_deleted = TRUE
        WHERE name = %s
        """,
        (dataset,),
    )


def get_archives_table_name(table_prefix: str) -> str:
    return _get_table_name(table_prefix, ARCHIVES_TABLE_SUFFIX)


def get_column_metadata_table_name(table_prefix: str) -> str:
    return _get_table_name(table_prefix, COLUMN_METADATA_TABLE_SUFFIX)


def get_datasets_table_name(table_prefix: str) -> str:
    return _get_table_name(table_prefix, DATASETS_TABLE_SUFFIX)


def get_long_span_archives_table_name(table_prefix: str) -> str:
    return _get_table_name(table_prefix, LONG_SPAN_ARCHIVES_TABLE_SUFFIX)


def get_files_table_name(table_prefix: str) -> str:
    # TODO: The files tables aren't created yet since their schema is still being settled.
    return _get_table_name(table_prefix, FILES_TABLE_SUFFIX)
