#include "ClppMatcher.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ArchiveReader.hpp>
#include <clp_s/Defs.hpp>
#include <clpp/Defs.hpp>
#include <clpp/Interpretation.hpp>

namespace clp_s::search {
namespace {
/**
 * Removes the leaf queries whose match is `*`, since they don't constrain the leaf's value and so
 * don't need to be represented as filter expressions downstream.
 * @param leaf_queries The leaf queries to filter in place.
 */
auto erase_unconstrained_leaves(std::vector<clpp::LeafQuery>& leaf_queries) -> void;

auto erase_unconstrained_leaves(std::vector<clpp::LeafQuery>& leaf_queries) -> void {
    std::erase_if(leaf_queries, [](clpp::LeafQuery const& leaf) -> bool {
        return "*" == leaf.m_query;
    });
}
}  // namespace

ClppMatcher::ClppMatcher(ArchiveReader* archive_reader, bool case_sensitive)
        : m_archive_reader{archive_reader},
          m_case_sensitive{case_sensitive} {
    if (ClpFlavour::Clpp != m_archive_reader->flavour()) {
        return;
    }
    auto const log_shape_dict{m_archive_reader->get_log_shape_dictionary()};
    if (nullptr == log_shape_dict) {
        throw std::runtime_error{"ClppMatcher got a null log shape dictionary"};
    }

    m_structurized_texts_per_log_shape.resize(log_shape_dict->get_entries().size());
    for (auto const& [schema_id, schema] : *m_archive_reader->get_schema_map()) {
        std::unordered_map<mpt_node_id_t, size_t> next_occurrence_idx;
        for (auto const& [structurized_text_id, log_shape_id] :
             schema.get_view().find_structurized_texts())
        {
            if (log_shape_id >= m_structurized_texts_per_log_shape.size()) {
                throw std::runtime_error{
                        "ClppMatcher found a schema referencing an unknown log shape ID"
                };
            }
            m_structurized_texts_per_log_shape.at(log_shape_id)
                    .push_back(
                            StructurizedTextRef{
                                    .schema_id = schema_id,
                                    .structurized_text_id = structurized_text_id,
                                    .occurrence_idx = next_occurrence_idx[structurized_text_id]++
                            }
                    );
        }
    }
}

auto ClppMatcher::get_occurrences(
        clpp::log_shape_id_t log_shape_id,
        mpt_node_id_t structurized_text_id
) const -> InterpretationMatch::Occurrences {
    InterpretationMatch::Occurrences occurrences;
    for (auto const& ref : m_structurized_texts_per_log_shape.at(log_shape_id)) {
        if (ref.structurized_text_id == structurized_text_id) {
            occurrences[ref.schema_id].push_back(ref.occurrence_idx);
        }
    }
    return occurrences;
}

auto ClppMatcher::find_matching_schemas(
        mpt_node_id_t structurized_text_id,
        std::string_view rule_name,
        std::optional<std::string_view> shape_query
) const -> std::unordered_set<int32_t> {
    std::unordered_set<int32_t> schema_ids;
    for_each_matching_occurrence(
            structurized_text_id,
            rule_name,
            shape_query,
            [&](InterpretationMatch::Occurrences const& occurrences, size_t) -> void {
                for (auto const& [schema_id, _] : occurrences) {
                    schema_ids.emplace(schema_id);
                }
            }
    );
    return schema_ids;
}

auto ClppMatcher::decompose_query(
        mpt_node_id_t structurized_text_id,
        std::string_view query,
        std::string_view rule_name
) -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
    if (rule_name.empty()) {
        return decompose_by_log_shapes(structurized_text_id, query);
    }
    return decompose_by_rule_name(structurized_text_id, query, rule_name);
}

auto ClppMatcher::ensure_parser() -> ystdlib::error_handling::Result<void> {
    if (nullptr != m_parsing_spec) {
        return ystdlib::error_handling::success();
    }
    auto const spec{YSTDLIB_ERROR_HANDLING_TRYX(m_archive_reader->read_parsing_spec())};
    m_parsing_spec = std::make_unique<log_surgeon::ParsingSpec>(
            log_surgeon::ParsingSpecBuilder{spec}.build()
    );
    return ystdlib::error_handling::success();
}

