#ifndef CLP_S_TIMESTAMPDICTIONARYWRITER_HPP
#define CLP_S_TIMESTAMPDICTIONARYWRITER_HPP

#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <clp_s/timestamp_parser/TimestampParser.hpp>

#include "SchemaTree.hpp"
#include "TimestampEntry.hpp"
#include "TraceableException.hpp"

namespace clp_s {
class TimestampDictionaryWriter {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    explicit TimestampDictionaryWriter() {
        auto quoted_patterns_result{timestamp_parser::get_all_default_quoted_timestamp_patterns()};
        auto numeric_patterns_result{timestamp_parser::get_default_numeric_timestamp_patterns()};
        if (quoted_patterns_result.has_error() || numeric_patterns_result.has_error()) {
            throw OperationFailed(ErrorCode::ErrorCodeFailure, __FILENAME__, __LINE__);
        }
        m_quoted_timestamp_patterns = std::move(quoted_patterns_result.value());
        m_numeric_timestamp_patterns = std::move(numeric_patterns_result.value());
    }

    /**
     * Writes the timestamp dictionary to a buffered stream.
     * @param stream
     */
    void write(std::stringstream& stream);

    /**
     * Ingests a timestamp entry from a string.
     * @param key
     * @param node_id
     * @param timestamp
     * @param is_json_literal
     * @return A pair containing:
     * - The timestamp in epoch nanoseconds.
     * - The pattern ID corresponding to the timestamp format.
     */
    [[nodiscard]] auto ingest_string_timestamp(
            std::string_view key,
            int32_t node_id,
            std::string_view timestamp,
            bool is_json_literal
    ) -> std::pair<epochtime_t, uint64_t>;

    /**
     * Ingests a numeric JSON entry.
     * @param key
     * @param node_id
     * @param timestamp
     * @return A pair containing:
     * - The timestamp in epoch nanoseconds.
     * - The pattern ID corresponding to the timestamp format.
     */
    [[nodiscard]] auto
    ingest_numeric_json_timestamp(std::string_view key, int32_t node_id, std::string_view timestamp)
            -> std::pair<epochtime_t, uint64_t>;

    /**
     * Ingests an unknown precision epoch timestamp.
     * @param key
     * @param node_id
     * @param timestamp
     * @return A pair containing:
     * - The timestamp in epoch nanoseconds.
     * - The pattern ID corresponding to the timestamp format.
     */
    [[nodiscard]] auto ingest_unknown_precision_epoch_timestamp(
            std::string_view key,
            int32_t node_id,
            int64_t timestamp
    ) -> std::pair<epochtime_t, uint64_t>;

    /**
     * TODO: guarantee epoch milliseconds. The current clp-s approach to encoding timestamps and
     * timestamp ranges makes no effort to convert second and nanosecond encoded timestamps into
     * millisecond encoded timestamps.
     * @return the beginning of this archive's time range as milliseconds since the UNIX epoch
     */
    epochtime_t get_begin_timestamp() const;

    /**
     * TODO: guarantee epoch milliseconds. The current clp-s approach to encoding timestamps and
     * timestamp ranges makes no effort to convert second and nanosecond encoded timestamps into
     * millisecond encoded timestamps.
     * @return the end of this archive's time range as milliseconds since the UNIX epoch
     */
    epochtime_t get_end_timestamp() const;

    /**
     * Clears and resets all internal state.
     */
    void clear();

private:
    using pattern_to_id_t = absl::flat_hash_map<std::string, uint64_t>;

    // Variables
    std::vector<std::pair<timestamp_parser::TimestampPattern, uint64_t>> m_string_patterns_and_ids;
    absl::flat_hash_map<std::string, std::pair<timestamp_parser::TimestampPattern, uint64_t>>
            m_numeric_pattern_to_id;
    uint64_t m_next_id{};

    std::unordered_map<int32_t, TimestampEntry> m_column_id_to_range;

    std::vector<timestamp_parser::TimestampPattern> m_quoted_timestamp_patterns;
    std::vector<timestamp_parser::TimestampPattern> m_numeric_timestamp_patterns;
};
}  // namespace clp_s

#endif  // CLP_S_TIMESTAMPDICTIONARYWRITER_HPP
