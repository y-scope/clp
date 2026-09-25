#include "ClppMatcher.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ArchiveReader.hpp>
#include <clpp/Defs.hpp>
#include <clpp/Interpretation.hpp>

namespace clp_s::search {
namespace {
/**
 * Removes the leaf queries whose match is `*`, since they don't constrain the leaf's value and so
 * don't need to be represented as filter expressions downstream.
 */
auto erase_unconstrained_leaves(std::vector<clpp::LeafQuery>& leaf_queries) -> void {
    std::erase_if(leaf_queries, [](clpp::LeafQuery const& leaf) -> bool {
        return "*" == leaf.m_query;
    });
}
}  // namespace

ClppMatcher::ClppMatcher(ArchiveReader* archive_reader, bool case_sensitive)
        : m_archive_reader{archive_reader},
          m_case_sensitive{case_sensitive} {
    if (false == m_archive_reader->experimental()) {
        return;
    }
    auto const log_shape_dict{m_archive_reader->get_log_shape_dictionary()};
    if (nullptr == log_shape_dict) {
        throw std::runtime_error{"ClppMatcher got a null log shape dictionary"};
    }

    m_schemas_by_log_shape.resize(log_shape_dict->get_entries().size());
    for (auto const& [schema_id, schema] : *m_archive_reader->get_schema_map()) {
        if (auto const log_shape_id{schema.get_view().find_log_shape_id()}) {
            if (*log_shape_id >= m_schemas_by_log_shape.size()) {
                throw std::runtime_error{
                        "ClppMatcher found a schema referencing an unknown log shape ID"
                };
            }
            m_schemas_by_log_shape.at(*log_shape_id).emplace(schema_id);
        }
    }
}

auto ClppMatcher::find_matching_schemas(
        std::string_view rule_name,
        std::optional<std::string_view> shape_query
) const -> std::unordered_set<int32_t> {
    std::unordered_set<int32_t> schema_ids;
    for_each_matching_occurrence(
            rule_name,
            shape_query,
            [&](std::unordered_set<int32_t> const& occurrence_schema_ids, size_t) -> void {
                schema_ids.insert(occurrence_schema_ids.begin(), occurrence_schema_ids.end());
            }
    );
    return schema_ids;
}

auto ClppMatcher::decompose_query(std::string_view query, std::string_view rule_name)
        -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
    if (rule_name.empty()) {
        return decompose_by_log_shapes(query);
    }
    return decompose_by_rule_name(query, rule_name);
}

auto ClppMatcher::ensure_parser() -> ystdlib::error_handling::Result<void> {
    if (nullptr != m_parser) {
        return ystdlib::error_handling::success();
    }
    auto const spec{YSTDLIB_ERROR_HANDLING_TRYX(m_archive_reader->read_parsing_spec())};
    m_parser = std::make_unique<log_surgeon::Parser>(log_surgeon::ParsingSpecBuilder{spec}.build());
    return ystdlib::error_handling::success();
}

auto ClppMatcher::decompose_by_log_shapes(std::string_view query)
        -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
    std::vector<std::string_view> log_shapes;
    auto const& entries{m_archive_reader->get_log_shape_dictionary()->get_entries()};
    log_shapes.reserve(entries.size());
    for (auto const& log_shape : entries) {
        log_shapes.emplace_back(log_shape.get_value());
    }

    YSTDLIB_ERROR_HANDLING_TRYV(ensure_parser());
    auto interpretations_per_shape{clpp::decompose_by_log_shapes(*m_parser, query, log_shapes)};
    std::vector<InterpretationMatch> matches;
    matches.reserve(interpretations_per_shape.size());
    for (clpp::log_shape_id_t log_shape_id{0}; log_shape_id < interpretations_per_shape.size();
         ++log_shape_id)
    {
        auto const& schema_ids{m_schemas_by_log_shape.at(log_shape_id)};
        if (schema_ids.empty()) {
            continue;
        }
        for (auto& leaf_queries : interpretations_per_shape.at(log_shape_id)) {
            erase_unconstrained_leaves(leaf_queries);
        }
        // If any interpretation leaves no value constraints, the shape satisfies the query
        // unconditionally, so every event in this shape's schemas matches and the other
        // interpretations are redundant.
        if (std::ranges::any_of(
                    interpretations_per_shape.at(log_shape_id),
                    [](std::vector<clpp::LeafQuery> const& leaf_queries) -> bool {
                        return leaf_queries.empty();
                    }
            ))
        {
            matches.push_back({.schema_ids{schema_ids}, .leaf_queries{}});
            continue;
        }
        for (auto& leaf_queries : interpretations_per_shape.at(log_shape_id)) {
            matches.push_back({.schema_ids{schema_ids}, .leaf_queries{std::move(leaf_queries)}});
        }
    }
    return matches;
}

auto ClppMatcher::decompose_by_rule_name(std::string_view query, std::string_view rule_name)
        -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
    YSTDLIB_ERROR_HANDLING_TRYV(ensure_parser());
    auto interpretations{clpp::decompose_by_rule_name(*m_parser, query, rule_name)};
    std::vector<InterpretationMatch> matches;
    matches.reserve(interpretations.size());
    for (auto& interpretation : interpretations) {
        erase_unconstrained_leaves(interpretation.m_leaf_queries);
        std::map<size_t, std::unordered_set<int32_t>> schema_ids_by_leaf_position;
        for_each_matching_occurrence(
                rule_name,
                interpretation.m_shape_query.view(),
                [&](std::unordered_set<int32_t> const& schema_ids, size_t leaf_position) -> void {
                    // Unconstrained interpretations are satisfied by any matching occurrence, so
                    // their schemas are collected under a single position.
                    auto const key{interpretation.m_leaf_queries.empty() ? 0 : leaf_position};
                    schema_ids_by_leaf_position[key].insert(schema_ids.begin(), schema_ids.end());
                }
        );
        for (auto& [leaf_position, schema_ids] : schema_ids_by_leaf_position) {
            auto leaf_queries{interpretation.m_leaf_queries};
            for (auto& leaf : leaf_queries) {
                leaf.m_leaf_position += leaf_position;
            }
            matches.push_back(
                    {.schema_ids{std::move(schema_ids)}, .leaf_queries{std::move(leaf_queries)}}
            );
        }
    }
    return matches;
}
}  // namespace clp_s::search
