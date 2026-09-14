#ifndef CLP_S_SEARCH_SCHEMAMATCH_HPP
#define CLP_S_SEARCH_SCHEMAMATCH_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <clp_s/Schema.hpp>
#include <clp_s/search/ast/FilterOperation.hpp>
#include <clp_s/search/ClppMatcher.hpp>
#include <clpp/Interpretation.hpp>

#include "../ReaderUtils.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/Literal.hpp"
#include "ast/Transformation.hpp"
#include "clp_s/ArchiveReader.hpp"
#include "clp_s/SchemaTree.hpp"

namespace clp_s::search {
class SchemaMatch : public ast::Transformation {
public:
    // Constructor
    SchemaMatch(std::shared_ptr<ArchiveReader> archive_reader, bool case_sensitive);

    /**
     * Runs the transformation on an expression
     * @param expr
     * @return The transformed expression
     */
    std::shared_ptr<ast::Expression> run(std::shared_ptr<ast::Expression>& expr) override;

    /**
     * @param schema
     * @return The query for a given schema
     */
    std::shared_ptr<ast::Expression> get_query_for_schema(int32_t schema);

    /**
     * Checks if a schema has been matched
     * @param schema_id
     * @return true if the schema has been matched, false otherwise
     */
    bool schema_matched(int32_t schema_id);

    /**
     * Checks if the column
     * @param schema
     * @param column_id
     * @return true if the column has been matched, false otherwise
     */
    bool schema_searches_against_column(int32_t schema, int32_t column_id);

    /**
     * Adds a searched column to the schema. only used for pure wildcard
     * @param schema
     * @param column
     */
    void add_searched_column_to_schema(int32_t schema, int32_t column);

    /**
     * @return The total number of clpp interpretations created during schema matching. Every
     * interpretation returned by a decomposition is counted once.
     */
    [[nodiscard]] auto get_num_clpp_interpretations() const -> uint64_t {
        return m_num_clpp_interpretations;
    }

private:
    // Methods
    /**
     * Builds an expression from a single interpretation of a decomposed clpp query, and registers
     * leaf columns in m_descriptor_to_schema for the given schemas.
     *
     * When m_leaf_queries is non-empty, returns an AndExpr of leaf equality/existence filters.
     * When m_leaf_queries is empty (the match is purely on shape text), registers the column and
     * returns an EXISTS filter.
     *
     * @param column The original column descriptor triggering clpp decomposition.
     * @param root_node_id The schema-tree node where decomposition is rooted.
     * @param interpretation The single interpretation to build the expression from.
     * @param matched_schema_ids Schemas to register the leaf columns against.
     * @return The expression, or std::nullopt if a leaf column cannot be resolved in the schema
     * tree.
     */
    auto build_leaf_query_expr(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            SchemaNode::id_t root_node_id,
            clpp::Interpretation const& interpretation,
            std::unordered_set<int32_t> const& matched_schema_ids
    ) -> std::optional<std::shared_ptr<ast::Expression>>;

    /**
     * Resolves a CLPP leaf's rule-name path against the schema tree and produces column
     * descriptors for each matching schema-tree node at the leaf position.
     * Computes the common prefix between the column's existing descriptor tokens and the
     * rule names, then resolves the remaining (non-overlapping) segments through the schema
     * tree. The returned ColumnDescriptor is a clp-s field path: the unresolved rule-name
     * segments are appended to the input column's descriptor tokens, so the result is a
     * column descriptor rather than a rule name.
     *
     * A rule name may resolve to several nodes because schema nodes are keyed by both name and
     * type, so one pair is returned per matching node.
     *
     * @param column The original column descriptor triggering clpp decomposition.
     * @param root_node_id The schema node where decomposition is rooted.
     * @param rule_names The split rule names from the log-surgeon qualified name.
     * @return A vector of (column, node_id) pairs, or std::nullopt if any rule name cannot be
     * resolved in the schema tree.
     */
    auto resolve_leaf_rule_descriptors(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            SchemaNode::id_t root_node_id,
            std::vector<std::string_view> const& rule_names
    )
            -> std::optional<
                    std::vector<std::pair<std::shared_ptr<ast::ColumnDescriptor>, SchemaNode::id_t>>
            >;

    /**
     * Finds all child schema nodes whose key name matches the given name.
     * @param parent_id The parent schema node ID.
     * @param key_name The key name to match.
     * @return A vector of matching child node IDs (may be empty).
     */
    auto find_child_nodes_by_key_name(SchemaNode::id_t parent_id, std::string_view key_name)
            -> std::vector<SchemaNode::id_t>;

