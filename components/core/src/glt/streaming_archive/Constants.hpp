#ifndef GLT_STREAMING_ARCHIVE_CONSTANTS_HPP
#define GLT_STREAMING_ARCHIVE_CONSTANTS_HPP

#include "../Defs.h"

namespace glt::streaming_archive {
constexpr archive_format_version_t cArchiveFormatVersion = cArchiveFormatDevVersionFlag | 8;
constexpr char cSegmentsDirname[] = "s";
constexpr char cSegmentListFilename[] = "segment_list.txt";
constexpr char cLogTypeDictFilename[] = "logtype.dict";
constexpr char cVarDictFilename[] = "var.dict";
constexpr char cFileNameDictFilename[] = "filename.dict";
constexpr char cLogTypeSegmentIndexFilename[] = "logtype.segindex";
constexpr char cVarSegmentIndexFilename[] = "var.segindex";
constexpr char cMetadataFileName[] = "metadata";
constexpr char cMetadataDBFileName[] = "metadata.db";
constexpr char cVarSegmentFileName[] = "variable_segments";
constexpr char cVarMetadataFileName[] = "metadata";
constexpr char cVariablesFileExtension[] = ".var";
constexpr char cSchemaFileName[] = "schema.txt";

namespace cMetadataDB {
constexpr char ArchivesTableName[] = "archives";
constexpr char FilesTableName[] = "files";
constexpr char EmptyDirectoriesTableName[] = "empty_directories";

namespace Archive {
constexpr char Id[] = "id";
constexpr char BeginTimestamp[] = "begin_timestamp";
constexpr char EndTimestamp[] = "end_timestamp";
constexpr char UncompressedSize[] = "uncompressed_size";
constexpr char Size[] = "size";
constexpr char CreatorId[] = "creator_id";
constexpr char CreationIx[] = "creation_ix";
}  // namespace Archive

namespace File {
constexpr char Id[] = "id";
constexpr char OrigFileId[] = "orig_file_id";
constexpr char Path[] = "path";
constexpr char BeginTimestamp[] = "begin_timestamp";
constexpr char EndTimestamp[] = "end_timestamp";
constexpr char TimestampPatterns[] = "timestamp_patterns";
constexpr char NumUncompressedBytes[] = "num_uncompressed_bytes";
constexpr char NumMessages[] = "num_messages";
constexpr char NumVariables[] = "num_variables";
constexpr char IsSplit[] = "is_split";
constexpr char SplitIx[] = "split_ix";
constexpr char SegmentId[] = "segment_id";
constexpr char SegmentTimestampsPosition[] = "segment_timestamps_position";
constexpr char SegmentLogtypesPosition[] = "segment_logtypes_position";
constexpr char SegmentVariablesPosition[] = "segment_variables_position";
constexpr char SegmentOffsetPosition[] = "segment_offset_position";
constexpr char ArchiveId[] = "archive_id";
}  // namespace File

namespace EmptyDirectory {
constexpr char Path[] = "path";
}  // namespace EmptyDirectory
}  // namespace cMetadataDB

namespace LogtypeTableType {
constexpr uint64_t NonCombined = 0;
constexpr uint64_t Combined = 1;
}  // namespace LogtypeTableType
}  // namespace glt::streaming_archive

#endif  // GLT_STREAMING_ARCHIVE_CONSTANTS_HPP
