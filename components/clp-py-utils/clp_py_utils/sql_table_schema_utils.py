import typing

from clp_py_utils.clp_config import (
    ARCHIVE_TAGS_TABLE_SUFFIX,
    ARCHIVES_TABLE_SUFFIX,
    COLUMN_METADATA_TABLE_SUFFIX,
    FILES_TABLE_SUFFIX,
    TAGS_TABLE_SUFFIX,
)


def create_archives_table(db_cursor, table_name: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_name}` (
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


def create_tags_table(db_cursor, table_name: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_name}` (
            `tag_id` INT unsigned NOT NULL AUTO_INCREMENT,
            `tag_name` VARCHAR(255) NOT NULL,
            UNIQUE KEY (`tag_name`) USING BTREE,
            PRIMARY KEY (`tag_id`)
        )
        """
    )


def create_archive_tags_table(
    db_cursor, table_name: str, archives_table_name: str, tags_table_name: str
) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_name}` (
            `archive_id` VARCHAR(64) NOT NULL,
            `tag_id` INT unsigned NOT NULL,
            PRIMARY KEY (`archive_id`,`tag_id`),
            FOREIGN KEY (`archive_id`) REFERENCES `{archives_table_name}` (`id`),
            FOREIGN KEY (`tag_id`) REFERENCES `{tags_table_name}` (`tag_id`)
        )
        """
    )


def create_files_table(db_cursor, table_name: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_name}` (
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


def create_datasets_table(db_cursor, table_name: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_name}` (
            `name` VARCHAR(255) NOT NULL,
            `archive_storage_directory` VARCHAR(4096) NOT NULL,
            PRIMARY KEY (`name`)
        )
        """
    )


def create_column_metadata_table(db_cursor, table_name: str) -> None:
    db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_name}` (
            `name` VARCHAR(512) NOT NULL,
            `type` TINYINT NOT NULL,
            PRIMARY KEY (`name`, `type`)
        )
        """
    )


def create_metadata_db_tables(
    db_cursor, table_prefix: str, dataset: typing.Optional[str] = None
) -> None:
    if dataset is not None:
        table_prefix = f"{table_prefix}{dataset}_"
        create_column_metadata_table(db_cursor, f"{table_prefix}{COLUMN_METADATA_TABLE_SUFFIX}")

    archives_table_name = f"{table_prefix}{ARCHIVES_TABLE_SUFFIX}"
    tags_table_name = f"{table_prefix}{TAGS_TABLE_SUFFIX}"
    archive_tags_table_name = f"{table_prefix}{ARCHIVE_TAGS_TABLE_SUFFIX}"
    files_table_name = f"{table_prefix}{FILES_TABLE_SUFFIX}"

    create_archives_table(db_cursor, archives_table_name)
    create_tags_table(db_cursor, tags_table_name)
    create_archive_tags_table(
        db_cursor, archive_tags_table_name, archives_table_name, tags_table_name
    )
    create_files_table(db_cursor, files_table_name)
