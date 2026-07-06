#ifndef CLP_S_AGGREGATION_HPP
#define CLP_S_AGGREGATION_HPP

#include <concepts>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <clp_s/Defs.hpp>

namespace clp_s {
/**
 * A single typed value in an aggregation result document.
 */
using AggregationValue = std::variant<int64_t, double, std::string, bool>;

/**
 * One aggregation result document: an ordered list of typed key-value pairs.
 */
using AggregationResult = std::vector<std::pair<std::string, AggregationValue>>;

/**
 * Requirements a type must satisfy to be used as an aggregator.
 * @tparam AggregatorType The type of the aggregator.
 */
template <typename AggregatorType>
concept AggregatorReq
        = requires(AggregatorType aggregator, std::string_view message, epochtime_t timestamp_ms) {
              /**
               * Adds a record to the aggregate.
               * @param message The message in the log event.
               * @param timestamp_ms The timestamp of the log event.
               */
              { aggregator.add_record(message, timestamp_ms) } -> std::same_as<void>;

              /**
               * Gets the aggregate's results.
               * @return The result documents produced by the aggregation.
               */
              { aggregator.get_results() } -> std::same_as<std::vector<AggregationResult>>;

              /**
               * Whether the caller must supply per-record metadata.
               */
              { AggregatorType::cNeedsMetadata } -> std::convertible_to<bool>;

              /**
               * Whether the caller must supply the marshalled record.
               */
              { AggregatorType::cNeedsMarshalledRecord } -> std::convertible_to<bool>;
          };

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

    explicit CountByTimeAggregation(int64_t bucket_size_ms) : m_bucket_size_ms{bucket_size_ms} {
        if (bucket_size_ms <= 0) {
            throw std::invalid_argument("CountByTimeAggregation bucket size must be positive.");
        }
    }

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
 * Collects the distinct values of a target field across matched records.
 */
class UniqueAggregation {
public:
    static constexpr bool cNeedsMetadata = false;
    static constexpr bool cNeedsMarshalledRecord = true;

    explicit UniqueAggregation(std::string_view field);

    auto add_record(std::string_view message, [[maybe_unused]] epochtime_t timestamp_ms) -> void;

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    std::string m_field;
    std::vector<std::string> m_field_path;
    std::set<AggregationValue> m_values;
};

/**
 * One of the supported aggregations that a search can apply to its matched records.
 */
using Aggregation = std::
        variant<CountAggregation, CountByTimeAggregation, MinMaxAggregation, UniqueAggregation>;
}  // namespace clp_s

#endif  // CLP_S_AGGREGATION_HPP
