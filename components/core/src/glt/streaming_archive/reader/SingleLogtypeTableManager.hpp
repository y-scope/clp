#ifndef CLP_SINGLELOGTYPETABLEMANAGER_HPP
#define CLP_SINGLELOGTYPETABLEMANAGER_HPP

// Project headers
#include <map>

#include "../../Query.hpp"
#include "CombinedLogtypeTable.hpp"
#include "LogtypeTableManager.hpp"

namespace glt::streaming_archive::reader {
class SingleLogtypeTableManager : public streaming_archive::reader::LogtypeTableManager {
public:
    SingleLogtypeTableManager() : m_variable_column_loaded(false){};
    void load_variable_columns(logtype_dictionary_id_t logtype_id);
    void close_variable_columns();
    bool get_next_row(Message& msg);
    bool peek_next_ts(epochtime_t& ts);
    void load_all();
    void skip_row();
    void load_partial_columns(size_t l, size_t r);
    void load_ts();

    void rearrange_queries(
            std::unordered_map<logtype_dictionary_id_t, LogtypeQueries> const& src_queries,
            std::vector<LogtypeQueries>& single_table_queries,
            std::map<combined_table_id_t, std::vector<LogtypeQueries>>& combined_table_queries
    );

    void open_combined_table(combined_table_id_t table_id);
    void open_and_preload_combined_table(
            combined_table_id_t table_id,
            logtype_dictionary_id_t logtype_id
    );
    void open_preloaded_combined_logtype_table(logtype_dictionary_id_t logtype_id);
    void close_combined_table();
    void open_combined_logtype_table(logtype_dictionary_id_t logtype_id);

    bool m_variable_column_loaded;
    LogtypeTable m_variable_columns;
    CombinedLogtypeTable m_combined_table_segment;

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

#endif  // CLP_SINGLELOGTYPETABLEMANAGER_HPP
