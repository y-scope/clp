import typing

from clp_py_utils.clp_config import (
    ARCHIVE_TAGS_TABLE_SUFFIX,
    ARCHIVES_TABLE_SUFFIX,
    COLUMN_METADATA_TABLE_SUFFIX,
    DATASETS_TABLE_SUFFIX,
    FILES_TABLE_SUFFIX,
    TAGS_TABLE_SUFFIX,
)


def _create_archives_table(db_cursor, table_prefix: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_prefix}{ARCHIVES_TABLE_SUFFIX}` (
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


def _create_tags_table(db_cursor, table_prefix: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_prefix}{TAGS_TABLE_SUFFIX}` (
            `tag_id` INT unsigned NOT NULL AUTO_INCREMENT,
            `tag_name` VARCHAR(255) NOT NULL,
            UNIQUE KEY (`tag_name`) USING BTREE,
            PRIMARY KEY (`tag_id`)
        )
        """
    )


def _create_archive_tags_table(db_cursor, table_prefix: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_prefix}{ARCHIVE_TAGS_TABLE_SUFFIX}` (
            `archive_id` VARCHAR(64) NOT NULL,
            `tag_id` INT unsigned NOT NULL,
            PRIMARY KEY (`archive_id`,`tag_id`),
            FOREIGN KEY (`archive_id`) REFERENCES `{table_prefix}{ARCHIVES_TABLE_SUFFIX}` (`id`),
            FOREIGN KEY (`tag_id`) REFERENCES `{table_prefix}{TAGS_TABLE_SUFFIX}` (`tag_id`)
        )
        """
    )


def _create_files_table(db_cursor, table_prefix: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_prefix}{FILES_TABLE_SUFFIX}` (
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


def _create_column_metadata_table(db_cursor, table_prefix: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_prefix}{COLUMN_METADATA_TABLE_SUFFIX}` (
            `name` VARCHAR(512) NOT NULL,
            `type` TINYINT NOT NULL,
            PRIMARY KEY (`name`, `type`)
        )
        """
    )


def create_datasets_table(db_cursor, table_prefix: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_prefix}{DATASETS_TABLE_SUFFIX}` (
            `name` VARCHAR(255) NOT NULL,
            `archive_storage_directory` VARCHAR(4096) NOT NULL,
            PRIMARY KEY (`name`)
        )
        """
    )


def create_metadata_db_tables(db_cursor, table_prefix: str, dataset: str | None = None) -> None:
    if dataset is not None:
        table_prefix = f"{table_prefix}{dataset}_"
        _create_column_metadata_table(db_cursor, table_prefix)

    _create_archives_table(db_cursor, table_prefix)
    _create_tags_table(db_cursor, table_prefix)
    _create_archive_tags_table(db_cursor, table_prefix)
    _create_files_table(db_cursor, table_prefix)
