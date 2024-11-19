#ifndef CLP_S_SEARCH_OUTPUT_HPP
#define CLP_S_SEARCH_OUTPUT_HPP

#include <map>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <simdjson.h>

#include "../ArchiveReader.hpp"
#include "../SchemaReader.hpp"
#include "../Utils.hpp"
#include "clp_search/Query.hpp"
#include "Expression.hpp"
#include "OutputHandler.hpp"
#include "SchemaMatch.hpp"
#include "StringLiteral.hpp"

using namespace simdjson;
using namespace clp_s::search::clp_search;

namespace clp_s::search {
class Output : public FilterClass {
public:
    Output(SchemaMatch& match,
           std::shared_ptr<Expression> expr,
           std::shared_ptr<ArchiveReader> archive_reader,
           std::shared_ptr<TimestampDictionaryReader> timestamp_dict,
           std::unique_ptr<OutputHandler> output_handler,
           bool ignore_case)
            : m_archive_reader(std::move(archive_reader)),
              m_schema_tree(m_archive_reader->get_schema_tree()),
              m_schemas(m_archive_reader->get_schema_map()),
              m_match(match),
              m_expr(std::move(expr)),
              m_timestamp_dict(std::move(timestamp_dict)),
              m_output_handler(std::move(output_handler)),
              m_ignore_case(ignore_case),
              m_should_marshal_records(m_output_handler->should_marshal_records()) {}

    /**
     * Filters messages from all archives
     * @return Whether the filter was performed successfully
     */
    bool filter();

private:
    enum class ExpressionType {
        And,
        Or,
        Filter
    };

    std::shared_ptr<ArchiveReader> m_archive_reader;
    std::shared_ptr<Expression> m_expr;
    SchemaMatch& m_match;
    std::unique_ptr<OutputHandler> m_output_handler;
    bool m_ignore_case;
    bool m_should_marshal_records{true};

    // variables for the current schema being filtered
    int32_t m_schema;
    SchemaReader* m_reader;

    std::shared_ptr<SchemaTree> m_schema_tree;
    std::shared_ptr<VariableDictionaryReader> m_var_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_log_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_array_dict;
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dict;

    std::shared_ptr<ReaderUtils::SchemaMap> m_schemas;

    std::map<std::string, std::optional<Query>> m_string_query_map;
    std::map<std::string, std::unordered_set<int64_t>> m_string_var_match_map;
    std::unordered_map<Expression*, Query*> m_expr_clp_query;
    std::unordered_map<Expression*, std::unordered_set<int64_t>*> m_expr_var_match_map;
    std::unordered_map<int32_t, std::vector<ClpStringColumnReader*>> m_clp_string_readers;
    std::unordered_map<int32_t, std::vector<VariableStringColumnReader*>> m_var_string_readers;
    std::unordered_map<int32_t, DateStringColumnReader*> m_datestring_readers;
    std::unordered_map<int32_t, std::vector<BaseColumnReader*>> m_basic_readers;
    std::unordered_map<int32_t, std::string> m_extracted_unstructured_arrays;
    uint64_t m_cur_message;
    EvaluatedValue m_expression_value;

    std::vector<ColumnDescriptor*> m_wildcard_columns;
    std::map<ColumnDescriptor*, std::set<int32_t>> m_wildcard_to_searched_basic_columns;
    LiteralTypeBitmask m_wildcard_type_mask{0};
    std::unordered_set<int32_t> m_metadata_columns;

    std::stack<
            std::pair<ExpressionType, OpList::iterator>,
            std::vector<std::pair<ExpressionType, OpList::iterator>>>
            m_expression_state;

    simdjson::ondemand::parser m_array_parser;
    std::string m_array_search_string;
    bool m_maybe_string, m_maybe_number;

    /**
     * Initializes the variables. Init is called once for each schema after which filter is called
     * once for every message in the schema
     * @param reader
     * @param schema_id
     * @param column_readers
     */
    void init(
            SchemaReader* reader,
            int32_t schema_id,
            std::vector<BaseColumnReader*> const& column_readers
    ) override;

