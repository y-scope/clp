#include "TimestampDictionaryWriter.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <clp_s/timestamp_parser/TimestampParser.hpp>

namespace clp_s {
void TimestampDictionaryWriter::write(std::stringstream& stream) {
    write_numeric_value<uint64_t>(stream, m_column_id_to_range.size());
    for (auto const& [id, range] : m_column_id_to_range) {
        range.write_to_stream(stream);
    }

    write_numeric_value<uint64_t>(
            stream,
            m_string_pattern_and_id_pairs.size() + m_numeric_pattern_to_id.size()
    );
    for (auto const& [quoted_pattern, pattern_id] : m_string_pattern_and_id_pairs) {
        write_numeric_value<uint64_t>(stream, pattern_id);

        auto const raw_pattern{quoted_pattern.get_pattern()};
        write_numeric_value<uint64_t>(stream, raw_pattern.length());
        stream.write(raw_pattern.data(), raw_pattern.size());
    }

    for (auto const& [raw_pattern, pattern_and_id] : m_numeric_pattern_to_id) {
        write_numeric_value<uint64_t>(stream, pattern_and_id.second);

        write_numeric_value<uint64_t>(stream, raw_pattern.size());
        stream.write(raw_pattern.data(), raw_pattern.size());
    }
}

auto TimestampDictionaryWriter::ingest_string_timestamp(
        std::string_view key,
        int32_t node_id,
        std::string_view timestamp,
        bool is_json_literal
) -> std::pair<epochtime_t, uint64_t> {
    auto& [_, timestamp_entry] = *m_column_id_to_range.try_emplace(node_id, key).first;

    // Try parsing the timestamp as one of the previously seen timestamp patterns
    for (auto const& [quoted_pattern, pattern_id] : m_string_pattern_and_id_pairs) {
        auto const parsing_result{timestamp_parser::parse_timestamp(
                timestamp,
                quoted_pattern,
                is_json_literal,
                m_generated_pattern
        )};
        if (parsing_result.has_error()) {
            continue;
        }
        auto const epoch_timestamp{parsing_result.value().first};
        timestamp_entry.ingest_timestamp(epoch_timestamp);
        return {epoch_timestamp, pattern_id};
    }

    // Fall back to consulting all known timestamp patterns
    auto const parsing_result{timestamp_parser::search_known_timestamp_patterns(
            timestamp,
            m_quoted_timestamp_patterns,
            is_json_literal,
            m_generated_pattern
    )};
    if (false == parsing_result.has_value()) {
        SPDLOG_ERROR("Failed to parse timestamp `{}` against known timestamp patterns.", timestamp);
        throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__);
    }

    auto const [epoch_timestamp, pattern] = parsing_result.value();
    timestamp_entry.ingest_timestamp(epoch_timestamp);
    auto const quoted_pattern_result{timestamp_parser::TimestampPattern::create(pattern)};
    if (quoted_pattern_result.has_error()) {
        SPDLOG_ERROR("Failed to create timestamp pattern.");
        throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__);
    }

    auto const new_pattern_id{m_next_id++};
    m_string_pattern_and_id_pairs.emplace_back(
            std::move(quoted_pattern_result.value()),
            new_pattern_id
    );
    return {epoch_timestamp, new_pattern_id};
}

auto TimestampDictionaryWriter::ingest_numeric_json_timestamp(
        std::string_view key,
        int32_t node_id,
        std::string_view timestamp
) -> std::pair<epochtime_t, uint64_t> {
    auto& [_, timestamp_entry] = *m_column_id_to_range.try_emplace(node_id, key).first;

    for (auto const& [raw_pattern, pattern_and_id] : m_numeric_pattern_to_id) {
        auto const& [pattern, id] = pattern_and_id;
        auto const parsing_result{timestamp_parser::parse_timestamp(
                timestamp,
                pattern_and_id.first,
                true,
                m_generated_pattern
        )};
        if (parsing_result.has_error()) {
            continue;
        }
        auto const epoch_timestamp{parsing_result.value().first};
        timestamp_entry.ingest_timestamp(epoch_timestamp);
        return {epoch_timestamp, pattern_and_id.second};
    }

    auto const optional_parsed_timestamp{timestamp_parser::search_known_timestamp_patterns(
            timestamp,
            m_numeric_timestamp_patterns,
            true,
            m_generated_pattern
    )};
    if (false == optional_parsed_timestamp.has_value()) {
        SPDLOG_ERROR("Failed to parse timestamp `{}` against known timestamp patterns.", timestamp);
        throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__);
    }

    auto const [epoch_timestamp, pattern] = optional_parsed_timestamp.value();
    timestamp_entry.ingest_timestamp(epoch_timestamp);
    auto const pattern_result{timestamp_parser::TimestampPattern::create(pattern)};
    if (pattern_result.has_error()) {
        SPDLOG_ERROR("Failed to create timestamp pattern.");
        throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__);
    }

    auto const new_pattern_id{m_next_id++};
    m_numeric_pattern_to_id.emplace(
            std::string{pattern},
            std::make_pair(std::move(pattern_result.value()), new_pattern_id)
    );
    return {epoch_timestamp, new_pattern_id};
}

auto TimestampDictionaryWriter::ingest_unknown_precision_epoch_timestamp(
        std::string_view key,
        int32_t node_id,
        int64_t timestamp
) -> std::pair<epochtime_t, uint64_t> {
    auto& [_, timestamp_entry] = *m_column_id_to_range.try_emplace(node_id, key).first;

    auto const [factor, precision] = timestamp_parser::estimate_timestamp_precision(timestamp);
    auto const epoch_timestamp{timestamp * factor};
    timestamp_entry.ingest_timestamp(epoch_timestamp);
    auto const pattern{fmt::format("\\{}", precision)};

    auto pattern_it{m_numeric_pattern_to_id.find(pattern)};
    if (m_numeric_pattern_to_id.end() == pattern_it) {
        auto const pattern_result{timestamp_parser::TimestampPattern::create(pattern)};
        if (pattern_result.has_error()) {
            SPDLOG_ERROR("Failed to create timestamp pattern.");
            throw OperationFailed(ErrorCodeFailure, __FILE__, __LINE__);
        }
        auto const new_pattern_id{m_next_id++};
        pattern_it
                = m_numeric_pattern_to_id
                          .emplace(
                                  std::move(pattern),
                                  std::make_pair(std::move(pattern_result.value()), new_pattern_id)
                          )
                          .first;
    }
    return {epoch_timestamp, pattern_it->second.second};
}

epochtime_t TimestampDictionaryWriter::get_begin_timestamp() const {
    auto it = m_column_id_to_range.begin();
    if (m_column_id_to_range.end() == it) {
        // replicate behaviour of CLP
        return 0;
    }

    return it->second.get_begin_timestamp();
}

epochtime_t TimestampDictionaryWriter::get_end_timestamp() const {
    auto it = m_column_id_to_range.begin();
    if (m_column_id_to_range.end() == it) {
        // replicate behaviour of CLP
        return 0;
    }

    return it->second.get_end_timestamp();
}

void TimestampDictionaryWriter::clear() {
    m_next_id = 0;
    m_string_pattern_and_id_pairs.clear();
    m_numeric_pattern_to_id.clear();
    m_column_id_to_range.clear();
}
}  // namespace clp_s
