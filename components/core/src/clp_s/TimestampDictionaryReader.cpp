#include "TimestampDictionaryReader.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

#include <clp_s/timestamp_parser/TimestampParser.hpp>

#include "search/ast/SearchUtils.hpp"
#include "TimestampPattern.hpp"

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
        std::string descriptor_namespace;
        if (auto rc = entry.try_read_from_file(decompressor); ErrorCodeSuccess != rc) {
            throw OperationFailed(rc, __FILENAME__, __LINE__);
        }

        if (false
            == clp_s::search::ast::tokenize_column_descriptor(
                    entry.get_key_name(),
                    tokens,
                    descriptor_namespace
            ))
        {
            throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
        }
        m_entries.emplace_back(std::move(entry));

        // TODO: Currently, we only allow a single authoritative timestamp column at ingestion time,
        // but the timestamp dictionary is designed to store the ranges of several timestamp
        // columns. We should enforce a convention that the first entry in the timestamp dictionary
        // corresponds to the "authoritative" timestamp column for the dataset.
        if (i == 0) {
            m_authoritative_timestamp_column_ids = m_entries.back().get_column_ids();
            m_authoritative_timestamp_tokenized_column = {tokens, descriptor_namespace};
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
        m_deprecated_patterns.emplace(id, TimestampPattern(0, pattern));
        auto timestamp_pattern_result{timestamp_parser::TimestampPattern::create(pattern)};
        if (timestamp_pattern_result.has_error()) {
            m_timestamp_patterns.emplace(id, std::nullopt);
        } else {
            m_timestamp_patterns.emplace(id, std::move(timestamp_pattern_result.value()));
        }
    }
    return ErrorCodeSuccess;
}

auto TimestampDictionaryReader::get_deprecated_timestamp_string_encoding(
        epochtime_t epoch,
        uint64_t format_id
) const -> std::string {
    std::string ret;
    m_deprecated_patterns.at(format_id).insert_formatted_timestamp(epoch, ret);

    return ret;
}

void TimestampDictionaryReader::append_timestamp_to_buffer(
        epochtime_t timestamp,
        uint64_t format_id,
        std::string& buffer
) const {
    auto const& pattern{m_timestamp_patterns.at(format_id)};
    if (false == pattern.has_value()) {
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }

    auto const marshal_result{
            timestamp_parser::marshal_timestamp(timestamp, pattern.value(), buffer)
    };
    if (marshal_result.has_error()) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
}
}  // namespace clp_s
