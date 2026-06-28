#include "Aggregation.hpp"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "archive_constants.hpp"
#include "search/ast/SearchUtils.hpp"

using std::string;
using std::string_view;

namespace clp_s {
namespace {
// `2^63`: exactly representable as a double and one past `INT64_MAX`. Any double `>=` this exceeds
// every `int64_t`.
constexpr double cInt64UpperBound{9223372036854775808.0};
// `-2^63 == INT64_MIN`, exactly representable as a double.
constexpr double cInt64Min{-9223372036854775808.0};

[[nodiscard]] auto is_less(int64_t lhs, int64_t rhs) -> bool {
    return lhs < rhs;
}

[[nodiscard]] auto is_less(double lhs, double rhs) -> bool {
    return lhs < rhs;
}

/**
 * Determines whether an integer is strictly less than a double without precision loss. Casting
 * `lhs` to a double would be lossy above `2^53`, so we instead range-check `rhs` against the
 * `int64_t` bounds and then compare integer parts exactly, letting the fractional part of `rhs`
 * break ties.
 * @param lhs
 * @param rhs
 * @return Whether `lhs < rhs`.
 */
[[nodiscard]] auto is_less(int64_t lhs, double rhs) -> bool {
    if (std::isnan(rhs)) {
        return false;
    }
    if (rhs >= cInt64UpperBound) {
        return true;
    }
    if (rhs < cInt64Min) {
        return false;
    }
    // `rhs` is now in `[INT64_MIN, INT64_MAX]`, so `trunc(rhs)` casts to `int64_t` exactly.
    auto const truncated{std::trunc(rhs)};
    auto const rhs_int{static_cast<int64_t>(truncated)};
    if (lhs != rhs_int) {
        return lhs < rhs_int;
    }
    // Integer parts are equal; `lhs < rhs` iff `rhs` has a positive fractional part.
    return rhs > truncated;
}

/**
 * Determines whether a double is strictly less than an integer without precision loss. See the
 * `int64_t`/`double` overload for the rationale.
 * @param lhs
 * @param rhs
 * @return Whether `lhs < rhs`.
 */
[[nodiscard]] auto is_less(double lhs, int64_t rhs) -> bool {
    if (std::isnan(lhs)) {
        return false;
    }
    if (lhs >= cInt64UpperBound) {
        return false;
    }
    if (lhs < cInt64Min) {
        return true;
    }
    auto const truncated{std::trunc(lhs)};
    auto const lhs_int{static_cast<int64_t>(truncated)};
    if (lhs_int != rhs) {
        return lhs_int < rhs;
    }
    // Integer parts are equal; `lhs < rhs` iff `lhs` has a negative fractional part.
    return lhs < truncated;
}
}  // namespace

auto CountAggregation::get_results() const -> std::vector<AggregationResult> {
    if (0 == m_count) {
        return {};
    }
    AggregationResult result;
    result.emplace_back(constants::results_cache::search::cCount, m_count);
    return {std::move(result)};
}

auto CountByTimeAggregation::get_results() const -> std::vector<AggregationResult> {
    std::vector<AggregationResult> results;
    results.reserve(m_bucket_counts.size());
    for (auto const& [bucket_timestamp, count] : m_bucket_counts) {
        AggregationResult result;
        result.emplace_back(constants::results_cache::search::cTimestamp, bucket_timestamp);
        result.emplace_back(constants::results_cache::search::cCount, count);
        results.push_back(std::move(result));
    }
    return results;
}

MinMaxAggregation::MinMaxAggregation(bool find_max, string_view field)
        : m_find_max{find_max},
          m_field{field} {
    string descriptor_namespace;
    if (false
        == search::ast::tokenize_column_descriptor(
                string{field},
                m_field_path,
                descriptor_namespace
        ))
    {
        throw std::invalid_argument("Invalid --min/--max field: " + string{field});
    }
    if (false == descriptor_namespace.empty()) {
        // Namespaced fields (e.g. the auto-generated `@` namespace) are marshalled under a
        // top-level object keyed by the namespace string, so navigate into it first.
        m_field_path.insert(m_field_path.begin(), descriptor_namespace);
    }
}

auto MinMaxAggregation::beats_extreme(Extreme candidate) const -> bool {
    auto const& current{m_extreme.value()};
    // `is_less` compares every `int64_t`/`double` combination exactly (no lossy casts). A candidate
    // beats the current maximum when the current value is smaller, and the current minimum when the
    // candidate is smaller.
    if (m_find_max) {
        return std::visit([](auto cand, auto cur) { return is_less(cur, cand); }, candidate, current);
    }
    return std::visit([](auto cand, auto cur) { return is_less(cand, cur); }, candidate, current);
}

auto MinMaxAggregation::add_record(string_view message, epochtime_t) -> void {
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(message);
    } catch (nlohmann::json::exception const&) {
        return;
    }

    nlohmann::json const* node{&doc};
    for (auto const& key : m_field_path) {
        if (false == node->is_object()) {
            return;
        }
        auto const it{node->find(key)};
        if (node->end() == it) {
            return;
        }
        node = &it.value();
    }

    if (false == node->is_number()) {
        return;
    }
    Extreme const candidate{
            node->is_number_integer() ? Extreme{node->get<int64_t>()} : Extreme{node->get<double>()}
    };
    if (false == m_extreme.has_value() || beats_extreme(candidate)) {
        m_extreme = candidate;
    }
}

auto MinMaxAggregation::get_results() const -> std::vector<AggregationResult> {
    if (false == m_extreme.has_value()) {
        return {};
    }
    AggregationResult result;
    result.emplace_back(constants::results_cache::search::cField, m_field);
    auto const* const key{
            m_find_max ? constants::results_cache::search::cMax
                       : constants::results_cache::search::cMin
    };
    AggregationValue const value{
            std::visit([](auto held) -> AggregationValue { return held; }, m_extreme.value())
    };
    result.emplace_back(key, value);
    return {std::move(result)};
}

auto aggregation_needs_metadata(Aggregation const& aggregation) -> bool {
    return std::visit(
            [](auto const& agg) { return std::decay_t<decltype(agg)>::cNeedsMetadata; },
            aggregation
    );
}

auto aggregation_needs_marshalled_record(Aggregation const& aggregation) -> bool {
    return std::visit(
            [](auto const& agg) { return std::decay_t<decltype(agg)>::cNeedsMarshalledRecord; },
            aggregation
    );
}
}  // namespace clp_s
