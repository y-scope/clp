#include "MultiLogtypeTablesManager.hpp"

#include <set>

#include "../LogtypeSizeTracker.hpp"

using glt::streaming_archive::LogtypeSizeTracker;

namespace glt::streaming_archive::reader {

void MultiLogtypeTablesManager::open(std::string const& segment_path) {
    LogtypeTableManager::open(segment_path);
}

bool MultiLogtypeTablesManager::check_variable_column(logtype_dictionary_id_t logtype_id) {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (m_logtype_tables.find(logtype_id) != m_logtype_tables.end()) {
        return true;
    }
    if (m_combined_tables.find(logtype_id) != m_combined_tables.end()) {
        return true;
    }
    return false;
}

epochtime_t MultiLogtypeTablesManager::get_timestamp_at_offset(
        logtype_dictionary_id_t logtype_id,
        size_t offset
) {
    if (m_logtype_tables.find(logtype_id) != m_logtype_tables.end()) {
        return m_logtype_tables[logtype_id].get_timestamp_at_offset(offset);
    } else if (m_combined_tables.find(logtype_id) != m_combined_tables.end()) {
        return m_combined_tables[logtype_id].get_timestamp_at_offset(offset);
    } else {
        SPDLOG_ERROR("request logtype id is invalid {}", logtype_id);
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

void MultiLogtypeTablesManager::load_variable_columns(logtype_dictionary_id_t logtype_id) {
    if (!m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    if (m_logtype_table_metadata.find(logtype_id) != m_logtype_table_metadata.end()) {
        if (m_logtype_tables.find(logtype_id) != m_logtype_tables.end()) {
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        auto const& logtype_metadata = m_logtype_table_metadata.at(logtype_id);
        m_logtype_tables[logtype_id].open_and_load_all(
                m_memory_mapped_segment_file.data(),
                logtype_metadata
        );

    } else if (m_combined_tables_metadata.find(logtype_id) != m_combined_tables_metadata.end()) {
        if (m_combined_tables.find(logtype_id) != m_combined_tables.end()) {
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
        // Now, we simply load everything belonging to a single combined table;
        load_all_tables(m_combined_tables_metadata[logtype_id].combined_table_id);
    } else {
        SPDLOG_ERROR("request logtype id is invalid {}", logtype_id);
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

void MultiLogtypeTablesManager::load_all_tables(combined_table_id_t combined_table_id) {
    std::set<LogtypeSizeTracker, std::greater<LogtypeSizeTracker>> combined_table_tracker;
    for (auto const& iter : m_combined_tables_metadata) {
        auto const& logtype_info = iter.second;
        if (logtype_info.combined_table_id == combined_table_id) {
            auto logtype_id = iter.first;
            if (m_combined_tables_metadata.find(logtype_id) == m_combined_tables_metadata.end()) {
                SPDLOG_ERROR("logtype id {} doesn't exist in either form of table");
            }
            combined_table_tracker
                    .emplace(logtype_id, logtype_info.num_columns, logtype_info.num_rows);
        }
    }

    // compressor for combined table. try to reuse only one compressor
#if USE_PASSTHROUGH_COMPRESSION
    streaming_compression::passthrough::Decompressor combined_table_decompressor;
#elif USE_ZSTD_COMPRESSION
    streaming_compression::zstd::Decompressor combined_table_decompressor;
#else
    static_assert(false, "Unsupported compression mode.");
#endif
    char const* compressed_stream_ptr = m_memory_mapped_segment_file.data()
                                        + m_combined_table_info[combined_table_id].m_begin_offset;
    size_t compressed_stream_size = m_combined_table_info[combined_table_id].m_size;
    combined_table_decompressor.open(compressed_stream_ptr, compressed_stream_size);
    for (auto const& logtype_table : combined_table_tracker) {
        auto const& logtype_id = logtype_table.get_id();
        assert(m_combined_tables.find(logtype_id) == m_combined_tables.end());
        m_combined_tables[logtype_id].open_and_read_once_only(
                logtype_id,
                combined_table_id,
                combined_table_decompressor,
                m_combined_tables_metadata
        );
    }
}

void MultiLogtypeTablesManager::get_variable_row_at_offset(
        logtype_dictionary_id_t logtype_id,
        size_t offset,
        Message& msg
) {
    if (m_logtype_tables.find(logtype_id) != m_logtype_tables.end()) {
        m_logtype_tables[logtype_id].get_message_at_offset(offset, msg);
    } else if (m_combined_tables.find(logtype_id) != m_combined_tables.end()) {
        m_combined_tables[logtype_id].get_message_at_offset(offset, msg);
    } else {
        SPDLOG_ERROR("request logtype id is invalid {}", logtype_id);
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

void MultiLogtypeTablesManager::close() {
    for (auto& variable_reader : m_logtype_tables) {
        variable_reader.second.close();
    }
    m_logtype_tables.clear();
    m_combined_tables.clear();
    // here we also rely on base class close
    LogtypeTableManager::close();
}
}  // namespace glt::streaming_archive::reader
