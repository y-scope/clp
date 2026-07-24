#include "aggregators.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include <clp_s/archive_constants.hpp>
#include <clp_s/int_float_compare.hpp>
#include <clp_s/search/ast/SearchUtils.hpp>

using std::string;
using std::string_view;

namespace clp_s {
namespace {
/**
 * Tokenizes an aggregation's target field into its key path.
 * @param field
 * @param field_path Returns the path's keys.
 * @throws std::invalid_argument if `field` is malformed, or if it names a non-default namespace.
 */
auto tokenize_aggregation_field(string_view field, std::vector<string>& field_path) -> void {
    string descriptor_namespace;
    if (false
        == search::ast::tokenize_column_descriptor(string{field}, field_path, descriptor_namespace))
    {
        throw std::invalid_argument("Invalid aggregation field: " + string{field});
    }
    if (false == descriptor_namespace.empty()) {
        throw std::invalid_argument(
                "The aggregation field must be in the default namespace; namespaced fields (e.g. "
                "the auto-generated \"@\" namespace) are not supported."
        );
    }
}

/**
 * Parses `message` as JSON and locates the value at `field_path`.
 * @param message A matched record, marshalled to a JSON string.
 * @param field_path
 * @param doc Returns the parsed document.
 * @return The value at `field_path`.
 * @return nullptr if `message` isn't valid JSON, or if `field_path` doesn't resolve to a value.
 */
auto
find_field_value(string_view message, std::vector<string> const& field_path, nlohmann::json& doc)
        -> nlohmann::json const* {
    try {
        doc = nlohmann::json::parse(message);
    } catch (nlohmann::json::exception const&) {
        return nullptr;
    }

    nlohmann::json const* current{&doc};
    for (auto const& key : field_path) {
        if (false == current->is_object()) {
            return nullptr;
        }
        auto const it{current->find(key)};
        if (current->end() == it) {
            return nullptr;
        }
        current = &it.value();
    }
    return current;
}

/**
 * Converts a scalar JSON value to an `AggregationValue`.
 * @param value
 * @return `value` as an `AggregationValue` if it's an integer, float, string, or boolean.
 * @return std::nullopt otherwise.
 */
auto to_aggregation_value(nlohmann::json const& value) -> std::optional<AggregationValue> {
    if (value.is_number_integer()) {
        return value.get<int64_t>();
    }
    if (value.is_number_float()) {
        return value.get<double>();
    }
    if (value.is_string()) {
        return value.get<string>();
    }
    if (value.is_boolean()) {
        return value.get<bool>();
    }
    return std::nullopt;
}
}  // namespace

auto CountAggregator::get_results() const -> std::vector<AggregationResult> {
    if (0 == m_count) {
        return {};
    }
    AggregationResult result;
    result.emplace_back(constants::results_cache::search::cCount, m_count);
    return {std::move(result)};
}

auto CountByTimeAggregator::get_results() const -> std::vector<AggregationResult> {
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

MinMaxAggregator::MinMaxAggregator(bool find_max, string_view field)
        : m_find_max{find_max},
          m_field{field} {
    tokenize_aggregation_field(field, m_field_path);
}

auto MinMaxAggregator::beats_extreme(Extreme candidate) const -> bool {
    auto const& current{m_extreme.value()};
    if (m_find_max) {
        return std::visit(
                [](auto cand, auto cur) { return is_less(cur, cand); },
                candidate,
                current
        );
    }
    return std::visit([](auto cand, auto cur) { return is_less(cand, cur); }, candidate, current);
}

auto MinMaxAggregator::add_record(string_view message, epochtime_t) -> void {
    nlohmann::json doc;
    auto const* const value{find_field_value(message, m_field_path, doc)};
    if (nullptr == value || false == value->is_number()) {
        return;
    }
    Extreme const candidate{
            value->is_number_integer() ? Extreme{value->get<int64_t>()}
                                       : Extreme{value->get<double>()}
    };
    if (false == m_extreme.has_value() || beats_extreme(candidate)) {
        m_extreme = candidate;
    }
}

auto MinMaxAggregator::get_results() const -> std::vector<AggregationResult> {
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

UniqueAggregator::UniqueAggregator(string_view field) : m_field{field} {
    tokenize_aggregation_field(field, m_field_path);
}

auto UniqueAggregator::add_record(string_view message, epochtime_t) -> void {
    nlohmann::json doc;
    auto const* const value{find_field_value(message, m_field_path, doc)};
    if (nullptr == value) {
        return;
    }
    auto aggregation_value{to_aggregation_value(*value)};
    if (aggregation_value.has_value()) {
        m_values.emplace(std::move(aggregation_value.value()));
    }
}

auto UniqueAggregator::get_results() const -> std::vector<AggregationResult> {
    std::vector<AggregationResult> results;
    results.reserve(m_values.size());
    for (auto const& value : m_values) {
        AggregationResult result;
        result.emplace_back(constants::results_cache::search::cField, m_field);
        result.emplace_back(constants::results_cache::search::cValue, value);
        results.push_back(std::move(result));
    }
    return results;
}
}  // namespace clp_s
