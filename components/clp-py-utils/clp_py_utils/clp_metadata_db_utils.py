from __future__ import annotations

from pathlib import Path

from clp_py_utils.clp_config import ArchiveOutput, StorageType

# Constants
MYSQL_TABLE_NAME_MAX_LEN = 64

ARCHIVES_TABLE_SUFFIX = "archives"
ARCHIVE_STORAGE_TOTALS_TABLE_SUFFIX = "archive_totals"
COLUMN_METADATA_TABLE_SUFFIX = "column_metadata"
DATASETS_TABLE_SUFFIX = "datasets"
FILES_TABLE_SUFFIX = "files"

TABLE_SUFFIX_MAX_LEN = max(
    len(ARCHIVES_TABLE_SUFFIX),
    len(COLUMN_METADATA_TABLE_SUFFIX),
    len(DATASETS_TABLE_SUFFIX),
    len(FILES_TABLE_SUFFIX),
)


_FNV1A_64_OFFSET_BASIS = 0xCBF29CE484222325
_FNV1A_64_PRIME = 0x100000001B3
_FNV1A_64_MASK = 0xFFFFFFFFFFFFFFFF


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


def create_archive_storage_totals_table(db_cursor, table_prefix: str) -> None:
    """Creates the shared archive storage totals table."""
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{get_archive_storage_totals_table_name(table_prefix)}` (
            `dataset` VARCHAR(255) NOT NULL,
            `archive_count` BIGINT UNSIGNED NOT NULL,
            `compressed_size` BIGINT NOT NULL,
            `uncompressed_size` BIGINT NOT NULL,
            PRIMARY KEY (`dataset`)
        )
        """
    )


def _get_fnv1a_64_hex(value: str) -> str:
    hash_value = _FNV1A_64_OFFSET_BASIS
    for byte in value.encode():
        hash_value ^= byte
        hash_value = (hash_value * _FNV1A_64_PRIME) & _FNV1A_64_MASK
    return f"{hash_value:016x}"


def get_archive_storage_totals_trigger_names(archives_table_name: str) -> tuple[str, str, str]:
    """Returns the insert, update, and delete trigger names for an archives table."""
    trigger_name_suffix = _get_fnv1a_64_hex(archives_table_name)
    return (
        f"archive_storage_ai_{trigger_name_suffix}",
        f"archive_storage_au_{trigger_name_suffix}",
        f"archive_storage_ad_{trigger_name_suffix}",
    )


def _get_archive_storage_totals_lock_name(archives_table_name: str) -> str:
    return f"archive_storage_totals_{_get_fnv1a_64_hex(archives_table_name)}"


def _get_existing_archive_storage_totals_triggers(
    db_cursor, archives_table_name: str, trigger_names: tuple[str, str, str]
) -> set[str]:
    db_cursor.execute(
        """
        SELECT TRIGGER_NAME
        FROM information_schema.TRIGGERS
        WHERE TRIGGER_SCHEMA = DATABASE()
          AND EVENT_OBJECT_TABLE = %s
          AND TRIGGER_NAME IN (%s, %s, %s)
        """,
        (archives_table_name, *trigger_names),
    )
    return {row["TRIGGER_NAME"] for row in db_cursor.fetchall()}


def _create_archive_storage_totals_triggers(
    db_cursor, table_prefix: str, dataset: str | None, existing_trigger_names: set[str]
) -> bool:
    archives_table_name = get_archives_table_name(table_prefix, dataset)
    totals_table_name = get_archive_storage_totals_table_name(table_prefix)
    insert_trigger_name, update_trigger_name, delete_trigger_name = (
        get_archive_storage_totals_trigger_names(archives_table_name)
    )
    dataset_sql_literal = "''" if dataset is None else "'" + dataset.replace("'", "''") + "'"
    trigger_definitions = (
        (
            insert_trigger_name,
            f"""
            AFTER INSERT ON `{archives_table_name}`
            FOR EACH ROW
            INSERT INTO `{totals_table_name}`
                (`dataset`, `archive_count`, `compressed_size`, `uncompressed_size`)
            VALUES ({dataset_sql_literal}, 1, NEW.`size`, NEW.`uncompressed_size`)
            ON DUPLICATE KEY UPDATE
                `archive_count` = `archive_count` + 1,
                `compressed_size` = `compressed_size` + NEW.`size`,
                `uncompressed_size` = `uncompressed_size` + NEW.`uncompressed_size`
            """,
        ),
        (
            update_trigger_name,
            f"""
            AFTER UPDATE ON `{archives_table_name}`
            FOR EACH ROW
            UPDATE `{totals_table_name}`
            SET `compressed_size` = `compressed_size` + NEW.`size` - OLD.`size`,
                `uncompressed_size` = `uncompressed_size`
                    + NEW.`uncompressed_size` - OLD.`uncompressed_size`
            WHERE `dataset` = {dataset_sql_literal}
            """,
        ),
        (
            delete_trigger_name,
            f"""
            AFTER DELETE ON `{archives_table_name}`
            FOR EACH ROW
            UPDATE `{totals_table_name}`
            SET `archive_count` = `archive_count` - 1,
                `compressed_size` = `compressed_size` - OLD.`size`,
                `uncompressed_size` = `uncompressed_size` - OLD.`uncompressed_size`
            WHERE `dataset` = {dataset_sql_literal}
            """,
        ),
    )

    created_trigger = False
    for trigger_name, trigger_definition in trigger_definitions:
        if trigger_name not in existing_trigger_names:
            db_cursor.execute(f"CREATE TRIGGER `{trigger_name}` {trigger_definition}")
            created_trigger = True
    return created_trigger


def reconcile_archive_storage_totals(
    db_cursor, table_prefix: str, dataset: str | None = None
) -> None:
    """Atomically replaces a scope's archive storage totals with an archive-table aggregate."""
    archives_table_name = get_archives_table_name(table_prefix, dataset)
    totals_table_name = get_archive_storage_totals_table_name(table_prefix)
    dataset_key = "" if dataset is None else dataset

    db_cursor.execute(f"LOCK TABLES `{archives_table_name}` WRITE, `{totals_table_name}` WRITE")
    try:
        db_cursor.execute(
            f"""
            SELECT COUNT(*) AS archive_count,
                   COALESCE(SUM(size), 0) AS compressed_size,
                   COALESCE(SUM(uncompressed_size), 0) AS uncompressed_size
            FROM `{archives_table_name}`
            """
        )
        totals = db_cursor.fetchone()
        db_cursor.execute(
            f"""
            REPLACE INTO `{totals_table_name}`
                (`dataset`, `archive_count`, `compressed_size`, `uncompressed_size`)
            VALUES (%s, %s, %s, %s)
            """,
            (
                dataset_key,
                totals["archive_count"],
                totals["compressed_size"],
                totals["uncompressed_size"],
            ),
        )
    finally:
        db_cursor.execute("UNLOCK TABLES")


def _archive_storage_totals_row_exists(db_cursor, table_prefix: str, dataset_key: str) -> bool:
    db_cursor.execute(
        f"""
        SELECT 1
        FROM `{get_archive_storage_totals_table_name(table_prefix)}`
        WHERE dataset = %s
        """,
        (dataset_key,),
    )
    return db_cursor.fetchone() is not None


def _acquire_archive_storage_totals_lock(db_cursor, lock_name: str) -> None:
    db_cursor.execute("SELECT GET_LOCK(%s, 60) AS acquired", (lock_name,))
    if db_cursor.fetchone()["acquired"] != 1:
        raise RuntimeError(f"Timed out acquiring archive storage totals lock '{lock_name}'.")


def _release_archive_storage_totals_lock(db_cursor, lock_name: str) -> None:
    db_cursor.execute("SELECT RELEASE_LOCK(%s) AS released", (lock_name,))
    if db_cursor.fetchone()["released"] != 1:
        raise RuntimeError(f"Failed to release archive storage totals lock '{lock_name}'.")


def ensure_archive_storage_totals(db_cursor, table_prefix: str, dataset: str | None = None) -> None:
    """Ensures archive storage totals triggers and the corresponding scope total exist."""
    create_archive_storage_totals_table(db_cursor, table_prefix)
    archives_table_name = get_archives_table_name(table_prefix, dataset)
    trigger_names = get_archive_storage_totals_trigger_names(archives_table_name)
    dataset_key = "" if dataset is None else dataset
    lock_name = _get_archive_storage_totals_lock_name(archives_table_name)

    _acquire_archive_storage_totals_lock(db_cursor, lock_name)
    try:
        existing_trigger_names = _get_existing_archive_storage_totals_triggers(
            db_cursor, archives_table_name, trigger_names
        )
        created_trigger = _create_archive_storage_totals_triggers(
            db_cursor, table_prefix, dataset, existing_trigger_names
        )
        if created_trigger or not _archive_storage_totals_row_exists(
            db_cursor, table_prefix, dataset_key
        ):
            reconcile_archive_storage_totals(db_cursor, table_prefix, dataset)
    finally:
        _release_archive_storage_totals_lock(db_cursor, lock_name)


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
    # `../../../docs/src/dev-docs/design-metadata-db.md`
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
) -> set[str]:
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

    _create_archives_table(db_cursor, archives_table_name)
    _create_files_table(db_cursor, table_prefix, dataset)
    ensure_archive_storage_totals(db_cursor, table_prefix, dataset)


def delete_archives_from_metadata_db(
    db_cursor, archive_ids: list[str], table_prefix: str, dataset: str | None
) -> None:
    """
    Deletes archives from the metadata database specified by a list of IDs. It also deletes
    the associated entries from the `files` table that reference these archives.

    The order of deletion follows the foreign key constraints, ensuring no violations occur during
    the process.

    :param db_cursor:
    :param archive_ids: The list of archive to delete.
    :param table_prefix:
    :param dataset:
    """
    ids_list_string = ", ".join(["%s"] * len(archive_ids))

    db_cursor.execute(
        f"""
        DELETE FROM `{get_files_table_name(table_prefix, dataset)}`
        WHERE archive_id in ({ids_list_string})
        """,
        archive_ids,
    )

    db_cursor.execute(
        f"""
        DELETE FROM `{get_archives_table_name(table_prefix, dataset)}`
        WHERE id in ({ids_list_string})
        """,
        archive_ids,
    )


def delete_dataset_from_metadata_db(db_cursor, table_prefix: str, dataset: str) -> None:
    """
    Deletes all tables associated with `dataset` from the metadata database.

    :param db_cursor:
    :param table_prefix:
    :param dataset:
    """
    archives_table_name = get_archives_table_name(table_prefix, dataset)
    lock_name = _get_archive_storage_totals_lock_name(archives_table_name)
    _acquire_archive_storage_totals_lock(db_cursor, lock_name)
    try:
        # Drop tables in an order such that no foreign key constraint is violated.
        tables_in_removal_order = [
            get_column_metadata_table_name(table_prefix, dataset),
            get_files_table_name(table_prefix, dataset),
            archives_table_name,
        ]

        for table in tables_in_removal_order:
            db_cursor.execute(f"DROP TABLE IF EXISTS `{table}`")

        # Dropping the archives table does not execute its row triggers. Delete the shared total
        # after the table is gone so no archive insert trigger can recreate it during deletion.
        db_cursor.execute(
            f"""
            DELETE FROM `{get_archive_storage_totals_table_name(table_prefix)}`
            WHERE dataset = %s
            """,
            (dataset,),
        )

        # Remove the dataset row from the datasets table
        db_cursor.execute(
            f"""
            DELETE FROM `{get_datasets_table_name(table_prefix)}`
            WHERE name = %s
            """,
            (dataset,),
        )
    finally:
        _release_archive_storage_totals_lock(db_cursor, lock_name)


def get_archive_storage_totals_table_name(table_prefix: str) -> str:
    return _get_table_name(table_prefix, ARCHIVE_STORAGE_TOTALS_TABLE_SUFFIX, None)


def get_archives_table_name(table_prefix: str, dataset: str | None) -> str:
    return _get_table_name(table_prefix, ARCHIVES_TABLE_SUFFIX, dataset)


def get_column_metadata_table_name(table_prefix: str, dataset: str | None) -> str:
    return _get_table_name(table_prefix, COLUMN_METADATA_TABLE_SUFFIX, dataset)


def get_datasets_table_name(table_prefix: str) -> str:
    return _get_table_name(table_prefix, DATASETS_TABLE_SUFFIX, None)


def get_files_table_name(table_prefix: str, dataset: str | None) -> str:
    return _get_table_name(table_prefix, FILES_TABLE_SUFFIX, dataset)
