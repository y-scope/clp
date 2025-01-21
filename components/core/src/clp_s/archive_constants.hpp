#ifndef CLP_S_ARCHIVE_CONSTANTS_HPP
#define CLP_S_ARCHIVE_CONSTANTS_HPP

#include <cstdint>

namespace clp_s::constants {
// Single file archive
constexpr char cTmpPostfix[] = ".tmp";

// Header and metadata section
constexpr char cArchiveHeaderFile[] = "/header";

// Schema files
constexpr char cArchiveSchemaMapFile[] = "/schema_ids";
constexpr char cArchiveSchemaTreeFile[] = "/schema_tree";

// Encoded record table files
constexpr char cArchiveTableMetadataFile[] = "/table_metadata";
constexpr char cArchiveTablesFile[] = "/0";

// Dictionary files
constexpr char cArchiveArrayDictFile[] = "/array.dict";
constexpr char cArchiveLogDictFile[] = "/log.dict";
constexpr char cArchiveVarDictFile[] = "/var.dict";

// Schema tree constants
constexpr char cRootNodeName[] = "";
constexpr int32_t cRootNodeId = -1;
constexpr char cMetadataSubtreeName[] = "";
constexpr char cLogEventIdxName[] = "log_event_idx";

namespace results_cache::decompression {
constexpr char cPath[]{"path"};
constexpr char cStreamId[]{"stream_id"};
constexpr char cBeginMsgIx[]{"begin_msg_ix"};
constexpr char cEndMsgIx[]{"end_msg_ix"};
constexpr char cIsLastChunk[]{"is_last_chunk"};
}  // namespace results_cache::decompression

namespace results_cache::search {
constexpr char cOrigFilePath[]{"orig_file_path"};
constexpr char cLogEventIx[]{"log_event_ix"};
constexpr char cTimestamp[]{"timestamp"};
constexpr char cMessage[]{"message"};
constexpr char cArchiveId[]{"archive_id"};
}  // namespace results_cache::search

}  // namespace clp_s::constants
#endif  // CLP_S_ARCHIVE_CONSTANTS_HPP
