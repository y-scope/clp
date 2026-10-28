#ifndef CLP_S_SEARCH_CLPPMATCHER_HPP
#define CLP_S_SEARCH_CLPPMATCHER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/string_utils/string_utils.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clp_s/Defs.hpp>
#include <clpp/Defs.hpp>
#include <clpp/Interpretation.hpp>
#include <clpp/TextShape.hpp>

namespace clp_s::search {
/**
 * Requirements for a callback invoked by `ClppMatcher::for_each_matching_occurrence` for each
 * occurrence (a parent rule occurrence or a whole log shape) whose shape matches the query.
 * @tparam OccurrenceMatchHandler The callback type.
 * @tparam Occurrences `ClppMatcher::InterpretationMatch::Occurrences`, taken as a parameter only
 * because the concept must be declared before `ClppMatcher`.
 * @param on_match
 * @param occurrences The per-schema same-node occurrence indices containing the matching log shape.
 * @param leaf_position The position of the matching occurrence's first leaf placeholder among the
 * leaf placeholders of the whole log shape (always 0 for a whole log shape).
 */
template <typename OccurrenceMatchHandler, typename Occurrences>
concept OccurrenceMatchHandlerReq = requires(
        OccurrenceMatchHandler const& on_match,
        Occurrences const& occurrences,
        size_t leaf_position
) { on_match(occurrences, leaf_position); };

/**
 * Utility for matching shapes and decomposing queries against them.
 */
class ClppMatcher {
public:
    // Types
    /**
     * A matched interpretation of a decomposed query, made of the per-schema same-node occurrence
     * indices containing a matching shape and any leaf queries constraining it. If `leaf_queries`
     * is empty then matching shapes satisfy the query without constraining any leaf values. Leaf
     * positions are relative to the leaf placeholders of the whole log shape.
     */
    struct InterpretationMatch {
        /**
         * Occurrence indices for each schema, relative to one StructurizedText MPT node.
         */
        using Occurrences = std::unordered_map<int32_t, std::vector<size_t>>;

        Occurrences occurrences;
        std::vector<clpp::LeafQuery> leaf_queries;
    };

    // Constructors
    /**
     * Builds an index from each log shape ID to its `StructurizedText`s to enable shape matching
     * and query decomposition. If the archive is not clpp-flavour construction is skipped and the
     * object is invalid.
     * @param archive_reader
     * @param case_sensitive Whether matching is case sensitive.
     * @throw std::runtime_error if the log shape dictionary is not valid.
     */
    ClppMatcher(ArchiveReader* archive_reader, bool case_sensitive);

    // Methods
    /**
     * Finds the schemas whose log shapes (or parent rule shapes of `rule_name`) satisfy
     * `shape_query`, considering only shapes of the `StructurizedText` node `structurized_text_id`.
     * @param structurized_text_id The `StructurizedText` node to search within.
     * @param rule_name A parent rule name, or empty to query the entire log shape.
     * @param shape_query A wildcard pattern, or std::nullopt to match every shape.
     * @return The matching schema IDs.
     */
    [[nodiscard]] auto find_matching_schemas(
            mpt_node_id_t structurized_text_id,
            std::string_view rule_name,
            std::optional<std::string_view> shape_query
    ) const -> std::unordered_set<int32_t>;

    /**
     * Decomposes `query` against the parent rule `rule_name`, or against every log shape if
     * `rule_name` is empty, then returns the possible interpretations. Only shapes of the
     * `StructurizedText` node `structurized_text_id` are considered.
     * @param structurized_text_id The `StructurizedText` node to search within.
     * @param query
     * @param rule_name A qualified parent rule name, or empty to decompose against every log shape.
     * @return The interpretation matches (empty if no interpretation matched a shape), or an error
     * code indicating the failure:
     * - Forwards `decompose_by_log_shapes`'s return values.
     * - Forwards `decompose_by_rule_name`'s return values.
     */
    [[nodiscard]] auto decompose_query(
            mpt_node_id_t structurized_text_id,
            std::string_view query,
            std::string_view rule_name
    ) -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>>;

private:
    // Types
    /**
     * A reference to a `StructurizedText` occurrence within a schema, identified by the schema ID
     * and the `StructurizedText` node's MPT node ID, plus its zero-based same-node occurrence index
     * in schema document order. Arrays can repeat the same MPT node, while sibling text fields can
     * use different MPT nodes, so neither schema ID nor node ID alone identifies an occurrence.
     */
    struct StructurizedTextRef {
        int32_t schema_id;
        mpt_node_id_t structurized_text_id;
        size_t occurrence_idx;
    };

