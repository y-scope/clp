#include "Aggregation.hpp"

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
        // A malformed field tokenizes to nothing, so `add_record` never matches and min/max yields
        // no result. The field is validated during command-line parsing, so this is just defensive.
        m_field_path.clear();
        return;
    }
    if (false == descriptor_namespace.empty()) {
        // Namespaced fields (e.g. the auto-generated `@` namespace) are marshalled under a
        // top-level object keyed by the namespace string, so navigate into it first.
        m_field_path.insert(m_field_path.begin(), descriptor_namespace);
    }
}

auto MinMaxAggregation::beats_extreme(Extreme candidate) const -> bool {
    auto const& current{m_extreme.value()};
    // Compare integers exactly; only fall back to a (lossy) double comparison when the types
    // differ.
    if (std::holds_alternative<int64_t>(candidate) && std::holds_alternative<int64_t>(current)) {
        auto const lhs{std::get<int64_t>(candidate)};
        auto const rhs{std::get<int64_t>(current)};
        return m_find_max ? lhs > rhs : lhs < rhs;
    }
    auto const as_double{[](Extreme value) {
        return std::visit([](auto held) { return static_cast<double>(held); }, value);
    }};
    return m_find_max ? as_double(candidate) > as_double(current)
                      : as_double(candidate) < as_double(current);
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
