#ifndef CLP_STREAMING_ARCHIVE_CONSTANTS_HPP
#define CLP_STREAMING_ARCHIVE_CONSTANTS_HPP

#include "../Defs.h"

namespace clp::streaming_archive {
constexpr char cSegmentsDirname[] = "s";
constexpr char cSegmentListFilename[] = "segment_list.txt";
constexpr char cLogTypeDictFilename[] = "logtype.dict";
constexpr char cVarDictFilename[] = "var.dict";
constexpr char cLogTypeSegmentIndexFilename[] = "logtype.segindex";
constexpr char cVarSegmentIndexFilename[] = "var.segindex";
constexpr char cMetadataFileName[] = "metadata";
constexpr char cMetadataDBFileName[] = "metadata.db";
constexpr char cSchemaFileName[] = "schema.txt";

namespace version {
// Version(s) for the disk format of the archive. The public branch has only one version; however, private branches have two.
// The private branch has its own independent disk format version, and a second version to specify which public release can
// read its private disk format. The following provides instructions to set the versions correctly.
//
// Public branch with version a.b.c
//
// | Public  version | Private  version |
// | --------------- | ---------------- |
// | a.b.c           | 0.0.0            |
//
// Private branch with version x.y.z compatible with public version a.b.c
//
// | Public  version | Private  version |
// | --------------- | ---------------- |
// | a.b.c           | x.y.z            |
//
// Private branch with version x.y.z incompatible with any public version.
//
// | Public  version | Private  version |
// | --------------- | ---------------- |
// | 0.0.0           | x.y.z            |
//
constexpr uint8_t cPublicVersionMajor{0};
constexpr uint8_t cPublicVersionMinor{1};
constexpr uint16_t cPublicVersionPatch{0};
constexpr archive_format_version_t cPublicVersion{
        cPublicVersionMajor << 24 | cPublicVersionMinor << 16| cPublicVersionPatch
};
constexpr archive_format_version_t cNullVersion{0};
constexpr archive_format_version_t cPrivateVersion{cNullVersion};
}

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
constexpr char BeginMessageIx[] = "begin_message_ix";
constexpr char NumMessages[] = "num_messages";
constexpr char NumVariables[] = "num_variables";
constexpr char IsSplit[] = "is_split";
constexpr char SplitIx[] = "split_ix";
constexpr char SegmentId[] = "segment_id";
constexpr char SegmentTimestampsPosition[] = "segment_timestamps_position";
constexpr char SegmentLogtypesPosition[] = "segment_logtypes_position";
constexpr char SegmentVariablesPosition[] = "segment_variables_position";
constexpr char ArchiveId[] = "archive_id";
}  // namespace File

namespace EmptyDirectory {
constexpr char Path[] = "path";
}  // namespace EmptyDirectory
}  // namespace cMetadataDB
}  // namespace clp::streaming_archive

#endif  // CLP_STREAMING_ARCHIVE_CONSTANTS_HPP
