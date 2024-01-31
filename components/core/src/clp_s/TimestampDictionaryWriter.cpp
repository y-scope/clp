#include "TimestampDictionaryWriter.hpp"

#include "Utils.hpp"

namespace clp_s {
void TimestampDictionaryWriter::write_timestamp_entries(
        std::map<std::string, TimestampEntry> const& ranges,
        ZstdCompressor& compressor
) {
    compressor.write_numeric_value<uint64_t>(ranges.size());

    for (auto const& range : ranges) {
        range.second.write_to_file(compressor);
    }
}

void TimestampDictionaryWriter::write_and_flush_to_disk() {
    write_timestamp_entries(
            m_global_column_key_to_range,
            m_dictionary_compressor
    );

    m_dictionary_compressor.write_numeric_value<uint64_t>(m_pattern_to_id.size());
    for (auto& it : m_pattern_to_id) {
        // write pattern ID
        m_dictionary_compressor.write_numeric_value<uint64_t>(it.second);

        std::string const& pattern = it.first->get_format();
        m_dictionary_compressor.write_numeric_value<uint64_t>(pattern.length());
        m_dictionary_compressor.write_string(pattern);
    }

    m_dictionary_compressor.flush();
    m_dictionary_file_writer.flush();
}

void TimestampDictionaryWriter::write_local_and_flush_to_disk() {
    write_timestamp_entries(
            m_local_column_key_to_range,
            m_dictionary_compressor_local
    );

    m_dictionary_compressor_local.flush();
    m_dictionary_file_writer_local.flush();
}

void TimestampDictionaryWriter::open(std::string const& dictionary_path, int compression_level) {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    m_dictionary_file_writer.open(dictionary_path, FileWriter::OpenMode::CreateForWriting);
    m_dictionary_compressor.open(m_dictionary_file_writer, compression_level);

    m_next_id = 0;
    m_is_open = true;
}

void TimestampDictionaryWriter::open_local(
        std::string const& dictionary_path,
        int compression_level
) {
    if (m_is_open_local) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    m_dictionary_file_writer_local.open(dictionary_path, FileWriter::OpenMode::CreateForWriting);
    m_dictionary_compressor_local.open(m_dictionary_file_writer_local, compression_level);

    m_is_open_local = true;
}

void TimestampDictionaryWriter::close() {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    // merge before writing overall archive because this
    // happens before the last sub-archive is written
    merge_local_range();
    write_and_flush_to_disk();
    m_dictionary_compressor.close();
    m_dictionary_file_writer.close();

    m_is_open = false;
}

void TimestampDictionaryWriter::close_local() {
    if (false == m_is_open_local) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    write_local_and_flush_to_disk();
    m_dictionary_compressor_local.close();
    m_dictionary_file_writer_local.close();

    m_is_open_local = false;

    // merge after every sub-archive
    merge_local_range();
    m_local_column_id_to_range.clear();
    m_local_column_key_to_range.clear();
}

uint64_t TimestampDictionaryWriter::get_pattern_id(TimestampPattern const* pattern) {
    if (0 == m_pattern_to_id.count(pattern)) {
        uint64_t id = m_next_id++;
        m_pattern_to_id[pattern] = id;

        return id;
    }

    return m_pattern_to_id.at(pattern);
}

epochtime_t TimestampDictionaryWriter::ingest_entry(
        std::string const& key,
        int32_t node_id,
        std::string const& timestamp,
        uint64_t& pattern_id
) {
    epochtime_t ret;
    size_t timestamp_begin_pos = 0, timestamp_end_pos = 0;
    TimestampPattern const* pattern = TimestampPattern::search_known_ts_patterns(
            timestamp,
            ret,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    if (m_local_column_id_to_range.count(node_id) == 0) {
        m_local_column_id_to_range.emplace(node_id, key);
    }
    m_local_column_id_to_range[node_id].ingest_timestamp(ret);

    if (pattern == nullptr) {
        throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__);
    }

    pattern_id = get_pattern_id(pattern);

    return ret;
}

void TimestampDictionaryWriter::ingest_entry(
        std::string const& key,
        int32_t node_id,
        double timestamp
) {
    if (m_local_column_id_to_range.count(node_id) == 0) {
        m_local_column_id_to_range.emplace(node_id, key);
    }
    m_local_column_id_to_range[node_id].ingest_timestamp(timestamp);
}

void TimestampDictionaryWriter::ingest_entry(
        std::string const& key,
        int32_t node_id,
        int64_t timestamp
) {
    if (m_local_column_id_to_range.count(node_id) == 0) {
        m_local_column_id_to_range.emplace(node_id, key);
    }
    m_local_column_id_to_range[node_id].ingest_timestamp(timestamp);
}

void TimestampDictionaryWriter::merge_local_range() {
    for (auto const& it : m_local_column_id_to_range) {
        std::string key = it.second.get_key_name();
        if (m_local_column_key_to_range.count(key) == 0) {
            m_local_column_key_to_range.emplace(key, it.second);
        } else {
            auto& entry = m_local_column_key_to_range[key];
            entry.merge_range(it.second);
            entry.insert_column_id(it.first);
        }
    }

    for (auto const& it : m_local_column_key_to_range) {
        if (m_global_column_key_to_range.count(it.first) == 0) {
            m_global_column_key_to_range.emplace(it.first, it.second);
        } else {
            auto& entry = m_global_column_key_to_range[it.first];
            entry.merge_range(it.second);
            entry.insert_column_ids(it.second.get_column_ids());
        }
    }
}
}  // namespace clp_s
