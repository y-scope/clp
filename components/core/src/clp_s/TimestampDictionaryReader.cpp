#include "TimestampDictionaryReader.hpp"

#include <unordered_set>

#include "Utils.hpp"

namespace clp_s {
ErrorCode TimestampDictionaryReader::read(ZstdDecompressor& decompressor) {
    ErrorCode error;
    uint64_t range_index_size;
    error = decompressor.try_read_numeric_value<uint64_t>(range_index_size);
    if (ErrorCodeSuccess != error) {
        return error;
    }

    for (uint64_t i = 0; i < range_index_size; ++i) {
        TimestampEntry entry;
        std::vector<std::string> tokens;
        if (auto rc = entry.try_read_from_file(decompressor); ErrorCodeSuccess != rc) {
            throw OperationFailed(rc, __FILENAME__, __LINE__);
        }

        if (false == StringUtils::tokenize_column_descriptor(entry.get_key_name(), tokens)) {
            throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
        }
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
    error = decompressor.try_read_numeric_value<uint64_t>(num_patterns);
    if (ErrorCodeSuccess != error) {
        return error;
    }
    for (uint64_t i = 0; i < num_patterns; ++i) {
        uint64_t id, pattern_len;
        std::string pattern;
        error = decompressor.try_read_numeric_value<uint64_t>(id);
        if (ErrorCodeSuccess != error) {
            return error;
        }
        error = decompressor.try_read_numeric_value<uint64_t>(pattern_len);
        if (ErrorCodeSuccess != error) {
            return error;
        }
        error = decompressor.try_read_string(pattern_len, pattern);
        if (ErrorCodeSuccess != error) {
            return error;
        }
        m_patterns[id] = TimestampPattern(0, pattern);
    }
    return ErrorCodeSuccess;
}

std::string
TimestampDictionaryReader::get_string_encoding(epochtime_t epoch, uint64_t format_id) const {
    std::string ret;
    m_patterns.at(format_id).insert_formatted_timestamp(epoch, ret);

    return ret;
}

}  // namespace clp_s