    /**
     * Handles an EXISTS or shape() filter at a LogMessage or ParentRule node: finds the schemas
     * whose shapes (log shape or `rule_name` parent rule shapes) satisfying `shape_query`, and
     * returns an EXISTS filter over the resolved column descriptor.
     * @param shape_query A wildcard pattern, or std::nullopt to match every shape.
     * @param op The operation to apply to the resolved column.
     * @return The filter expression, or nullptr if no schemas match.
     */
    auto build_shape_match_filter(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            SchemaNode::id_t root_node_id,
            std::string_view rule_name,
            std::optional<std::string_view> shape_query,
            ast::FilterOperation op,
            bool is_inverted
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Handles a value filter at a LogMessage or ParentRule node by decomposing `query` and OR-ing
     * together the leaf query expressions of every interpretation found to match a schema.
     *
     * Each interpretation's leaf columns are registered (via register_clpp_resolved_column) only
     * for the schemas whose log shape matched that interpretation. intersect_schemas records those
     * per-sub-expression schema sets, and split_expression_by_schema uses them so that each
     * schema's query (m_schema_to_query) keeps only the interpretations relevant to it.
     * @param rule_name The parent rule name, or empty for a LogMessage node.
     * @return The filter expression, or nullptr if no interpretation matched.
     * @throw std::runtime_error if `ClppMatcher::decompose_query` returns an error.
     * @throw Propagates `ClppMatcher::decompose_query`'s exceptions.
     */
    auto build_decomposed_query_filter(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            SchemaNode::id_t root_node_id,
            std::string_view rule_name,
            std::string_view query
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Resolves a clpp filter at a LogMessage or ParentRule node into an expression.
     *
     * `root_node_id` decides what the query is matched against: at a LogMessage node it is matched
     * against whole log shapes, and at a ParentRule node only against that rule's shapes.
     *
     * There are 3 possible filter cases:
     * 1. EXISTS/NEXISTS: matches every schema that contains the node.
     * 2. shape(): matches the schemas containing shapes satisfying the query.
     * 3. Any other operation: the query is decomposed into interpretations, each expanded
     *   into leaf filters.
     *
     * @param column The column triggering clpp decomposition.
     * @param root_node_id The schema-tree node where decomposition is rooted (LogMessage or
     * ParentRule).
     * @param filter The FilterExpr containing the operation and operand.
     * @return The transformed expression on success, or nullptr if no schema matches, the filter
     * has no operand, or the operand can not be converted to a string for the filter's operation
     * (e.g. a range operation).
     */
    auto build_clpp_query_filter(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            SchemaNode::id_t root_node_id,
            ast::FilterExpr const& filter
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Reads the dictionaries needed to read the columns in the given schema view, recursing into
     * unordered object sub-schemas.
     */
    auto read_dictionaries_for_schema(SchemaView const& schema) -> void;

    // Data members
    std::unordered_map<uint32_t, std::set<std::shared_ptr<ast::ColumnDescriptor>>>
            m_column_to_descriptor;
    // TODO: The value in the map can be a set of k:v pairs with a hash & comparison
    // that only considers the key since each column descriptor only has one matching
    // column id per schema
    std::unordered_map<ast::ColumnDescriptor::id_t, std::map<int32_t, int32_t>>
            m_descriptor_to_schema;
    std::map<ast::ColumnDescriptor::id_t, std::set<int32_t>> m_unresolved_descriptor_to_descriptor;
    std::unordered_map<ast::Expression*, std::unordered_set<int32_t>> m_expression_to_schemas;
    std::unordered_set<int32_t> m_matched_schema_ids;
    std::unordered_set<int32_t> m_array_search_schema_ids;
    std::map<int32_t, std::shared_ptr<ast::Expression>> m_schema_to_query;

    std::unordered_map<int32_t, std::set<int32_t>> m_schema_to_searched_columns;
    std::shared_ptr<ArchiveReader> m_archive_reader;
    std::shared_ptr<SchemaTree> m_tree;
    std::shared_ptr<ReaderUtils::SchemaMap> m_schemas;
    bool m_clpp_decomposed_query{false};
    bool m_clpp_node_matched{false};
    uint64_t m_num_clpp_interpretations{0};
    ClppMatcher m_clpp_matcher;

    /**
     * Populates the column mapping for a given column
     * @param column
     * @param node_id
     * @return true if matching is successful, false otherwise
     */
    auto populate_column_mapping(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            int32_t node_id,
            std::shared_ptr<ast::Expression> const& expr
    ) -> std::tuple<bool, std::shared_ptr<ast::Expression>>;

    /**
     * Populates the column mapping for a given column
     * @param column
     * @return
     */
    auto populate_column_mapping(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            std::shared_ptr<ast::Expression> const& expr
    ) -> std::tuple<bool, std::shared_ptr<ast::Expression>>;

    /**
     * Populates the column mapping for a given expression
     * @param cur
     * @return The transformed expression
     */
    std::shared_ptr<ast::Expression> populate_column_mapping(
            std::shared_ptr<ast::Expression> const& cur
    );

    /**
     * Populates the schema mapping
     */
    void populate_schema_mapping();

    /**
     * Finds common schemas and relevant columns across filters and stores the mapping
     * @param cur
     * @return The transformed expression
     */
    std::shared_ptr<ast::Expression> intersect_schemas(std::shared_ptr<ast::Expression> cur);

    /**
     * Finds common schemas and relevant columns across filters. The `first` parameter is true
     * on the initial call (initializing `common_schema` with the matched schemas for the first
     * column) and false on recursive calls (intersecting subsequent columns into `common_schema`).
     * @param cur The current expression node to process.
     * @param common_schema The set of schemas common to all processed columns so far, updated
     *     in place.
     * @param columns The set of column descriptors found; populated on each call.
     * @param first True on the initial call, false on recursive calls.
     * @return True when `first` was true and no column was a pure wildcard; false otherwise.
     */
    bool intersect_and_sub_expr(
            std::shared_ptr<ast::Expression> const& cur,
            std::set<int32_t>& common_schema,
            std::set<ast::ColumnDescriptor*>& columns,
            bool first
    );

    /**
     * Computes the set of literal types a column may resolve to, as a bitmask.
     *
     * A column can map to a different node (and therefore a different literal type) in each schema,
     * so the mask is the union of its node types across several schemas. split_expression_by_schema
     * later replaces this mask with the single type the column has in one specific schema.
     *
     * For a column resolved by ClppMatcher, `schemas` is ignored: ClppMatcher already determined
     * exactly which schemas the query matched, so the union is taken over those schemas instead.
     * For any other column, the union is taken over the schemas in `schemas` that the column also
     * maps to.
     *
     * @param column The column whose mask is being computed.
     * @param schemas The schemas to take the union over.
     * @return The bitmask of literal types.
     */
    [[nodiscard]] auto compute_candidate_types(
            ast::ColumnDescriptor const& column,
            std::set<int32_t> const& schemas
    ) const -> ast::literal_type_bitmask_t;

    /**
     * Splits an expression into sub-expressions based on the schemas it searches against
     * @param expr
     * @param queries a map from schema id to expression
     * @param relevant_schemas
     */
    void split_expression_by_schema(
            std::shared_ptr<ast::Expression> const& expr,
            std::map<int32_t, std::shared_ptr<ast::Expression>>& queries,
            std::unordered_set<int32_t> const& relevant_schemas
    );

    /**
     * @param col_id The id of the column descriptor.
     * @param schema
     * @return The column id for a given column descriptor id.
     */
    int32_t get_column_id_for_descriptor(ast::ColumnDescriptor::id_t col_id, int32_t schema);

    /**
     * Marks a column as clpp-resolved and records (`in m_descriptor_to_schema`) that the column
     * resolves to `node_id` in each of `matched_schema_ids`.
     *
     * ClppMatcher matched only the schemas whose shape satisfied the query, and that subset must
     * survive to the end of SchemaMatch::run. Two things preserve it:
     * - run() clears m_column_to_descriptor before re-running populate_column_mapping, but leaves
     *   m_descriptor_to_schema intact, so these mappings survive.
     * - The is_clpp_resolved flag keeps the column out of m_column_to_descriptor, so
     *   populate_schema_mapping skips it. That function maps a column to every schema containing
     *   its node, which for a clpp column would add back the schemas whose shape did not match.
     *
     * @param column The column descriptor to register.
     * @param node_id The schema-tree node ID to map to.
     * @param matched_schema_ids The schemas to register the column for.
     */
    auto register_clpp_resolved_column(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            SchemaNode::id_t node_id,
            std::unordered_set<int32_t> const& matched_schema_ids
    ) -> void;

    /**
     * @param col_id The id of the column descriptor.
     * @param schema
     * @return The literal type for a given column descriptor id.
     */
    ast::LiteralType
    get_literal_type_for_column(ast::ColumnDescriptor::id_t col_id, int32_t schema);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_SCHEMAMATCH_HPP
