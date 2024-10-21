#ifndef CLP_S_ARCHIVE_CONSTANTS_HPP
#define CLP_S_ARCHIVE_CONSTANTS_HPP

namespace clp_s::constants {
// Single file archive
constexpr char cArchiveFile[] = "/archive";

// Schema files
constexpr char cArchiveSchemaMapFile[] = "/schema_ids";
constexpr char cArchiveSchemaTreeFile[] = "/schema_tree";

// Encoded record table files
constexpr char cArchiveTableMetadataFile[] = "/table_metadata";
constexpr char cArchiveTablesFile[] = "/tables";

// Dictionary files
constexpr char cArchiveArrayDictFile[] = "/array.dict";
constexpr char cArchiveLogDictFile[] = "/log.dict";
constexpr char cArchiveTimestampDictFile[] = "/timestamp.dict";
constexpr char cArchiveVarDictFile[] = "/var.dict";

namespace results_cache::decompression {
constexpr char cPath[]{"path"};
constexpr char cOrigFileId[]{"orig_file_id"};
constexpr char cBeginMsgIx[]{"begin_msg_ix"};
constexpr char cEndMsgIx[]{"end_msg_ix"};
constexpr char cIsLastIrChunk[]{"is_last_ir_chunk"};
}  // namespace results_cache::decompression

}  // namespace clp_s::constants
#endif  // CLP_S_ARCHIVE_CONSTANTS_HPP
