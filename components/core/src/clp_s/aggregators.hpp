#ifndef CLP_S_AGGREGATORS_HPP
#define CLP_S_AGGREGATORS_HPP

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
 * A single typed value in an aggregation's result document.
 */
using AggregationValue = std::variant<int64_t, double, std::string, bool>;

/**
 * One aggregation's result document: an ordered list of typed key-value pairs.
 */
using AggregationResult = std::vector<std::pair<std::string, AggregationValue>>;

/**
 * Requirements a type must satisfy to be used as an aggregator.
 * @tparam AggregatorType
 */
template <typename AggregatorType>
concept AggregatorReq = requires(
        AggregatorType aggregator,
        std::string_view message,
        epochtime_t timestamp_millisecs
) {
    /**
     * Adds a record to the aggregate.
     * @param message The message in the log event.
     * @param timestamp_millisecs The timestamp of the log event.
     */
    { aggregator.add_record(message, timestamp_millisecs) } -> std::same_as<void>;

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
class CountAggregator {
public:
    // Static constants
    static constexpr bool cNeedsMetadata{false};
    static constexpr bool cNeedsMarshalledRecord{false};

    // Methods
    auto add_record(
            [[maybe_unused]] std::string_view message,
            [[maybe_unused]] epochtime_t timestamp_millisecs
    ) -> void {
        m_count += 1;
    }

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    // Data members
    int64_t m_count{};
};

/**
 * Counts the number of matched records in each time bucket of a fixed size.
 */
class CountByTimeAggregator {
public:
    // Static constants
    static constexpr bool cNeedsMetadata{true};
    static constexpr bool cNeedsMarshalledRecord{false};

    // Constructors
    explicit CountByTimeAggregator(int64_t bucket_size_millisecs)
            : m_bucket_size_millisecs{bucket_size_millisecs} {
        if (bucket_size_millisecs <= 0) {
            throw std::invalid_argument("CountByTimeAggregator bucket size must be positive.");
        }
    }

    // Methods
    [[nodiscard]] auto get_bucket_size_millisecs() const -> int64_t {
        return m_bucket_size_millisecs;
    }

    auto add_record([[maybe_unused]] std::string_view message, epochtime_t timestamp_millisecs)
            -> void {
        int64_t const bucket
                = (timestamp_millisecs / m_bucket_size_millisecs) * m_bucket_size_millisecs;
        m_bucket_counts[bucket] += 1;
    }

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    // Data members
    int64_t m_bucket_size_millisecs;
    std::map<int64_t, int64_t> m_bucket_counts;
};

/**
 * Counts the number of matched records grouped by the value of a target field.
 */
class GroupByCountAggregator {
public:
    // Static constants
    static constexpr bool cNeedsMetadata{false};
    static constexpr bool cNeedsMarshalledRecord{true};

    // Constructors
    explicit GroupByCountAggregator(std::string_view field);

    // Methods
    auto add_record(std::string_view message, [[maybe_unused]] epochtime_t timestamp_millisecs)
            -> void;

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    // Data members
    std::string m_field;
    std::vector<std::string> m_field_path;
    std::map<AggregationValue, int64_t> m_counts;
};

/**
 * Tracks the minimum or maximum value of a target field across matched records.
 */
class MinMaxAggregator {
public:
    // Static constants
    static constexpr bool cNeedsMetadata{false};
    static constexpr bool cNeedsMarshalledRecord{true};

    // Constructors
    MinMaxAggregator(bool find_max, std::string_view field);

    // Methods
    auto add_record(std::string_view message, [[maybe_unused]] epochtime_t timestamp_millisecs)
            -> void;

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    // Types
    using Extreme = std::variant<int64_t, double>;

    // Methods
    /**
     * @param candidate The value to compare against the current extreme.
     * @return Whether `candidate` is more extreme (per the min/max mode) than the current extreme.
     */
    [[nodiscard]] auto beats_extreme(Extreme candidate) const -> bool;

    // Data members
    bool m_find_max;
    std::string m_field;
    std::vector<std::string> m_field_path;
    std::optional<Extreme> m_extreme;
};

/**
 * Collects the distinct values of a target field across matched records.
 */
class UniqueAggregator {
public:
    // Static constants
    static constexpr bool cNeedsMetadata{false};
    static constexpr bool cNeedsMarshalledRecord{true};

    // Constructors
    explicit UniqueAggregator(std::string_view field);

    // Methods
    auto add_record(std::string_view message, [[maybe_unused]] epochtime_t timestamp_millisecs)
            -> void;

    [[nodiscard]] auto get_results() const -> std::vector<AggregationResult>;

private:
    // Data members
    std::string m_field;
    std::vector<std::string> m_field_path;
    std::set<AggregationValue> m_values;
};

/**
 * One of the supported aggregators that a search can apply to its matched records.
 */
using Aggregator = std::variant<
        CountAggregator,
        CountByTimeAggregator,
        GroupByCountAggregator,
        MinMaxAggregator,
        UniqueAggregator>;
}  // namespace clp_s

#endif  // CLP_S_AGGREGATORS_HPP
