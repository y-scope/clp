#ifndef CLP_S_SEARCH_SCHEMAMATCH_HPP
#define CLP_S_SEARCH_SCHEMAMATCH_HPP

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "../ReaderUtils.hpp"
#include "ColumnDescriptor.hpp"
#include "Expression.hpp"
#include "FilterExpr.hpp"
#include "Literal.hpp"
#include "Transformation.hpp"

namespace clp_s::search {
class SchemaMatch : public Transformation {
public:
    // Constructor
    SchemaMatch(std::shared_ptr<SchemaTree> tree, std::shared_ptr<ReaderUtils::SchemaMap> schemas);

    /**
     * Runs the transformation on an expression
     * @param expr
     * @return The transformed expression
     */
    std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) override;

    /**
     * @param schema
     * @return The query for a given schema
     */
    std::shared_ptr<Expression> get_query_for_schema(int32_t schema);

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
    std::unordered_map<uint32_t, std::set<ColumnDescriptor*>> m_column_to_descriptor;
    // TODO: The value in the map can be a set of k:v pairs with a hash & comparison
    // that only considers the key since each column descriptor only has one matching
    // column id per schema
    std::unordered_map<ColumnDescriptor*, std::map<int32_t, int32_t>> m_descriptor_to_schema;
    std::map<ColumnDescriptor*, std::set<int32_t>> m_unresolved_descriptor_to_descriptor;
    std::unordered_map<Expression*, std::unordered_set<int32_t>> m_expression_to_schemas;
    std::unordered_set<int32_t> m_matched_schema_ids;
    std::unordered_set<int32_t> m_array_schema_ids;
    std::unordered_set<int32_t> m_array_search_schema_ids;
    std::map<int32_t, std::shared_ptr<Expression>> m_schema_to_query;

    std::unordered_map<int32_t, std::set<int32_t>> m_schema_to_searched_columns;
    std::shared_ptr<SchemaTree> m_tree;
    std::shared_ptr<ReaderUtils::SchemaMap> m_schemas;

    /**
     * Populates the column mapping for a given column
     * @param column
     * @param node_id
     * @return true if matching is successful, false otherwise
     */
    bool populate_column_mapping(ColumnDescriptor* column, int32_t node_id);

    /**
     * Populates the column mapping for a given column
     * @param column
     * @return
     */
    bool populate_column_mapping(ColumnDescriptor* column);

    /**
     * Populates the column mapping for a given expression
     * @param cur
     * @return The transformed expression
     */
    std::shared_ptr<Expression> populate_column_mapping(std::shared_ptr<Expression> cur);

    /**
     * Populates the schema mapping
     */
    void populate_schema_mapping();

    /**
     * Finds common schemas and relevant columns across filters and stores the mapping
     * @param cur
     * @return The transformed expression
     */
    std::shared_ptr<Expression> intersect_schemas(std::shared_ptr<Expression> cur);

    /**
     * Finds common schemas and relevant columns across filters
     * @param cur
     * @param common_schema
     * @param columns
     * @param first
     * @return true before firstly processing common schemas, false otherwise
     */
    bool intersect_and_sub_expr(
            std::shared_ptr<Expression> const& cur,
            std::set<int32_t>& common_schema,
            std::set<ColumnDescriptor*>& columns,
            bool first
    );

    /**
     * Splits an expression into sub-expressions based on the schemas it searches against
     * @param expr
     * @param queries a map from schema id to expression
     * @param relevant_schemas
     */
    void split_expression_by_schema(
            std::shared_ptr<Expression> const& expr,
            std::map<int32_t, std::shared_ptr<Expression>>& queries,
            std::unordered_set<int32_t> const& relevant_schemas
    );

    /**
     * @param column
     * @param schema
     * @return The column id for a given column descriptor
     */
    int32_t get_column_id_for_descriptor(ColumnDescriptor* column, int32_t schema);

    /**
     * @param column
     * @param schema
     * @return The literal type for a given column descriptor
     */
    LiteralType get_literal_type_for_column(ColumnDescriptor* column, int32_t schema);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_SCHEMAMATCH_HPP
