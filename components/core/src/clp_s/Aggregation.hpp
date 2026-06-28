#ifndef CLP_S_AGGREGATION_HPP
#define CLP_S_AGGREGATION_HPP

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "Defs.hpp"

namespace clp_s {
/**
 * A single typed value in an aggregation result document.
 */
using AggregationValue = std::variant<int64_t, double, std::string>;

/**
 * One aggregation result document: an ordered list of typed key-value pairs.
 */
using AggregationResult = std::vector<std::pair<std::string, AggregationValue>>;

/**
 * Counts the number of matched records.
 */
class CountAggregation {
public:
    static constexpr bool cNeedsMetadata = false;
    static constexpr bool cNeedsMarshalledRecord = false;

    auto
    add_record([[maybe_unused]] std::string_view message, [[maybe_unused]] epochtime_t timestamp_ms)
            -> void {
        m_count += 1;
    }

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    int64_t m_count{};
};

/**
 * Counts the number of matched records in each time bucket of a fixed size.
 */
class CountByTimeAggregation {
public:
    static constexpr bool cNeedsMetadata = true;
    static constexpr bool cNeedsMarshalledRecord = false;

    explicit CountByTimeAggregation(int64_t bucket_size_ms) : m_bucket_size_ms{bucket_size_ms} {}

    [[nodiscard]] auto get_bucket_size_ms() const -> int64_t { return m_bucket_size_ms; }

    auto add_record([[maybe_unused]] std::string_view message, epochtime_t timestamp_ms) -> void {
        int64_t const bucket = (timestamp_ms / m_bucket_size_ms) * m_bucket_size_ms;
        m_bucket_counts[bucket] += 1;
    }

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    int64_t m_bucket_size_ms;
    std::map<int64_t, int64_t> m_bucket_counts;
};

/**
 * Tracks the minimum or maximum value of a target field across matched records.
 */
class MinMaxAggregation {
public:
    static constexpr bool cNeedsMetadata = false;
    static constexpr bool cNeedsMarshalledRecord = true;

    MinMaxAggregation(bool find_max, std::string_view field);

    auto add_record(std::string_view message, [[maybe_unused]] epochtime_t timestamp_ms) -> void;

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    using Extreme = std::variant<int64_t, double>;

    /**
     * @param candidate
     * @return Whether `candidate` is more extreme (per the min/max mode) than the current extreme.
     */
    [[nodiscard]] auto beats_extreme(Extreme candidate) const -> bool;

    bool m_find_max;
    std::string m_field;
    std::vector<std::string> m_field_path;
    std::optional<Extreme> m_extreme;
};

/**
 * A search aggregation to perform.
 */
using Aggregation = std::variant<CountAggregation, CountByTimeAggregation, MinMaxAggregation>;

/**
 * @param aggregation
 * @return Whether the aggregation needs per-record metadata (i.e. the timestamp).
 */
[[nodiscard]] auto aggregation_needs_metadata(Aggregation const& aggregation) -> bool;

/**
 * @param aggregation
 * @return Whether the aggregation needs each matched record marshalled into its JSON message.
 */
[[nodiscard]] auto aggregation_needs_marshalled_record(Aggregation const& aggregation) -> bool;
}  // namespace clp_s

#endif  // CLP_S_AGGREGATION_HPP
