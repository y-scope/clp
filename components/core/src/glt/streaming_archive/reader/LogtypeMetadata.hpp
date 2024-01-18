#ifndef STREAMING_ARCHIVE_READER_LOGTYPE_METADATA_HPP
#define STREAMING_ARCHIVE_READER_LOGTYPE_METADATA_HPP
#include <vector>

#include "../../Defs.h"

namespace glt::streaming_archive::reader {

// logtype belonging to single logtype table
class LogtypeMetadata {
public:
    size_t num_rows;
    size_t num_columns;
    std::vector<size_t> column_offset;
    std::vector<size_t> column_size;
    size_t ts_offset;
    size_t ts_size;
    size_t file_id_offset;
    size_t file_id_size;
};

// logtype belonging to combined logtype table
class CombinedMetadata {
public:
    size_t num_rows;
    size_t num_columns;
    size_t combined_table_id;
    // byte offset of the table's beginning position.
    size_t offset;
};

class CombinedTableInfo {
public:
    size_t m_begin_offset;  // table's start offset
    size_t m_size;  // compressed table size.
};
}  // namespace glt::streaming_archive::reader

#endif  // STREAMING_ARCHIVE_READER_LOGTYPE_METADATA_HPP
