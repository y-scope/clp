#ifndef CLP_S_ARCHIVE_CONSTANTS_HPP
#define CLP_S_ARCHIVE_CONSTANTS_HPP

namespace clp_s::constants {
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
}  // namespace clp_s::constants
#endif  // CLP_S_ARCHIVE_CONSTANTS_HPP
