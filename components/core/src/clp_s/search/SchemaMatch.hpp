#ifndef CLP_S_SEARCH_SCHEMAMATCH_HPP
#define CLP_S_SEARCH_SCHEMAMATCH_HPP

#include <concepts>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/DictionaryReader.hpp>
#include <clpp/DecomposedQuery.hpp>

#include "../ReaderUtils.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/Literal.hpp"
#include "ast/Transformation.hpp"
#include "clp_s/ArchiveReader.hpp"
#include "clp_s/SchemaTree.hpp"

namespace clp_s::search {
/**
 * A callable that takes a std::string_view and returns something convertible to bool.
 * Used to constrain the matcher argument of `find_schemas_matching_predicate`.
 */
template <typename Matcher>
concept StringViewPredicate = std::predicate<Matcher, std::string_view>;

class SchemaMatch : public ast::Transformation {
public:
    // Constructor
    SchemaMatch(std::shared_ptr<ArchiveReader> archive_reader);

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
     * Checks if the schema has an array field
     * @param schema_id
     * @return true if the schema has, false otherwise
     */
    bool has_array(int32_t schema_id);

    /**
     * Checks if the schema has an array field to be searched against
     * @param schema_id
     * @return true if the schema has, false otherwise
     */
    bool has_array_search(int32_t schema_id);

private:
    // Methods
    /**
     * Builds an expression from a single interpretation of a decomposed clpp query, and registers
     * leaf columns in m_descriptor_to_schema for the given schemas.
     *
     * When m_leaf_queries is non-empty, returns an AndExpr of leaf equality/existence filters.
     * When m_leaf_queries is empty (the match is purely on static shape text), registers the column
     * and returns an EXISTS filter — every document in the matched schemas contains the queried
     * text.
     *
     * Returns std::nullopt only if a leaf column cannot be resolved in the schema tree.
     * @param column The original column descriptor triggering clpp decomposition.
     * @param root_node_id The schema-tree node where decomposition is rooted.
     * @param interpretation The single interpretation to build the expression from.
     * @param matched_schema_ids Schemas to register the leaf columns against.
     * @return The expression, or std::nullopt on resolution failure.
     */
    auto build_leaf_query_expr(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            SchemaNode::id_t root_node_id,
            clpp::DecomposedQuery::Interpretation const& interpretation,
            std::unordered_set<int32_t> const& matched_schema_ids
    ) -> std::optional<std::shared_ptr<ast::Expression>>;

    /**
     * Resolves a CLPP leaf's rule-name path against the schema tree and produces column
     * descriptors for each matching schema-tree node at the leaf position.
     * Computes the common prefix between the column's existing descriptor tokens and the
     * rule names, then resolves the remaining (non-overlapping) segments through the schema
     * tree.
     *
     * @param column The original column descriptor triggering clpp decomposition.
     * @param root_node_id The schema node where decomposition is rooted.
     * @param rule_names The split rule names from the log-surgeon FQN.
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
     * Builds the reverse mapping from log_shape_id to schema_id by scanning schemas
     * for their NodeType::LogTypeID nodes.
     */
    void build_log_shape_id_to_schema_id_map();

    /**
     * Finds all child schema nodes whose key name matches the given name.
     * @param parent_id The parent schema node ID.
     * @param key_name The key name to match.
     * @return A vector of matching child node IDs (may be empty).
     */
    auto find_child_nodes_by_key_name(SchemaNode::id_t parent_id, std::string_view key_name)
            -> std::vector<SchemaNode::id_t>;

    /**
     * Iterates the log shape dictionary and returns schema IDs whose log shapes match a
     * predicate. For LogMessage nodes, the predicate receives the full log-shape value. For
     * ParentRule nodes, the predicate receives the log shape substring for the parent rule match of
     * `qualified_name`.
     * @tparam Matcher A callable `bool(std::string_view)` returning true if the argument matches.
     * @param qualified_name The fully qualified name of the node (empty for LogMessage).
     * @param log_shape_dict The log shape dictionary to scan.
     * @param matcher A Matcher returning true if a predicate matches a log-shape.
     * @return The set of schema IDs whose log shapes matched.
     */
    template <StringViewPredicate Matcher>
    auto find_schemas_matching_predicate(
            std::string_view qualified_name,
            clp_s::LogShapeDictionaryReader& log_shape_dict,
            Matcher const& matcher
    ) -> std::unordered_set<int32_t>;

    /**
     * Looks up a decomposed clpp query from the cache, lazily initializing the log-surgeon
     * parser and parsing specificatino on first use. The cache is keyed on the fully qualified
     * column name and the raw query string.
     * @param qualified_name The fully qualified dot-separated column name (e.g.
     * "message.block_id").
     * @param query The raw CLP-string query text.
     * @return A pointer to the cached DecomposedQuery on success.
     */
    auto lookup_decomposed_query(std::string const& qualified_name, std::string const& query)
            -> ystdlib::error_handling::Result<clpp::DecomposedQuery const*>;