    // Methods
    /**
     * Collects same-node occurrences having log shape ID `log_shape_id`.
     * @param log_shape_id
     * @param structurized_text_id
     * @return The matching occurrence indices keyed by schema ID.
     */
    [[nodiscard]] auto
    get_occurrences(clpp::log_shape_id_t log_shape_id, mpt_node_id_t structurized_text_id) const
            -> InterpretationMatch::Occurrences;

    /**
     * Builds the log-surgeon parser from the archive's parsing spec if it hasn't been built yet.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `ArchiveReader::read_parsing_spec`'s return values.
     */
    [[nodiscard]] auto ensure_parser() -> ystdlib::error_handling::Result<void>;

    /**
     * Decomposes `query` against the log shapes returning interpretations that matched a log shape.
     * @param structurized_text_id The `StructurizedText` node to search within.
     * @return The interpretation matches, or an error code indicating the failure:
     * - Forwards `ArchiveReader::read_parsing_spec`'s return values.
     * - Forwards `clpp::decompose_by_log_shapes`'s return values.
     */
    [[nodiscard]] auto
    decompose_by_log_shapes(mpt_node_id_t structurized_text_id, std::string_view query)
            -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>>;

    /**
     * Decomposes `query` against the parent rule `rule_name`, then matches each interpretation's
     * shape query against each occurrence of `rule_name` in relevant log shapes.
     * @param structurized_text_id The `StructurizedText` node to search within.
     * @return The interpretation matches, or an error code indicating the failure:
     * - Forwards `ArchiveReader::read_parsing_spec`'s return values.
     * - Forwards `clpp::decompose_by_rule_name`'s return values.
     */
    [[nodiscard]] auto decompose_by_rule_name(
            mpt_node_id_t structurized_text_id,
            std::string_view query,
            std::string_view rule_name
    ) -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>>;

    /**
     * Invokes `on_match` for every occurrence of `rule_name` (or every whole log shape if
     * `rule_name` is empty) whose shape satisfies `shape_query`, walking occurrences in document
     * order. Only shapes of the `StructurizedText` node `structurized_text_id` are considered.
     * @tparam OnMatch The callback type of `on_match`, constrained by `OccurrenceMatchHandlerReq`.
     * @param structurized_text_id The `StructurizedText` node to search within.
     * @param rule_name A parent rule name, or empty to match the entire log shape.
     * @param shape_query A wildcard pattern, or std::nullopt to match every shape.
     * @param on_match Invoked for every matching occurrence.
     * @throw Propagates `ArchiveReader::get_parent_rule_shapes_array`'s exceptions.
     */
    template <OccurrenceMatchHandlerReq<InterpretationMatch::Occurrences> OnMatch>
    auto for_each_matching_occurrence(
            mpt_node_id_t structurized_text_id,
            std::string_view rule_name,
            std::optional<std::string_view> shape_query,
            OnMatch const& on_match
    ) const -> void;

    // Data members
    ArchiveReader* m_archive_reader;
    bool m_case_sensitive{false};
    std::vector<std::vector<StructurizedTextRef>> m_structurized_texts_per_log_shape;
    std::unique_ptr<log_surgeon::ParsingSpec> m_parsing_spec;
};

template <OccurrenceMatchHandlerReq<ClppMatcher::InterpretationMatch::Occurrences> OnMatch>
auto ClppMatcher::for_each_matching_occurrence(
        mpt_node_id_t structurized_text_id,
        std::string_view rule_name,
        std::optional<std::string_view> shape_query,
        OnMatch const& on_match
) const -> void {
    auto const matches_query{[&](std::string_view shape) -> bool {
        return false == shape_query.has_value()
               || clp::string_utils::wildcard_match_unsafe(shape, *shape_query, m_case_sensitive);
    }};
    auto const& log_shape_entries{m_archive_reader->get_log_shape_dictionary()->get_entries()};
    for (clpp::log_shape_id_t log_shape_id{0}; log_shape_id < log_shape_entries.size();
         ++log_shape_id)
    {
        auto const occurrences{get_occurrences(log_shape_id, structurized_text_id)};
        if (occurrences.empty()) {
            continue;
        }
        clpp::TextShape<std::string_view> const log_shape{
                log_shape_entries.at(log_shape_id).get_value()
        };
        if (rule_name.empty()) {
            if (matches_query(log_shape.view())) {
                on_match(occurrences, 0);
            }
            continue;
        }
        for (auto const& parent_match :
             m_archive_reader->get_parent_rule_shapes_array().get(log_shape_id))
        {
            if (rule_name != parent_match.m_name) {
                continue;
            }
            if (matches_query(log_shape.view().substr(parent_match.m_start, parent_match.m_size))) {
                on_match(occurrences, log_shape.count_placeholders_before(parent_match.m_start));
            }
        }
    }
}
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_CLPPMATCHER_HPP
