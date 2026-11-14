#include "TimestampDictionaryReader.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

#include <spdlog/spdlog.h>

#include <clp_s/timestamp_parser/TimestampParser.hpp>

#include "ReaderUtils.hpp"
#include "search/ast/SearchUtils.hpp"
#include "SingleFileArchiveDefs.hpp"
#include "TimestampPattern.hpp"

namespace clp_s {
auto TimestampDictionaryReader::read(
        ZstdDecompressor& decompressor,
        bool has_deprecated_timestamp_format
) -> ErrorCode {
    ErrorCode error;
    uint64_t num_entries{0};
    error = decompressor.try_read_numeric_value<uint64_t>(num_entries);
    if (ErrorCodeSuccess != error) {
        return error;
    }

    if (num_entries > 1) {
        SPDLOG_ERROR(
                "Timestamp dictionary contains {} timestamp entries, but at most one is supported.",
                num_entries
        );
        throw OperationFailed(ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }

    if (1 == num_entries) {
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

        m_authoritative_timestamp_column_ids = entry.get_column_ids();
        m_authoritative_timestamp_tokenized_column
                = {std::move(tokens), std::move(descriptor_namespace)};
        m_timestamp_entry.emplace(std::move(entry));
    }

    uint64_t num_patterns{0};
    error = decompressor.try_read_numeric_value<uint64_t>(num_patterns);
    if (ErrorCodeSuccess != error) {
        return error;
    }
    for (uint64_t i{0}; i < num_patterns; ++i) {
        uint64_t id{0};
        uint64_t pattern_len_u64{0};
        std::string pattern;

        error = decompressor.try_read_numeric_value<uint64_t>(id);
        if (ErrorCodeSuccess != error) {
            return error;
        }

        error = decompressor.try_read_numeric_value<uint64_t>(pattern_len_u64);
        if (ErrorCodeSuccess != error) {
            return error;
        }

        auto const pattern_len_result{ReaderUtils::try_uint64_to_size_t(pattern_len_u64)};
        if (pattern_len_result.has_error()) {
            return ErrorCodeOutOfBounds;
        }

        error = decompressor.try_read_string(pattern_len_result.value(), pattern);
        if (ErrorCodeSuccess != error) {
            return error;
        }

        if (has_deprecated_timestamp_format) {
            m_deprecated_patterns.emplace(id, TimestampPattern(0, pattern));
            continue;
        }

        auto timestamp_pattern_result{timestamp_parser::TimestampPattern::create(pattern)};
        if (timestamp_pattern_result.has_error()) {
            auto const& timestamp_error{timestamp_pattern_result.error()};
            SPDLOG_ERROR(
                    "Error loading timestamp pattern `{}` - {} - {}",
                    pattern,
                    timestamp_error.category().name(),
                    timestamp_error.message()
            );
            return ErrorCodeCorrupt;
        }
        m_timestamp_patterns.emplace(id, std::move(timestamp_pattern_result.value()));
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
    auto const marshal_result{timestamp_parser::marshal_timestamp(timestamp, pattern, buffer)};
    if (marshal_result.has_error()) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
}
}  // namespace clp_s
