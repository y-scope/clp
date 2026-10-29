#ifndef CLP_S_TIMESTAMPDICTIONARYREADER_HPP
#define CLP_S_TIMESTAMPDICTIONARYREADER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <clp_s/timestamp_parser/TimestampParser.hpp>

#include "FileReader.hpp"
#include "TimestampEntry.hpp"
#include "TimestampPattern.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
class TimestampDictionaryReader {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Methods
    /**
     * Reads the timestamp dictionary from a decompressor.
     * @param decompressor
     * @param has_deprecated_timestamp_format
     * @return ErrorCodeSuccess on success, and the relevant ErrorCode otherwise
     */
    [[nodiscard]] auto read(ZstdDecompressor& decompressor, bool has_deprecated_timestamp_format)
            -> ErrorCode;

    /**
     * Gets the string encoding for a given epoch and format ID by interpreting the pattern
     * identified by `format_id` as a `clp_s::TimestampPattern`.
     * @param epoch
     * @param format_id
     * @return The string encoding for the given epoch and format ID.
     */
    [[nodiscard]] auto
    get_deprecated_timestamp_string_encoding(epochtime_t epoch, uint64_t format_id) const
            -> std::string;

    /**
     * Marshals and appends the `timestamp` to the `buffer` by interpreting the timestamp pattern
     * referenced by `format_id` as a `clp_s::timestamp_parser::TimestampPattern`.
     * @param timestamp
     * @param format_id
     * @param buffer
     * @throws OperationFailed if the format indicated by `format_id` cannot be interpreted as a
     * `clp_s::timestamp_parser::TimestampPattern`.
     */
    void append_timestamp_to_buffer(
            epochtime_t timestamp,
            uint64_t format_id,
            std::string& buffer
    ) const;

    /**
     * Gets iterators for the column to range mappings
     * @return begin and end iterators for the column to range mappings
     */
    auto tokenized_column_to_range_begin() const { return m_tokenized_column_to_range.begin(); }

    auto tokenized_column_to_range_end() const { return m_tokenized_column_to_range.end(); }

    std::optional<std::pair<std::vector<std::string>, std::string>>&
    get_authoritative_timestamp_tokenized_column() {
        return m_authoritative_timestamp_tokenized_column;
    }

    std::unordered_set<int32_t>& get_authoritative_timestamp_column_ids() {
        return m_authoritative_timestamp_column_ids;
    }

private:
    using tokenized_column_to_range_t
            = std::vector<std::pair<std::vector<std::string>, TimestampEntry*>>;

    // Variables
    std::unordered_map<uint64_t, TimestampPattern> m_deprecated_patterns;
    std::unordered_map<uint64_t, timestamp_parser::TimestampPattern> m_timestamp_patterns;
    std::vector<TimestampEntry> m_entries;
    tokenized_column_to_range_t m_tokenized_column_to_range;

    std::optional<std::pair<std::vector<std::string>, std::string>>
            m_authoritative_timestamp_tokenized_column;
    std::unordered_set<int32_t> m_authoritative_timestamp_column_ids;
};
}  // namespace clp_s

#endif  // CLP_S_TIMESTAMPDICTIONARYREADER_HPP
