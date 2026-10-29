#ifndef GLT_STREAMING_ARCHIVE_READER_SINGLELOGTYPETABLEMANAGER_HPP
#define GLT_STREAMING_ARCHIVE_READER_SINGLELOGTYPETABLEMANAGER_HPP

// Project headers
#include <map>

#include "../../Query.hpp"
#include "CombinedLogtypeTable.hpp"
#include "LogtypeTableManager.hpp"

namespace glt::streaming_archive::reader {
class SingleLogtypeTableManager : public streaming_archive::reader::LogtypeTableManager {
public:
    SingleLogtypeTableManager() : m_logtype_table_loaded(false) {}

    void open_logtype_table(logtype_dictionary_id_t logtype_id);
    void close_logtype_table();

    void load_all();
    void load_partial_columns(size_t l, size_t r);
    void load_ts();

    void skip_row();
    bool get_next_row(Message& msg);
    bool peek_next_ts(epochtime_t& ts);

    void open_combined_table(combined_table_id_t table_id);
    void close_combined_table();
    void load_logtype_table_from_combine(logtype_dictionary_id_t logtype_id);

    void rearrange_queries(
            std::unordered_map<logtype_dictionary_id_t, LogtypeQueries> const& src_queries,
            std::vector<LogtypeQueries>& single_table_queries,
            std::map<combined_table_id_t, std::vector<LogtypeQueries>>& combined_table_queries
    );

    // getter
    LogtypeTable& logtype_table() { return m_logtype_table; }

    CombinedLogtypeTable& combined_tables() { return m_combined_tables; }

private:
    bool m_logtype_table_loaded;
    LogtypeTable m_logtype_table;
    CombinedLogtypeTable m_combined_tables;

    // compressor for combined table. try to reuse only one compressor
#if USE_PASSTHROUGH_COMPRESSION
    streaming_compression::passthrough::Decompressor m_combined_table_decompressor;
#elif USE_ZSTD_COMPRESSION
    streaming_compression::zstd::Decompressor m_combined_table_decompressor;
#else
    static_assert(false, "Unsupported compression mode.");
#endif
};
}  // namespace glt::streaming_archive::reader

#endif  // GLT_STREAMING_ARCHIVE_READER_SINGLELOGTYPETABLEMANAGER_HPP