auto
ClppMatcher::decompose_by_log_shapes(mpt_node_id_t structurized_text_id, std::string_view query)
        -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
    std::vector<log_surgeon::CCharArray> log_shapes;
    auto const& entries{m_archive_reader->get_log_shape_dictionary()->get_entries()};
    log_shapes.reserve(entries.size());
    for (auto const& log_shape : entries) {
        log_shapes.emplace_back(log_surgeon::CCharArray::from_string_view(log_shape.get_value()));
    }

    YSTDLIB_ERROR_HANDLING_TRYV(ensure_parser());
    auto interpretations_per_shape{YSTDLIB_ERROR_HANDLING_TRYX(
            clpp::decompose_by_log_shapes(*m_parsing_spec, query, log_shapes)
    )};
    std::vector<InterpretationMatch> matches;
    matches.reserve(interpretations_per_shape.size());
    for (clpp::log_shape_id_t log_shape_id{0}; log_shape_id < interpretations_per_shape.size();
         ++log_shape_id)
    {
        auto const occurrences{get_occurrences(log_shape_id, structurized_text_id)};
        if (occurrences.empty()) {
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
            matches.push_back({.occurrences{occurrences}, .leaf_queries{}});
            continue;
        }
        for (auto& leaf_queries : interpretations_per_shape.at(log_shape_id)) {
            matches.push_back({.occurrences{occurrences}, .leaf_queries{std::move(leaf_queries)}});
        }
    }
    return matches;
}

auto ClppMatcher::decompose_by_rule_name(
        mpt_node_id_t structurized_text_id,
        std::string_view query,
        std::string_view rule_name
) -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>> {
    YSTDLIB_ERROR_HANDLING_TRYV(ensure_parser());
    auto interpretations{YSTDLIB_ERROR_HANDLING_TRYX(
            clpp::decompose_by_rule_name(*m_parsing_spec, query, rule_name)
    )};
    std::vector<InterpretationMatch> matches;
    matches.reserve(interpretations.size());
    for (auto& interpretation : interpretations) {
        erase_unconstrained_leaves(interpretation.m_leaf_queries);
        std::map<size_t, InterpretationMatch::Occurrences> occurrences_by_leaf_position;
        for_each_matching_occurrence(
                structurized_text_id,
                rule_name,
                interpretation.m_shape_query.view(),
                [&](InterpretationMatch::Occurrences const& occurrences,
                    size_t leaf_position) -> void {
                    // Unconstrained interpretations are satisfied by any matching occurrence, so
                    // their occurrences are collected under a single position.
                    auto const key{interpretation.m_leaf_queries.empty() ? 0 : leaf_position};
                    auto& grouped_occurrences{occurrences_by_leaf_position[key]};
                    for (auto const& [schema_id, occurrence_indices] : occurrences) {
                        auto& indices{grouped_occurrences[schema_id]};
                        indices.insert(
                                indices.end(),
                                occurrence_indices.begin(),
                                occurrence_indices.end()
                        );
                    }
                }
        );
        for (auto& [leaf_position, occurrences] : occurrences_by_leaf_position) {
            // A schema's occurrence can be collected several times when a log shape contains
            // multiple matching parent rule occurrences at the same leaf position.
            for (auto& indices : occurrences | std::views::values) {
                std::ranges::sort(indices);
                auto const [duplicates_begin, duplicates_end]{std::ranges::unique(indices)};
                indices.erase(duplicates_begin, duplicates_end);
            }
            auto leaf_queries{interpretation.m_leaf_queries};
            for (auto& leaf : leaf_queries) {
                leaf.m_leaf_position += leaf_position;
            }
            matches.push_back(
                    {.occurrences{std::move(occurrences)}, .leaf_queries{std::move(leaf_queries)}}
            );
        }
    }
    return matches;
}
}  // namespace clp_s::search
