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
 * Tokenizes an aggregation's target field into its path components.
 * @param field
 * @param field_path Returns the tokenized path components.
 * @throws std::invalid_argument if `field` cannot be tokenized, or if it is in a non-default
 * namespace.
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
 * Parses `message` as JSON and navigates to the node at `field_path`.
 * @param message
 * @param field_path
 * @param doc Returns the parsed document, which owns the returned node.
 * @return A pointer, valid for the lifetime of `doc`, to the node at `field_path`.
 * @return nullptr if `message` is not valid JSON, or if any path component is missing or traverses
 * a non-object.
 */
auto
find_field_node(string_view message, std::vector<string> const& field_path, nlohmann::json& doc)
        -> nlohmann::json const* {
    try {
        doc = nlohmann::json::parse(message);
    } catch (nlohmann::json::exception const&) {
        return nullptr;
    }

    nlohmann::json const* node{&doc};
    for (auto const& key : field_path) {
        if (false == node->is_object()) {
            return nullptr;
        }
        auto const it{node->find(key)};
        if (node->end() == it) {
            return nullptr;
        }
        node = &it.value();
    }
    return node;
}

/**
 * Extracts a scalar node's value as an `AggregationValue`.
 * @param node
 * @return The node's value if it is an integer, float, string, or boolean.
 * @return std::nullopt otherwise.
 */
auto node_to_aggregation_value(nlohmann::json const& node) -> std::optional<AggregationValue> {
    if (node.is_number_integer()) {
        return node.get<int64_t>();
    }
    if (node.is_number_float()) {
        return node.get<double>();
    }
    if (node.is_string()) {
        return node.get<string>();
    }
    if (node.is_boolean()) {
        return node.get<bool>();
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
    auto const* const node{find_field_node(message, m_field_path, doc)};
    if (nullptr == node || false == node->is_number()) {
        return;
    }
    Extreme const candidate{
            node->is_number_integer() ? Extreme{node->get<int64_t>()} : Extreme{node->get<double>()}
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
    auto const* const node{find_field_node(message, m_field_path, doc)};
    if (nullptr == node) {
        return;
    }
    auto value{node_to_aggregation_value(*node)};
    if (value.has_value()) {
        m_values.emplace(std::move(value.value()));
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