    /**
     * Evaluates an expression
     * @param expr
     * @param schema
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate(Expression* expr, int32_t schema);

    /**
     * Evaluates a filter expression
     * @param expr
     * @param schema
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_filter(FilterExpr* expr, int32_t schema);

    /**
     * Evaluates a wildcard filter expression
     * @param expr
     * @param schema
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_wildcard_filter(FilterExpr* expr, int32_t schema);

    /**
     * Evaluates a int filter expression
     * @param op
     * @param column_id
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_int_filter(
            FilterOperation op,
            int32_t column_id,
            std::shared_ptr<Literal> const& operand
    );

    /**
     * Evaluates a int filter expression
     * @param op
     * @param value
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    static bool evaluate_int_filter_core(FilterOperation op, int64_t value, int64_t operand);

    /**
     * Evaluates a float filter expression
     * @param op
     * @param column_id
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_float_filter(
            FilterOperation op,
            int32_t column_id,
            std::shared_ptr<Literal> const& operand
    );

    /**
     * Evaluates the core of a float filter expression
     * @param op
     * @param value
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    static bool evaluate_float_filter_core(FilterOperation op, double value, double operand);

    /**
     * Evaluates a clp string filter expression
     * @param op
     * @param q
     * @param readers
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_clp_string_filter(
            FilterOperation op,
            Query* q,
            std::vector<ClpStringColumnReader*> const& readers
    ) const;

    /**
     * Evaluates a var string filter expression
     * @param op
     * @param reader
     * @param matching_vars
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_var_string_filter(
            FilterOperation op,
            std::vector<VariableStringColumnReader*> const& readers,
            std::unordered_set<int64_t>* matching_vars
    ) const;

    /**
     * Evaluates a epoch date string filter expression
     * @param op
     * @param reader
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_epoch_date_filter(
            FilterOperation op,
            DateStringColumnReader* reader,
            std::shared_ptr<Literal>& operand
    );

    /**
     * Evaluates an array filter expression
     * @param op
     * @param unresolved_tokens
     * @param value
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_array_filter(
            FilterOperation op,
            DescriptorList const& unresolved_tokens,
            std::string& value,
            std::shared_ptr<Literal> const& operand
    );

    /**
     * Evaluates a filter expression on a single value for precise array search.
     * @param item
     * @param op
     * @param unresolved_tokens
     * @param cur_idx
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    inline bool evaluate_array_filter_value(
            ondemand::value& item,
            FilterOperation op,
            DescriptorList const& unresolved_tokens,
            size_t cur_idx,
            std::shared_ptr<Literal> const& operand
    ) const;

    /**
     * Evaluates a filter expression on an array (top level or nested) for precise array search.
     * @param array
     * @param op
     * @param unresolved_tokens
     * @param cur_idx
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_array_filter_array(
            ondemand::array& array,
            FilterOperation op,
            DescriptorList const& unresolved_tokens,
            size_t cur_idx,
            std::shared_ptr<Literal> const& operand
    ) const;

    /**
     * Evaluates a filter expression on an object inside of an array for precise array search.
     * @param object
     * @param op
     * @param unresolved_tokens
     * @param cur_idx
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_array_filter_object(
            ondemand::object& object,
            FilterOperation op,
            DescriptorList const& unresolved_tokens,
            size_t cur_idx,
            std::shared_ptr<Literal> const& operand
    ) const;

    /**
     * Evaluates a wildcard array filter expression
     * @param op
     * @param value
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_wildcard_array_filter(
            FilterOperation op,
            std::string& value,
            std::shared_ptr<Literal> const& operand
    );

    /**
     * The implementation of evaluate_wildcard_array_filter
     * @param array
     * @param op
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_wildcard_array_filter(
            ondemand::array& array,
            FilterOperation op,
            std::shared_ptr<Literal> const& operand
    ) const;

    /**
     * The implementation of evaluate_wildcard_array_filter
     * @param object
     * @param op
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_wildcard_array_filter(
            ondemand::object& object,
            FilterOperation op,
            std::shared_ptr<Literal> const& operand
    ) const;

    /**
     * Evaluates a bool filter expression
     * @param op
     * @param column_id
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    bool evaluate_bool_filter(
            FilterOperation op,
            int32_t column_id,
            std::shared_ptr<Literal> const& operand
    );

    /**
     * Populates the string queries
     * @param expr
     */
    void populate_string_queries(std::shared_ptr<Expression> const& expr);

    /**
     * Populates the set of internal columns that get ignored during dynamic wildcard expansion.
     */
    void populate_internal_columns();

    /**
     * Constant propagates an expression
     * @param expr
     * @param schema_id
     * @return EvaluatedValue::True if the expression evaluates to true, EvaluatedValue::False
     * if the expression evaluates to false, EvaluatedValue::Unknown otherwise
     */
    EvaluatedValue constant_propagate(std::shared_ptr<Expression> const& expr, int32_t schema_id);

    /**
     * Populates searched wildcard columns
     * @param expr
     */
    void populate_searched_wildcard_columns(std::shared_ptr<Expression> const& expr);

    /**
     * Adds wildcard columns to searched columns
     */
    void add_wildcard_columns_to_searched_columns();

    /**
     * Gets the cached decompressed structured array for the current message stored in the column
     * column_id. Decompressing array fields can be expensive, so this interface allows us to
     * decompress lazily, and decompress the field only once.
     *
     * Note: the string is returned by reference to allow our array search code to adjust the string
     * so that we have enough padding for simdjson.
     * @param column_id
     * @return the string representing the unstructured array stored in the column column_id
     */
    std::string& get_cached_decompressed_unstructured_array(int32_t column_id);

    // Methods inherited from FilterClass
    bool filter(uint64_t cur_message) override;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_OUTPUT_HPP
