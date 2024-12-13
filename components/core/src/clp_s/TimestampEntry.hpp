#ifndef CLP_S_TIMESTAMPENTRY_HPP
#define CLP_S_TIMESTAMPENTRY_HPP

#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "Defs.hpp"
#include "ErrorCode.hpp"
#include "search/FilterOperation.hpp"
#include "Utils.hpp"
#include "ZstdDecompressor.hpp"

using clp_s::search::FilterOperation;

namespace clp_s {
class TimestampEntry {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "TimestampEntry operation failed"; }
    };

    // Constants
    enum TimestampEncoding : uint64_t {
        UnkownTimestampEncoding,
        Epoch,
        DoubleEpoch
    };

    // Constructors
    TimestampEntry()
            : m_encoding(UnkownTimestampEncoding),
              m_epoch_start_double(cDoubleEpochTimeMax),
              m_epoch_end_double(cDoubleEpochTimeMin),
              m_epoch_start(cEpochTimeMax),
              m_epoch_end(cEpochTimeMin) {}

    TimestampEntry(std::string_view key_name)
            : m_encoding(UnkownTimestampEncoding),
              m_epoch_start_double(cDoubleEpochTimeMax),
              m_epoch_end_double(cDoubleEpochTimeMin),
              m_epoch_start(cEpochTimeMax),
              m_epoch_end(cEpochTimeMin),
              m_key_name(key_name) {}

    /**
     * Ingest a timestamp potentially adjusting the start and end bounds for this
     * TimestampEntry.
     * @param timestamp the timestamp to be ingested
     */
    void ingest_timestamp(epochtime_t timestamp);
    void ingest_timestamp(double timestamp);

    /**
     * Merge a timestamp range potentially adjusting the start and end bounds for this
     * @param timestamp the timestamp to be ingested
     */
    void merge_range(TimestampEntry const& entry);

    /**
     * Write the timestamp entry to a buffered stream.
     * @param compressor
     */
    void write_to_stream(std::stringstream& stream) const;

    /**
     * Try to read the timestamp entry from a file
     * @param decompressor
     * @return ErrorCode
     */
    ErrorCode try_read_from_file(ZstdDecompressor& decompressor);

    /**
     * Read the timestamp entry from a file
     * @param decompressor
     */
    void read_from_file(ZstdDecompressor& decompressor);

    /**
     * Check if a timestamp is in the range of this TimestampEntry
     * @param op
     * @param timestamp
     * @return
     */
    EvaluatedValue evaluate_filter(FilterOperation op, double timestamp);
    EvaluatedValue evaluate_filter(FilterOperation op, epochtime_t timestamp);

    std::string get_key_name() const { return m_key_name; }

    std::unordered_set<int32_t> const& get_column_ids() const { return m_column_ids; }

    void insert_column_id(int32_t column_id) { m_column_ids.insert(column_id); }

    void insert_column_ids(std::unordered_set<int32_t> const& column_ids) {
        m_column_ids.insert(column_ids.begin(), column_ids.end());
    }

    /**
     * TODO: guarantee epoch milliseconds. The current clp-s approach to encoding timestamps and
     * timestamp ranges makes no effort to convert second and nanosecond encoded timestamps into
     * millisecond encoded timestamps.
     * @return the beginning of the time range as milliseconds since the UNIX epoch
     */
    epochtime_t get_begin_timestamp() const;

    /**
     * TODO: guarantee epoch milliseconds. The current clp-s approach to encoding timestamps and
     * timestamp ranges makes no effort to convert second and nanosecond encoded timestamps into
     * millisecond encoded timestamps.
     * @return the end of the time range as milliseconds since the UNIX epoch
     */
    epochtime_t get_end_timestamp() const;

private:
    TimestampEncoding m_encoding;
    double m_epoch_start_double, m_epoch_end_double;
    epochtime_t m_epoch_start, m_epoch_end;

    std::string m_key_name;
    std::unordered_set<int32_t> m_column_ids;
};
}  // namespace clp_s

#endif  // CLP_S_TIMESTAMPENTRY_HPP
