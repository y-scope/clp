import axios from "axios";


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

/**
 * Column names for the `compression_jobs` table.
 */
enum COMPRESSION_JOBS_TABLE_COLUMN_NAMES {
    CLP_BINARY_VERSION = "clp_binary_version",
    CLP_CONFIG = "clp_config",
    COMPRESSED_SIZE = "compressed_size",
    CREATION_TIME = "creation_time",
    DURATION = "duration",
    ID = "id",
    NUM_TASKS = "num_tasks",
    NUM_TASKS_COMPLETED = "num_tasks_completed",
    ORIGINAL_SIZE = "original_size",
    START_TIME = "start_time",
    STATUS = "status",
    STATUS_MSG = "status_msg",
    UNCOMPRESSED_SIZE = "uncompressed_size",
    UPDATE_TIME = "update_time",
}

/**
 * Query the SQL server with the queryString.
 *
 * @param queryString
 * @return
 */
const querySql = async <T>(queryString: string) => {
    return axios.post<T>("/query/sql", {queryString});
};

export {
    CLP_ARCHIVES_TABLE_COLUMN_NAMES,
    CLP_FILES_TABLE_COLUMN_NAMES,
    COMPRESSION_JOBS_TABLE_COLUMN_NAMES,
    querySql,
};
