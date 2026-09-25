#ifndef CLP_S_SEARCH_CLPPMATCHER_HPP
#define CLP_S_SEARCH_CLPPMATCHER_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/string_utils/string_utils.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clpp/Defs.hpp>
#include <clpp/Interpretation.hpp>
#include <clpp/TextShape.hpp>

namespace clp_s::search {
/**
 * Utility for running clpp queries.
 */
class ClppMatcher {
public:
    // Types
    /**
     * A matched interpretation of a clpp query, made of the schema IDs containing a matching shape
     * (log shape or parent rule shape) and any leaf queries constraining it. If `leaf_queries` is
     * empty then matching shapes satisfy the query without constraining any leaf values.
     * Leaf positions are relative to the leaf placeholders of the whole log shape.
     */
    struct InterpretationMatch {
        std::unordered_set<int32_t> schema_ids;
        std::vector<clpp::LeafQuery> leaf_queries;
    };

    // Constructors
    /**
     * Builds the log shape to schemas index using `archive_reader` to enable clpp querying.
     * If the archive is not experimental construction is skipped and the object is invalid.
     * @param archive_reader
     * @param case_sensitive Whether matching is case sensitive.
     * @throw std::runtime_error if the log shape dictionary is not valid.
     */
    ClppMatcher(ArchiveReader* archive_reader, bool case_sensitive);

    // Methods
    /**
     * Finds the schemas whose log shapes (or parent rule shapes of `rule_name`) satisfy
     * `shape_query`.
     * @param rule_name A parent rule name, or empty to query the entire log shape.
     * @param shape_query A wildcard pattern, or std::nullopt to match every shape.
     * @return The matching schema IDs.
     */
    [[nodiscard]] auto find_matching_schemas(
            std::string_view rule_name,
            std::optional<std::string_view> shape_query
    ) const -> std::unordered_set<int32_t>;

    /**
     * Decomposes `query` against the parent rule `rule_name`, or against every log shape if
     * `rule_name` is empty, then returns the possible interpretations.
     * @param query
     * @param rule_name A qualified parent rule name, or empty to decompose against every log shape.
     * @return The interpretation matches (empty if no interpretation matched a shape), or an error
     * code indicating the failure:
     * - Forwards `decompose_by_log_shapes`'s return values.
     * - Forwards `decompose_by_rule_name`'s return values.
     */
    [[nodiscard]] auto decompose_query(std::string_view query, std::string_view rule_name)
            -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>>;

private:
    // Methods
    /**
     * Builds the log-surgeon parser from the archive's parsing spec if it hasn't been built yet.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `ArchiveReader::read_parsing_spec`'s return values.
     */
    [[nodiscard]] auto ensure_parser() -> ystdlib::error_handling::Result<void>;

    /**
     * Decomposes `query` against the log shapes returning interpretations that matched a log shape.
     * @return The interpretation matches, or an error code indicating the failure:
     * - Forwards `ArchiveReader::read_parsing_spec`'s return values.
     */
    [[nodiscard]] auto decompose_by_log_shapes(std::string_view query)
            -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>>;

    /**
     * Decomposes `query` against the parent rule `rule_name`, then matches each interpretation's
     * shape query against each occurrence of `rule_name` in relevant log shapes.
     * @return The interpretation matches, or an error code indicating the failure:
     * - Forwards `ArchiveReader::read_parsing_spec`'s return values.
     */
    [[nodiscard]] auto decompose_by_rule_name(std::string_view query, std::string_view rule_name)
            -> ystdlib::error_handling::Result<std::vector<InterpretationMatch>>;

    /**
     * Invokes `on_match` for every occurrence of `rule_name` (or every whole log shape if
     * `rule_name` is empty) whose shape satisfies `shape_query`, walking occurrences in document
     * order.
     * @param rule_name A parent rule name, or empty to match the entire log shape.
     * @param shape_query A wildcard pattern, or std::nullopt to match every shape.
     * @param on_match Invoked as `on_match(schema_ids, leaf_position)` with the schema IDs of the
     * log shape and the position of the occurrence's first leaf placeholder within the log shape (0
     * for a whole log shape).
     * @throw Propagates `ArchiveReader::get_parent_rule_shapes`'s exceptions.
     */
    template <typename OnMatch>
    auto for_each_matching_occurrence(
            std::string_view rule_name,
            std::optional<std::string_view> shape_query,
            OnMatch const& on_match
    ) const -> void;

    // Data members
    ArchiveReader* m_archive_reader;
    bool m_case_sensitive{false};
    std::vector<std::unordered_set<int32_t>> m_schemas_by_log_shape;
    std::unique_ptr<log_surgeon::Parser> m_parser;
};

template <typename OnMatch>
auto ClppMatcher::for_each_matching_occurrence(
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
        auto const& schema_ids{m_schemas_by_log_shape.at(log_shape_id)};
        if (schema_ids.empty()) {
            continue;
        }
        clpp::TextShape<std::string_view> const log_shape{
                log_shape_entries.at(log_shape_id).get_value()
        };
        if (rule_name.empty()) {
            if (matches_query(log_shape.view())) {
                on_match(schema_ids, 0);
            }
            continue;
        }
        for (auto const& parent_match :
             m_archive_reader->get_parent_rule_shapes().at(log_shape_id).get())
        {
            if (rule_name != parent_match.m_name) {
                continue;
            }
            if (matches_query(log_shape.view().substr(parent_match.m_start, parent_match.m_size))) {
                on_match(schema_ids, log_shape.count_placeholders_before(parent_match.m_start));
            }
        }
    }
}
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_CLPPMATCHER_HPP
