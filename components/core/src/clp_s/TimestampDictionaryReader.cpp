#include "TimestampDictionaryReader.hpp"

#include <unordered_set>

#include "Utils.hpp"

namespace clp_s {
void TimestampDictionaryReader::open(std::string const& dictionary_path) {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    constexpr size_t cDecompressorFileReadBufferCapacity = 16 * 1024;  // 16 KB

    m_dictionary_file_reader.open(dictionary_path);
    m_dictionary_decompressor.open(m_dictionary_file_reader, cDecompressorFileReadBufferCapacity);

    m_is_open = true;
}

void TimestampDictionaryReader::close() {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    m_dictionary_decompressor.close();
    m_dictionary_file_reader.close();
}

void TimestampDictionaryReader::read_new_entries() {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    ErrorCode error;

    uint64_t range_index_size;
    error = m_dictionary_decompressor.try_read_numeric_value<uint64_t>(range_index_size);
    if (ErrorCodeSuccess != error) {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }

    for (uint64_t i = 0; i < range_index_size; ++i) {
        TimestampEntry entry;
        std::vector<std::string> tokens;
        entry.try_read_from_file(m_dictionary_decompressor);
        StringUtils::tokenize_column_descriptor(entry.get_key_name(), tokens);
        m_entries.emplace_back(std::move(entry));

        // TODO: Currently, we only allow a single authoritative timestamp column at ingestion time,
        // but the timestamp dictionary is designed to store the ranges of several timestamp
        // columns. We should enforce a convention that the first entry in the timestamp dictionary
        // corresponds to the "authoritative" timestamp column for the dataset.
        if (i == 0) {
            m_authoritative_timestamp_column_ids = m_entries.back().get_column_ids();
            m_authoritative_timestamp_tokenized_column = tokens;
        }

        m_tokenized_column_to_range.emplace_back(std::move(tokens), &m_entries.back());
    }

    uint64_t num_patterns;
    error = m_dictionary_decompressor.try_read_numeric_value<uint64_t>(num_patterns);
    if (ErrorCodeSuccess != error) {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }
    for (int i = 0; i < num_patterns; ++i) {
        uint64_t id, pattern_len;
        std::string pattern;
        error = m_dictionary_decompressor.try_read_numeric_value<uint64_t>(id);
        if (ErrorCodeSuccess != error) {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }
        error = m_dictionary_decompressor.try_read_numeric_value<uint64_t>(pattern_len);
        if (ErrorCodeSuccess != error) {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }
        error = m_dictionary_decompressor.try_read_string(pattern_len, pattern);
        if (ErrorCodeSuccess != error) {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }
        m_patterns[id] = TimestampPattern(0, pattern);
    }
}

std::string
TimestampDictionaryReader::get_string_encoding(epochtime_t epoch, uint64_t format_id) const {
    std::string ret;
    m_patterns.at(format_id).insert_formatted_timestamp(epoch, ret);

    return ret;
}

}  // namespace clp_s
