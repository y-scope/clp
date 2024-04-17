#include "SingleLogtypeTableManager.hpp"

#include <queue>

#include "../LogtypeSizeTracker.hpp"

namespace glt::streaming_archive::reader {
void SingleLogtypeTableManager::open_logtype_table(logtype_dictionary_id_t logtype_id) {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (m_logtype_table_loaded != false) {
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }

    auto const& logtype_metadata = m_logtype_table_metadata[logtype_id];
    m_logtype_table.open(m_memory_mapped_segment_file.data(), logtype_metadata);
    m_logtype_table_loaded = true;
}

void SingleLogtypeTableManager::close_logtype_table() {
    m_logtype_table.close();
    m_logtype_table_loaded = false;
}

bool SingleLogtypeTableManager::get_next_row(Message& msg) {
    return m_logtype_table.get_next_message(msg);
}

bool SingleLogtypeTableManager::peek_next_ts(epochtime_t& ts) {
    return m_logtype_table.peek_next_ts(ts);
}

void SingleLogtypeTableManager::load_all() {
    m_logtype_table.load_all();
}

void SingleLogtypeTableManager::skip_row() {
    m_logtype_table.skip_row();
}

void SingleLogtypeTableManager::load_partial_columns(size_t l, size_t r) {
    m_logtype_table.load_variable_columns(l, r);
}

void SingleLogtypeTableManager::load_ts() {
    m_logtype_table.load_timestamp();
}

void SingleLogtypeTableManager::open_combined_table(combined_table_id_t table_id) {
    char const* compressed_stream_ptr
            = m_memory_mapped_segment_file.data() + m_combined_table_info[table_id].m_begin_offset;
    size_t compressed_stream_size = m_combined_table_info[table_id].m_size;
    m_combined_table_decompressor.open(compressed_stream_ptr, compressed_stream_size);
    m_combined_tables.open(table_id);
}

void SingleLogtypeTableManager::close_combined_table() {
    m_combined_tables.close();
    m_combined_table_decompressor.close();
}

void SingleLogtypeTableManager::load_logtype_table_from_combine(logtype_dictionary_id_t logtype_id
) {
    m_combined_tables.load_logtype_table(
            logtype_id,
            m_combined_table_decompressor,
            m_combined_tables_metadata
    );
}

// rearrange queries to separate them into single table and combined table ones.
// also make sure that they are sorted in a way such that the order is same as them on the disk.
void SingleLogtypeTableManager::rearrange_queries(
        std::unordered_map<logtype_dictionary_id_t, LogtypeQueries> const& src_queries,
        std::vector<LogtypeQueries>& single_table_queries,
        std::map<combined_table_id_t, std::vector<LogtypeQueries>>& combined_table_queries
) {
    // Sort the logtype table in descending order of table_size
    std::priority_queue<LogtypeSizeTracker> single_table_tracker;
    std::map<combined_table_id_t, std::priority_queue<LogtypeSizeTracker>> combined_table_tracker;
    for (auto const& iter : src_queries) {
        auto logtype_id = iter.first;
        if (m_logtype_table_metadata.count(logtype_id) != 0) {
            auto const& logtype_info = m_logtype_table_metadata[logtype_id];
            single_table_tracker
                    .emplace(logtype_id, logtype_info.num_columns, logtype_info.num_rows);
        } else {
            if (m_combined_tables_metadata.find(logtype_id) == m_combined_tables_metadata.end()) {
                SPDLOG_ERROR("logtype id {} doesn't exist in either form of table");
            }
            auto const& logtype_info = m_combined_tables_metadata[logtype_id];
            combined_table_tracker[logtype_info.combined_table_id]
                    .emplace(logtype_id, logtype_info.num_columns, logtype_info.num_rows);
        }
    }

    while (!single_table_tracker.empty()) {
        auto const& sorted_logtype_id = single_table_tracker.top().get_id();
        single_table_queries.push_back(src_queries.at(sorted_logtype_id));
        single_table_tracker.pop();
    }

    for (auto& combined_table_iter : combined_table_tracker) {
        combined_table_id_t table_id = combined_table_iter.first;
        auto& tracker_queue = combined_table_iter.second;
        while (!tracker_queue.empty()) {
            auto const& sorted_logtype_id = tracker_queue.top().get_id();
            combined_table_queries[table_id].push_back(src_queries.at(sorted_logtype_id));
            tracker_queue.pop();
        }
    }
}
}  // namespace glt::streaming_archive::reader
