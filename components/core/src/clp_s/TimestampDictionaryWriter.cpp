#include "TimestampDictionaryWriter.hpp"

#include "Utils.hpp"

namespace clp_s {
void TimestampDictionaryWriter::write_timestamp_entries(
        std::map<std::string, TimestampEntry> const& ranges,
        ZstdCompressor& compressor
) {
    compressor.write_numeric_value<uint64_t>(ranges.size());

    for (auto const& range : ranges) {
        range.second.write_to_file(compressor, range.first);
    }
}

void TimestampDictionaryWriter::write_and_flush_to_disk() {
    write_timestamp_entries(m_global_column_to_range, m_dictionary_compressor);

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
    write_timestamp_entries(m_local_column_to_range, m_dictionary_compressor_local);

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
    m_local_column_to_range.clear();
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
        std::string const& timestamp,
        uint64_t& id
) {
    epochtime_t ret;
    size_t timestamp_begin_pos = 0, timestamp_end_pos = 0;
    TimestampPattern const* pattern = TimestampPattern::search_known_ts_patterns(
            timestamp,
            ret,
            timestamp_begin_pos,
            timestamp_end_pos
    );
    m_local_column_to_range[key].ingest_timestamp(ret);

    if (pattern == nullptr) {
        throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__);
    }

    id = get_pattern_id(pattern);

    return ret;
}

void TimestampDictionaryWriter::ingest_entry(std::string const& key, double timestamp) {
    m_local_column_to_range[key].ingest_timestamp(timestamp);
}

void TimestampDictionaryWriter::ingest_entry(std::string const& key, int64_t timestamp) {
    m_local_column_to_range[key].ingest_timestamp(timestamp);
}

void TimestampDictionaryWriter::merge_local_range() {
    for (auto const& it : m_local_column_to_range) {
        m_global_column_to_range[it.first].merge_range(it.second);
    }
}

bool TimestampDictionaryWriter::get_first_global_range(TimestampEntry& entry) {
    if (m_global_column_to_range.empty()) {
        return false;
    }

    entry = m_global_column_to_range.begin()->second;
    return true;
}
}  // namespace clp_s
