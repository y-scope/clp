#ifndef CLP_S_SEARCH_SCHEMAMATCH_HPP
#define CLP_S_SEARCH_SCHEMAMATCH_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <clp_s/ArchiveReader.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/Schema.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/NarrowSchemasExpr.hpp>
#include <clp_s/search/ClppMatcher.hpp>
#include <clpp/Interpretation.hpp>

#include "../ReaderUtils.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/Literal.hpp"
#include "ast/Transformation.hpp"

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
     * Hands the per-schema expressions built by `run` over to the caller, leaving none behind.
     * @return The expression to evaluate against each matched schema, keyed by schema ID.
     */
    [[nodiscard]] auto release_schema_expressions()
            -> std::map<int32_t, std::shared_ptr<ast::Expression>>;

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
     * @return The total number of interpretations produced by query decomposition during schema
     * matching.
     */
    [[nodiscard]] auto get_num_clpp_interpretations() const -> uint64_t {
        return m_num_clpp_interpretations;
    }

private:
    // Static methods
    /**
     * Builds a copy of a pure wildcard `filter` narrowed to every non-decomposable column type,
     * i.e., with `ClppDecomposeT` removed from the column's matching types. Decomposable nodes are
     * covered separately by per-node filters.
     * @param column The pure wildcard column.
     * @param filter The pure wildcard filter to copy.
     * @return The filter, or nullptr if `column` is a shape() column (shapes exist only on
     * decomposable nodes) or matches no other type.
     */
    [[nodiscard]] static auto build_non_decomposable_wildcard_filter(
            ast::ColumnDescriptor const& column,
            ast::FilterExpr const& filter
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Builds a `NarrowSchemasExpr` that narrows its enclosing expression to `schema_ids` (or, when
     * inverted, to every other schema) without constraining any column value.
     * @param schema_ids The schemas to narrow to.
     * @param is_inverted Whether the expression holds for the schemas outside `schema_ids` instead.
     * @return The expression, or nullptr if `schema_ids` is empty and `is_inverted` is false (no
     * schema can match). An inverted expression over an empty set holds for every schema.
     */
    [[nodiscard]] static auto
    narrow_to_schemas(std::unordered_set<int32_t> const& schema_ids, bool is_inverted = false)
            -> std::shared_ptr<ast::Expression>;

    // Methods
    /**
     * Builds an expression from a single interpretation of a decomposed query, and registers the
     * leaf columns against the given schemas.
     *
     * When `leaf_queries` is non-empty, returns an AndExpr of leaf equality filters (one per leaf
     * query). When `leaf_queries` is empty (the shape text itself satisfies the query, so no leaf
     * value is constrained), returns a `NarrowSchemasExpr` for `matched_schema_ids`.
     *
     * @param column The original column descriptor triggering query decomposition.
     * @param decomposition_root The decomposable node where the query's decomposition is rooted.
     * @param leaf_queries The interpretation's leaf queries, constraining value matches.
     * @param matched_schema_ids Schemas to register the leaf columns against.
     * @return The expression.
     * @return std::nullopt if a leaf column cannot be resolved in the MPT.
     */
    auto build_leaf_query_expr(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t decomposition_root,
            std::span<clpp::LeafQuery const> leaf_queries,
            std::unordered_set<int32_t> const& matched_schema_ids
    ) -> std::optional<std::shared_ptr<ast::Expression>>;

    /**
     * Builds the negation of a single interpretation of a decomposed query, and registers the leaf
     * columns against the given schemas.
     *
     * An interpretation holds when every leaf query matches, so its negation holds when any leaf
     * query fails to match. The expression is returned in De Morgan'd form (an OrExpr over leaves,
     * each an AndExpr of inverted equality filters, one per MPT node the leaf resolves to), so
     * inversion is carried only by leaf filters.
     *
     * @param column The original column descriptor triggering query decomposition.
     * @param decomposition_root The decomposable node where the query's decomposition is rooted.
     * @param leaf_queries The interpretation's leaf queries; must be non-empty.
     * @param matched_schema_ids Schemas to register the leaf columns against.
     * @return The expression.
     * @return std::nullopt if a leaf column cannot be resolved in the MPT.
     */
    auto build_negated_leaf_query_expr(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t decomposition_root,
            std::span<clpp::LeafQuery const> leaf_queries,
            std::unordered_set<int32_t> const& matched_schema_ids
    ) -> std::optional<std::shared_ptr<ast::Expression>>;

    /**
     * Resolves a single leaf query to its MPT nodes and builds one equality filter per node, each
     * on a column bound to the node, pinned to the leaf's position, and mapped to
     * `matched_schema_ids` only.
     * @param column The original column descriptor triggering query decomposition.
     * @param decomposition_root The decomposable node where the query's decomposition is rooted.
     * @param leaf The leaf query.
     * @param matched_schema_ids Schemas to map the leaf columns to.
     * @param is_inverted Whether each equality filter is inverted.
     * @return One filter per resolved node.
     * @return std::nullopt if the leaf cannot be resolved in the MPT.
     */
    auto build_leaf_filters(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t decomposition_root,
            clpp::LeafQuery const& leaf,
            std::unordered_set<int32_t> const& matched_schema_ids,
            bool is_inverted
    ) -> std::optional<std::vector<std::shared_ptr<ast::Expression>>>;

    /**
     * Resolves a leaf's rule-name path against the MPT and produces column descriptors for
     * each matching MPT node at the leaf position.
     *
     * Computes the common prefix between the column's existing descriptor tokens and the rule
     * names, then resolves the remaining (non-overlapping) segments through the MPT. The returned
     * ColumnDescriptor is a clp-s field path: the unresolved rule-name segments are appended to the
     * input column's descriptor tokens, so the result is a column descriptor rather than a rule
     * name. A rule name may resolve to several nodes because MPT nodes are keyed by both name and
     * type, so one pair is returned per matching node.
     *
     * @param column The original column descriptor triggering query decomposition.
     * @param decomposition_root The decomposable node where the query's decomposition is rooted.
     * @param rule_names The split rule names from the log-surgeon qualified name.
     * @return One (column, node ID) pair per matching node.
     * @return std::nullopt if any rule name cannot be resolved in the MPT.
     */
    auto resolve_leaf_rule_descriptors(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t decomposition_root,
            std::vector<std::string_view> const& rule_names
    ) -> std::
            optional<std::vector<std::pair<std::shared_ptr<ast::ColumnDescriptor>, mpt_node_id_t>>>;

    /**
     * Finds all child MPT nodes whose key name matches the given name.
     * @param parent_id The parent MPT node ID.
     * @param key_name The key name to match.
     * @return A vector of matching child node IDs (may be empty).
     */
    auto find_child_nodes_by_key_name(mpt_node_id_t parent_id, std::string_view key_name)
            -> std::vector<mpt_node_id_t>;

    /**
     * Handles a shape() filter at a decomposable node by resolving it to a `NarrowSchemasExpr`
     * over:
     * - not inverted: the schemas whose shapes satisfy `shape_query`.
     * - inverted: the schemas containing the node whose shapes do not satisfy `shape_query`.
     * @param decomposition_root The decomposable node where the query's decomposition is rooted.
     * @param shape_query A wildcard pattern matched against the node's shapes.
     * @param is_inverted Whether the original filter was negated.
     * @return The `NarrowSchemasExpr`, or nullptr if no schemas are selected.
     */
    auto build_shape_filter(
            mpt_node_id_t decomposition_root,
            std::string_view shape_query,
            bool is_inverted
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Handles a non-inverted value filter at a decomposable node by decomposing `query`
     * and OR-ing together the leaf query expressions of every interpretation found to match a
     * schema.
     *
     * Each interpretation's leaf columns are registered only against the schemas containing a shape
     * matching that interpretation, so that later passes give each schema only the interpretations
     * relevant to it.
     * @param column The column triggering query decomposition.
     * @param decomposition_root The decomposable node where the query's decomposition is rooted.
     * @param rule_name The parent rule name, or empty for a StructurizedText node.
     * @param query The value to match against the node's decomposed shapes.
     * @param is_inverted Whether the original filter was negated.
     * @return The filter expression, or nullptr if no interpretation matched.
     * @throws std::runtime_error if `ClppMatcher::decompose_query` returns an error.
     */
    auto build_decomposed_query_filter(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t decomposition_root,
            std::string_view rule_name,
            std::string_view query,
            bool is_inverted
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Builds the expression for a negated value filter at a decomposable node.
     *
     * Schemas are grouped by their (matching) interpretations, and each group's operand registers
     * its columns against only the schemas in that group. Each group becomes:
     * - nothing, if an interpretation with no leaf constraints applies (the query always holds);
     * - a `NarrowSchemasExpr`, if no interpretation applies (the query never holds);
     * - an AndExpr of the applicable interpretations' negated leaf query expressions otherwise.
     * Interpretations whose leaves cannot be resolved can never hold and are omitted.
     * @param column The column triggering query decomposition.
     * @param decomposition_root The decomposable node where the query's decomposition is rooted.
     * @param interpretations The interpretation matches applicable to a schema.
     * @return The filter expression, or nullptr if no schema can match.
     */
    auto build_negated_decomposed_query_filter(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t decomposition_root,
            std::vector<ClppMatcher::InterpretationMatch> const& interpretations
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * @param narrowing
     * @return The schemas `narrowing` holds for: its schema IDs, or their complement over the
     * archive's schemas if it is inverted.
     */
    [[nodiscard]] auto get_narrowed_schemas(ast::NarrowSchemasExpr const& narrowing) const
            -> std::set<int32_t>;

    /**
     * @param node_id A decomposable node.
     * @return Every schema containing `node_id`.
     */
    [[nodiscard]] auto find_schemas_containing_node(mpt_node_id_t node_id) const
            -> std::unordered_set<int32_t>;

    /**
     * Resolves a filter at a decomposable node into an expression.
     *
     * `decomposition_root` decides what the query is matched against: at a StructurizedText node it
     * is matched against whole log shapes, and at a ParentRule node only against that rule's
     * shapes.
     *
     * There are 3 possible filter cases:
     * 1. EXISTS/NEXISTS: narrows to the schemas that contain the node, or to their complement when
     *    the filter holds for events lacking the node (NEXISTS, or inverted EXISTS).
     * 2. shape(): narrows to the schemas containing shapes satisfying the query.
     * 3. Any other operation: the query is decomposed into interpretations, each expanded into
     *    leaf filters.
     *
     * @param column The column triggering query decomposition.
     * @param decomposition_root The decomposable node where the query's decomposition is rooted.
     * @param filter The FilterExpr containing the operation and operand.
     * @return The transformed expression on success, or nullptr if no schema matches, the filter
     * has no operand, or the operand can not be converted to a string for the filter's operation
     * (e.g. a range operation).
     */
    auto build_decomposable_node_filter(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t decomposition_root,
            ast::FilterExpr const& filter
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Builds a fully resolved column descriptor whose token path targets the decomposable MPT node
     * `node_id`, typed as `node_id`'s literal type and keeping `column`'s namespace, subtree type
     * and function.
     *
     * Decomposable nodes only ever exist within the object subtree (see
     * `JsonParser::parse_unstructured_text`), so the token path is built by walking from `node_id`
     * up to the root of the object subtree for `column`'s namespace. Non-decomposable nodes are
     * bound directly by node ID instead (see `build_resolved_node_filter`) and must not be passed
     * here.
     * @param column The unbound column that `node_id` was matched from.
     * @param node_id A decomposable node.
     * @return The resolved column descriptor.
     */
    [[nodiscard]] auto build_resolved_decomposable_column(
            ast::ColumnDescriptor const& column,
            mpt_node_id_t node_id
    ) const -> std::shared_ptr<ast::ColumnDescriptor>;

    /**
     * Builds `filter` against the single MPT node `node_id`: a decomposable node gets a
     * decomposable-node filter; any other node gets a copy of `column` bound to `node_id`.
     * @param column The unbound column that matched `node_id`.
     * @param node_id An MPT node matched from `column`.
     * @param filter The filter targeting `column`.
     * @return The expression, or nullptr if no schema can match.
     */
    [[nodiscard]] auto build_resolved_node_filter(
            ast::ColumnDescriptor const& column,
            mpt_node_id_t node_id,
            ast::FilterExpr const& filter
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Builds `filter` against every MPT node in `node_ids` (see `build_resolved_node_filter`) and
     * combines the per-node filters. A value filter on `column` holds when the value at any of
     * the nodes matches, so the per-node filters are OR-ed together. When `filter` is an
     * existence filter that holds for events lacking the column (NEXISTS, or inverted EXISTS), it
     * holds only when the column is absent at every node, so the per-node filters are AND-ed
     * together. (An inverted value filter does not hold for events lacking the column, so it is
     * OR-ed like any other value filter.)
     * @param column The column that matched `node_ids`.
     * @param node_ids The MPT nodes matched from `column`.
     * @param filter The filter targeting `column`.
     * @return The combined expression, or nullptr if no schema can match.
     */
    [[nodiscard]] auto build_multi_node_filter(
            ast::ColumnDescriptor const& column,
            std::vector<mpt_node_id_t> const& node_ids,
            ast::FilterExpr const& filter
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Builds the pure wildcard expansion: an OR of the non-decomposable wildcard filter and one
     * resolved filter per decomposable node.
     * @param column The pure wildcard column.
     * @param filter The non-inverted filter.
     * @param decomposable_node_ids Every decomposable node in `column`'s namespace. Must be
     * non-empty.
     * @return The expanded expression, or nullptr if no schema can match.
     */
    [[nodiscard]] auto build_pure_wildcard_expansion(
            ast::ColumnDescriptor const& column,
            ast::FilterExpr const& filter,
            std::vector<mpt_node_id_t> const& decomposable_node_ids
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Builds the negated pure wildcard expansion, which holds when neither any decomposable node
     * nor any other column matches `filter`. Schemas are grouped by the exact set of decomposable
     * nodes they contain so that each group's conjunction stays small; AND-ing every node for every
     * schema would blow up under OrOfAndForm.
     * @param column The pure wildcard column.
     * @param filter The inverted filter.
     * @param decomposable_node_ids Every decomposable node in `column`'s namespace. Must be
     * non-empty.
     * @return The expanded expression, or nullptr if no schema can match.
     */
    [[nodiscard]] auto build_negated_pure_wildcard_expansion(
            ast::ColumnDescriptor const& column,
            ast::FilterExpr const& filter,
            std::vector<mpt_node_id_t> const& decomposable_node_ids
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Builds the negated expression for one group of schemas that contain exactly the decomposable
     * nodes `node_ids`: an AND of a `NarrowSchemasExpr` for `schema_ids`, the inverted
     * non-decomposable wildcard filter, and the negated filter of every node in `node_ids`.
     * @param column The pure wildcard column.
     * @param filter The inverted filter.
     * @param node_ids The nodes contained by every schema in `schema_ids` (may be empty).
     * @param schema_ids The schemas that contain exactly `node_ids`.
     * @return The expression, or nullptr if some node's negation cannot hold in any schema.
     */
    [[nodiscard]] auto build_negated_decomposable_node_group(
            ast::ColumnDescriptor const& column,
            ast::FilterExpr const& filter,
            std::vector<mpt_node_id_t> const& node_ids,
            std::unordered_set<int32_t> const& schema_ids
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Resolves the nearest enclosing `StructurizedText` node of `node_id`. If `node_id` is a
     * `StructurizedText`, it resolves to itself. Because log events contain only sibling (never
     * nested) `StructurizedText`s, every decomposable node has exactly one such enclosing node,
     * whose ID identifies which unstructured field a query targets.
     * @param node_id The MPT node whose enclosing `StructurizedText` to find.
     * @return The enclosing `StructurizedText` node's ID (`node_id` itself when it is a
     * `StructurizedText`), or `constants::cRootNodeId` if `node_id` has no enclosing
     * `StructurizedText`.
     */
    [[nodiscard]] auto find_enclosing_structurized_text_node_id(mpt_node_id_t node_id) const
            -> mpt_node_id_t;

    /**
     * Reads the dictionaries needed to read the columns in the given schema view, recursing into
     * unordered object sub-schemas.
     */
    auto read_dictionaries_for_schema(SchemaView const& schema) -> void;

    /**
     * Computes the set of literal types a column may resolve to, as a bitmask.
     *
     * A column can map to a different node (and therefore a different literal type) in each schema,
     * so the mask is the union of its node types across several schemas. split_expression_by_schema
     * later replaces this mask with the single type the column has in one specific schema.
     *
     * For a decomposed leaf rule column, `schemas` is ignored: it is bound to a single node, so its
     * type is that node's type regardless of schema.
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
     * Records `column` as mapped to MPT node `node_id` in `m_column_to_descriptor`.
     * @param column
     * @param node_id
     */
    auto record_column_at_node(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t node_id
    ) -> void;

    /**
     * Handles the special case of a pure wildcard column (`*`). A pure wildcard has no MPT node of
     * its own, so the object subtree of `column`'s namespace is walked to collect every
     * decomposable node:
     * - If the filter is expandable over decomposable nodes and such nodes exist, it is rewritten
     *   into an expression with one filter per node plus a non-decomposable wildcard filter over
     *   every other column type (or `EmptyExpr` if no schema can match).
     * - If the column is a shape() column and no such nodes exist, the column matches no node.
     * - Otherwise, it falls back to matching by node type.
     * @param column The pure wildcard column.
     * @param expr The FilterExpr containing `column`.
     * @return The rewritten expression (`expr` itself if unchanged), or nullptr if the column
     * matched no node.
     */
    [[nodiscard]] auto resolve_pure_wildcard(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            std::shared_ptr<ast::Expression> const& expr
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Resolves a pure wildcard by checking that at least one MPT node has a type the column can
     * match. If the filter is inverted and no node type matches, nothing can satisfy it, so its
     * negation holds for every record and the filter is rewritten to always match.
     * @param column The pure wildcard column.
     * @param expr The FilterExpr containing `column`.
     * @return `expr` if some node type matched, an inverted (always-true) `EmptyExpr` if none did
     * and the filter is inverted, or nullptr if none did and the filter is not inverted.
     */
    [[nodiscard]] auto resolve_pure_wildcard_by_node_type(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            std::shared_ptr<ast::Expression> const& expr
    ) const -> std::shared_ptr<ast::Expression>;

    /**
     * Expands a filter on a column with unresolved wildcard tokens over the MPT nodes the column
     * matched on the first resolution pass (see `build_multi_node_filter`).
     * @param column The column with unresolved wildcard tokens.
     * @param filter The filter containing `column`.
     * @return The expansion, or `EmptyExpr` if no schema can match.
     */
    auto
    expand_unresolved_column(ast::ColumnDescriptor const& column, ast::FilterExpr const& filter)
            -> std::shared_ptr<ast::Expression>;

    // Data members
    std::unordered_map<uint32_t, std::set<std::shared_ptr<ast::ColumnDescriptor>>>
            m_column_to_descriptor;
    std::unordered_map<ast::Expression*, std::unordered_set<int32_t>> m_expression_to_schemas;
    std::unordered_set<int32_t> m_matched_schema_ids;
    std::map<int32_t, std::shared_ptr<ast::Expression>> m_schema_to_expression;

    std::unordered_map<int32_t, std::set<int32_t>> m_schema_to_searched_columns;
    std::shared_ptr<ArchiveReader> m_archive_reader;
    std::shared_ptr<SchemaTree> m_tree;
    std::shared_ptr<ReaderUtils::SchemaMap> m_schemas;
    // Whether column resolution rewrote the expression (wildcard expansion, query decomposition or
    // multi-node expansion), requiring normalization and a second resolution pass.
    bool m_expr_rewritten{false};
    uint64_t m_num_clpp_interpretations{0};
    ClppMatcher m_clpp_matcher;

    /**
     * Matches a column's token path against a subtree of the MPT, starting from `subtree_root_id`.
     * For a node to match, its type must be compatible with the column's filter.
     *
     * Wildcard tokens may match any number of intermediate nodes (including zero). Matches are
     * recorded according to the column:
     * - fully resolved column: each matched leaf node is appended to `leaf_node_ids` for the
     *   caller to resolve;
     * - column with wildcard tokens: matches are recorded in the column's resolved nodes, from
     *   which the caller expands the column over the matched nodes;
     * - fully resolved column reaching an unstructured array: the remaining tokens are stored on
     *   the column and the array node is appended to `leaf_node_ids`.
     *
     * A bound column skips the walk. It is recorded in `m_column_to_descriptor` so that
     * `populate_schema_mapping` maps it to every schema containing its node, except for a
     * decomposed leaf column, whose schemas are already fixed.
     * @param column The column descriptor to resolve.
     * @param subtree_root_id The ID of the MPT node to start resolution from.
     * @param leaf_node_ids Receives the leaf nodes matched by a fully resolved column.
     * @return Whether the column matched at least one node.
     */
    auto populate_column_mapping(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            mpt_node_id_t subtree_root_id,
            std::vector<mpt_node_id_t>& leaf_node_ids
    ) -> bool;

    /**
     * Resolves a single column against the MPT, choosing which subtree root(s) to start
     * from. If the column has an explicit subtree type (e.g. `metadata`), only that subtree is
     * searched; otherwise every subtree matching the column's namespace is searched (except the
     * Metadata subtree).
     *
     * A fully resolved column may match several same-named leaf nodes of different types. If none
     * is decomposable, every match is recorded in `m_column_to_descriptor`. Otherwise `expr` is
     * rewritten with `build_multi_node_filter` over all matches, so a decomposable node's filter
     * and its non-decomposable siblings are resolved together.
     * @param column The column descriptor to resolve.
     * @param expr The FilterExpr containing `column`.
     * @return The rewritten expression (`expr` itself if unchanged, or `EmptyExpr` if every matched
     * node was found unsatisfiable), or nullptr if the column matched no node.
     */
    auto populate_column_mapping(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            std::shared_ptr<ast::Expression> const& expr
    ) -> std::shared_ptr<ast::Expression>;

    /**
     * Resolves every column reference in a query expression tree against the archive's MPT.
     * Filters whose column matches no node are replaced with `EmptyExpr` (no results). This is
     * the first transformation applied to a query, and runs a second time after Or-of-And
     * normalization if the first pass left unresolved wildcards or arrays, or rewrote the
     * expression (wildcard expansion, query decomposition or multi-node expansion).
     * @param cur The expression to process.
     * @return The transformed expression.
     * @throws std::runtime_error if a shape() column has no decomposable node to target.
     */
    auto populate_column_mapping(std::shared_ptr<ast::Expression> const& cur)
            -> std::shared_ptr<ast::Expression>;

    /**
     * Fills each column's schema-to-node mapping from `m_column_to_descriptor` by scanning every
     * schema's nodes. Pure wildcards (resolved per schema at evaluation) are skipped.
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
     * Determines whether a NEXISTS filter on `column` can hold for records of `schema_id`: either
     * the schema has no node for the column, or the column resolved to an unstructured array in
     * the schema with tokens left unresolved, so the array must be scanned for the remaining keys.
     * @param column
     * @param schema_id
     * @return Whether `schema_id` must be searched for the NEXISTS filter.
     */
    [[nodiscard]] auto
    is_nexists_candidate_schema(ast::ColumnDescriptor const& column, int32_t schema_id) const
            -> bool;

    /**
     * Splits an expression into one expression per schema it searches against. Each filter is
     * split by `split_filter_for_schema`; a `NarrowSchemasExpr` becomes the constant true in every
     * schema it holds for.
     * @param expr
     * @param schema_expressions Receives the expression for each schema ID.
     * @param relevant_schemas The schemas `expr` searches against.
     */
    void split_expression_by_schema(
            std::shared_ptr<ast::Expression> const& expr,
            std::map<int32_t, std::shared_ptr<ast::Expression>>& schema_expressions,
            std::unordered_set<int32_t> const& relevant_schemas
    );

    /**
     * Splits a filter for one schema: the copy's column is bound to the schema's node for that
     * column and matches only the node's type. The column is left unbound when the schema has no
     * such node, i.e. for a pure wildcard column or a NEXISTS filter on an absent column.
     * @param filter
     * @param schema_id
     * @return The copy of `filter` to evaluate against `schema_id`.
     */
    [[nodiscard]] auto split_filter_for_schema(ast::FilterExpr const& filter, int32_t schema_id)
            -> std::shared_ptr<ast::Expression>;

    /**
     * Appends `operand` to the `AndExpr` for `schema_id` in `schema_expressions`, creating the
     * `AndExpr` if absent.
     * @param schema_expressions The per-schema expressions being built.
     * @param schema_id
     * @param operand
     * @param inverted Whether a newly created `AndExpr` is inverted.
     */
    static auto append_to_and(
            std::map<int32_t, std::shared_ptr<ast::Expression>>& schema_expressions,
            int32_t schema_id,
            std::shared_ptr<ast::Expression> const& operand,
            bool inverted
    ) -> void;

    /**
     * Appends `operand` to the `OrExpr` for `schema_id` in `schema_expressions`. An absent
     * expression becomes `operand` itself; an expression that is not an `OrExpr` is first wrapped
     * in one.
     * @param schema_expressions The per-schema expressions being built.
     * @param schema_id
     * @param operand
     */
    static auto append_to_or(
            std::map<int32_t, std::shared_ptr<ast::Expression>>& schema_expressions,
            int32_t schema_id,
            std::shared_ptr<ast::Expression> const& operand
    ) -> void;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_SCHEMAMATCH_HPP