    /**
     * Decomposes a CLP-string query at a LogMessage or ParentRule node, matches log shapes against
     * the dictionary, and returns an OrExpr of leaf filter expressions.
     *
     * Handles both direct LogMessage queries (message: "...") and ParentRule queries.
     * Wildcard expansion (message.*:...) is handled by the unresolved descriptor expansion in
     * populate_column_mapping, not here.
     *
     * Returns nullptr if no schemas match, decomposition fails, or a leaf column cannot be
     * resolved in the schema tree.
     * @param column The column triggering clpp decomposition.
     * @param root_node_id The schema-tree node where decomposition is rooted (LogMessage or
     *     ParentRule).
     * @param filter The FilterExpr containing the operation and operand.
     * @return The transformed expression on success, nullptr otherwise.
     */
    auto resolve_clpp_query(
            std::shared_ptr<ast::ColumnDescriptor> const& column,
            SchemaNode::id_t root_node_id,
            ast::FilterExpr const& filter
    ) -> std::shared_ptr<ast::Expression>;

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
    std::unordered_set<int32_t> m_array_schema_ids;
    std::unordered_set<int32_t> m_array_search_schema_ids;
    std::map<int32_t, std::shared_ptr<ast::Expression>> m_schema_to_query;

    std::unordered_map<int32_t, std::set<int32_t>> m_schema_to_searched_columns;
    std::shared_ptr<SchemaTree> m_tree;
    std::shared_ptr<ReaderUtils::SchemaMap> m_schemas;
    // TODO clpp: refactor m_tree and m_schemas
    std::shared_ptr<ArchiveReader> m_archive_reader;
    bool m_clpp_decomposed_query{false};
    std::string m_parsing_spec_str;
    log_surgeon::ParsingSpec* m_parsing_spec{nullptr};
    std::unique_ptr<log_surgeon::ParserHandle> m_parser;
    absl::flat_hash_map<std::pair<std::string, std::string>, clpp::DecomposedQuery>
            m_decomposed_query_cache;
    std::unordered_map<clpp::log_shape_id_t, std::vector<int32_t>> m_log_shape_id_to_schema_id;

    /**
     * Expands a wildcard at a CLPP node (LogMessage or ParentRule) by iterating
     * the node's children and dispatching each child to CLPP decomposition
     * (for ParentRule children) or direct leaf filters.
     *
     * @param resolved_column The resolved column descriptor for the CLPP node.
     * @param node The schema-tree node.
     * @param filter The FilterExpr containing the operation and operand.
     * @param possibilities The expression (must be an OrExpr) to add child results to.
     */
    void expand_clpp_node_children(
            std::shared_ptr<ast::ColumnDescriptor> const& resolved_column,
            SchemaNode const& node,
            ast::FilterExpr const& filter,
            std::shared_ptr<ast::Expression> const& possibilities
    );

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
     * Marks a column as CLPP-resolved and registers its schema-to-node-id mappings
     * in m_descriptor_to_schema. Must be called during the first pass so that the
     * correct schema subset is preserved — m_descriptor_to_schema is not cleared
     * between passes, and the is_clpp_resolved flag prevents re-registration in
     * m_column_to_descriptor which would cause populate_schema_mapping to inflate
     * the mappings.
     * @param column The column descriptor to register.
     * @param node_id The schema-tree node ID to anchor the column at.
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

template <StringViewPredicate Matcher>
auto SchemaMatch::find_schemas_matching_predicate(
        std::string_view qualified_name,
        clp_s::LogShapeDictionaryReader& log_shape_dict,
        Matcher const& matcher
) -> std::unordered_set<int32_t> {
    std::vector<clpp::log_shape_id_t> matched_shape_ids;
    for (auto const& log_shape : log_shape_dict.get_entries()) {
        auto const log_shape_str{std::string_view{log_shape.get_value()}};
        if (qualified_name.empty()) {
            if (matcher(log_shape_str)) {
                matched_shape_ids.emplace_back(log_shape.get_id());
            }
        } else {
            auto shapes{m_archive_reader->get_parent_rule_shapes().at(log_shape.get_id())};
            for (auto const& parent_match : shapes.get()) {
                if (qualified_name == parent_match.m_name
                    && matcher(log_shape_str.substr(parent_match.m_start, parent_match.m_size)))
                {
                    matched_shape_ids.emplace_back(log_shape.get_id());
                    break;
                }
            }
        }
    }
    std::unordered_set<int32_t> schema_ids;
    for (auto const id : matched_shape_ids) {
        if (auto const it{m_log_shape_id_to_schema_id.find(id)};
            m_log_shape_id_to_schema_id.end() != it)
        {
            schema_ids.insert(it->second.begin(), it->second.end());
        }
    }
    return schema_ids;
}
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_SCHEMAMATCH_HPP
