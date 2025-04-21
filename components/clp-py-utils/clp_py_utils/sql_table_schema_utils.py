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


def create_archive_tags_table(db_cursor, table_name: str) -> None:
    metadata_db_cursor.execute(
        f"""
        CREATE TABLE IF NOT EXISTS `{table_name}` (
            `archive_id` VARCHAR(64) NOT NULL,
            `tag_id` INT unsigned NOT NULL,
            PRIMARY KEY (`archive_id`,`tag_id`),
            FOREIGN KEY (`archive_id`) REFERENCES `{table_prefix}archives` (`id`),
            FOREIGN KEY (`tag_id`) REFERENCES `{table_prefix}tags` (`tag_id`)
        )
        """
    )


def create_files_table(db_cursor, table_name: str) -> None:
    metadata_db_cursor.execute(
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
            `name` VARCHAR(512) NOT NULL,
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
