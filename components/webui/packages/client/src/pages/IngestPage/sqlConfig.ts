/**
 * Column names for the `clp_archives` table.
 */
enum CLP_ARCHIVES_TABLE_COLUMN_NAMES {
    BEGIN_TIMESTAMP = "begin_timestamp",
    END_TIMESTAMP = "end_timestamp",
    UNCOMPRESSED_SIZE = "uncompressed_size",
    SIZE = "size",
}

/**
 * Column names for the `clp_files` table.
 */
enum CLP_FILES_TABLE_COLUMN_NAMES {
    ORIG_FILE_ID = "orig_file_id",
    NUM_MESSAGES = "num_messages",
}

export {
    CLP_ARCHIVES_TABLE_COLUMN_NAMES,
    CLP_FILES_TABLE_COLUMN_NAMES,
};
