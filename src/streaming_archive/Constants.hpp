// Project headers
#include "../Defs.h"

#ifndef STREAMING_ARCHIVE_CONSTANTS_HPP
#define STREAMING_ARCHIVE_CONSTANTS_HPP

#define STREAMING_ARCHIVE_METADATA_DB_ARCHIVES_TABLE_NAME "archives"
#define STREAMING_ARCHIVE_METADATA_DB_FILES_TABLE_NAME "files"
#define STREAMING_ARCHIVE_METADATA_DB_EMPTY_DIRECTORIES_TABLE_NAME "empty_directories"

#define STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_ID "id"
#define STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_STORAGE_ID "storage_id"
#define STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_UNCOMPRESSED_SIZE "uncompressed_size"
#define STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_SIZE "size"
#define STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_CREATOR_ID "creator_id"
#define STREAMING_ARCHIVE_METADATA_DB_ARCHIVE_CREATION_IX "creation_ix"

#define STREAMING_ARCHIVE_METADATA_DB_FILE_ID "id"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_ORIG_FILE_ID "orig_file_id"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_PATH "path"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_BEGIN_TIMESTAMP "begin_timestamp"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_END_TIMESTAMP "end_timestamp"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_TIMESTAMP_PATTERNS "timestamp_patterns"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_UNCOMPRESSED_BYTES "num_uncompressed_bytes"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_MESSAGES "num_messages"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_NUM_VARIABLES "num_variables"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_IS_SPLIT "is_split"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_SPLIT_IX "split_ix"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_ID "segment_id"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_TIMESTAMPS_POSITION "segment_timestamps_position"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_LOGTYPES_POSITION "segment_logtypes_position"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_SEGMENT_VARIABLES_POSITION "segment_variables_position"
#define STREAMING_ARCHIVE_METADATA_DB_FILE_ARCHIVE_ID "archive_id"

#define STREAMING_ARCHIVE_METADATA_DB_EMPTY_DIRECTORY_PATH "path"

namespace streaming_archive {
    constexpr archive_format_version_t cArchiveFormatVersion = 1;
    constexpr char cLogsDirname[] = "l";
    constexpr char cSegmentsDirname[] = "s";
    constexpr char cSegmentListFilename[] = "segment_list.txt";
    constexpr char cLogTypeDictFilename[] = "logtype.dict";
    constexpr char cVarDictFilename[] = "var.dict";
    constexpr char cLogTypeSegmentIndexFilename[] = "logtype.segindex";
    constexpr char cVarSegmentIndexFilename[] = "var.segindex";
    constexpr char cMetadataFileName[] = "metadata";
    constexpr char cMetadataDBFileName[] = "metadata.db";
    constexpr char cTimestampsFileExtension[] = ".tme";
    constexpr char cLogTypeIdsFileExtension[] = ".lid";
    constexpr char cVariablesFileExtension[] = ".var";
}

#endif // STREAMING_ARCHIVE_CONSTANTS_HPP
