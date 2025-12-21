#ifndef CLP_S_SEARCH_QUERYRUNNER_HPP
#define CLP_S_SEARCH_QUERYRUNNER_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <simdjson.h>

#include "../../clp/Query.hpp"
#include "../ArchiveReader.hpp"
#include "../ColumnReader.hpp"
#include "../DictionaryReader.hpp"
#include "../ReaderUtils.hpp"
#include "../SchemaReader.hpp"
#include "../SchemaTree.hpp"
#include "../TimestampDictionaryReader.hpp"
#include "../Utils.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/FilterOperation.hpp"
#include "ast/Literal.hpp"
#include "SchemaMatch.hpp"

namespace clp_s::search {
/**
 * This class is a core component of the log search system responsible for executing parsed queries
 * represented as abstract syntax trees (ASTs). It sets up the necessary context for each schema and
 * evaluates filter expressions against log messages from various schemas. It optimizes query
 * execution through techniques like constant propagation and pre-processing. It also leverages
 * dictionary lookups for efficient handling of string-based queries and supports wildcard and
 * array-based filtering.
 */
class QueryRunner : public FilterClass {
public:
    QueryRunner(
            std::shared_ptr<SchemaMatch> const& match,
            std::shared_ptr<ast::Expression> const& expr,
            std::shared_ptr<ArchiveReader> const& archive_reader,
            bool ignore_case
    )
            : m_archive_reader(archive_reader),
              m_expr(expr),
              m_match(match),
              m_ignore_case(ignore_case),
              m_schema_tree(m_archive_reader->get_schema_tree()),
              m_var_dict(m_archive_reader->get_variable_dictionary()),
              m_log_dict(m_archive_reader->get_log_type_dictionary()),
              m_array_dict(m_archive_reader->get_array_dictionary()),
              m_timestamp_dict(m_archive_reader->get_timestamp_dictionary()),
              m_schemas(m_archive_reader->get_schema_map()) {}

    // Destructor
    virtual ~QueryRunner() = default;

    QueryRunner(QueryRunner const&) = delete;
    auto operator=(QueryRunner const&) -> QueryRunner& = delete;

    QueryRunner(QueryRunner&&) = delete;
    auto operator=(QueryRunner&&) -> QueryRunner& = delete;

    /**
     * Initializes the query processing context that is common to all schemas.
     */
    void global_init();

    /**
     * Initializes the query processing context for a given schema.
     *
     * It clears any previous schema-specific data and initializes internal data structures required
     * for query execution based on the provided schema ID. Then it performs constant propagation on
     * the expression. If the expression evaluates to false, it returns EvaluatedValue::False.
     * Otherwise, it sets the wildcard matching type mask.
     *
     * @param schema_id
     */
    auto schema_init(int32_t schema_id) -> EvaluatedValue;

protected:
    // Methods inherited from FilterClass
    auto filter(uint64_t cur_message) -> bool override;

    /**
     * Clears all column readers.
     */
    void clear_readers();

    /**
     * Initializes and registers a column reader for a given column ID.
     *
     * @param column_id
     * @param column_reader
     */
    void initialize_reader(int32_t column_id, BaseColumnReader* column_reader);

private:
    enum class ExpressionType : uint8_t {
        And,
        Or,
        Filter
    };

    std::shared_ptr<ArchiveReader> m_archive_reader;
    std::shared_ptr<ast::Expression> m_expr;
    std::shared_ptr<SchemaMatch> m_match;
    bool m_ignore_case;

    // variables for the current schema being filtered
    int32_t m_schema{-1};
    SchemaReader* m_reader{nullptr};

    std::shared_ptr<SchemaTree> m_schema_tree;
    std::shared_ptr<VariableDictionaryReader> m_var_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_log_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_array_dict;
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dict;

    std::shared_ptr<ReaderUtils::SchemaMap> m_schemas;

    std::map<std::string, std::optional<clp::Query>> m_string_query_map;
    std::map<std::string, std::unordered_set<int64_t>> m_string_var_match_map;
    std::unordered_map<ast::Expression*, clp::Query*> m_expr_clp_query;
    std::unordered_map<ast::Expression*, std::unordered_set<int64_t>*> m_expr_var_match_map;
    std::unordered_map<int32_t, std::vector<ClpStringColumnReader*>> m_clp_string_readers;
    std::unordered_map<int32_t, std::vector<VariableStringColumnReader*>> m_var_string_readers;
    std::unordered_map<int32_t, TimestampColumnReader*> m_timestamp_readers;
    DeprecatedDateStringColumnReader* m_deprecated_datestring_reader{nullptr};
    std::unordered_map<int32_t, std::vector<BaseColumnReader*>> m_basic_readers;
    std::unordered_map<int32_t, std::string> m_extracted_unstructured_arrays;
    uint64_t m_cur_message{0};
    EvaluatedValue m_expression_value{EvaluatedValue::Unknown};

    std::vector<ast::ColumnDescriptor*> m_wildcard_columns;
    std::map<ast::ColumnDescriptor*, std::set<int32_t>> m_wildcard_to_searched_basic_columns;
    ast::literal_type_bitmask_t m_wildcard_type_mask{0};
    std::unordered_set<int32_t> m_metadata_columns;

    std::
            stack<std::pair<ExpressionType, ast::OpList::iterator>,
                  std::vector<std::pair<ExpressionType, ast::OpList::iterator>>>
                    m_expression_state;

    simdjson::ondemand::parser m_array_parser;
    std::string m_array_search_string;
    bool m_maybe_string{false};
    bool m_maybe_number{false};

    /**
     * Initializes the variables. Init is called once for each schema after which filter is called
     * once for every message in the schema
     * @param reader
     * @param column_readers
     */
    void init(SchemaReader* reader, std::vector<BaseColumnReader*> const& column_readers) override;

    /**
     * Evaluates an expression
     * @param expr
     * @param schema
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate(ast::Expression* expr, int32_t schema) -> bool;

    /**
     * Evaluates a filter expression
     * @param expr
     * @param schema
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_filter(ast::FilterExpr* expr, int32_t schema) -> bool;

    /**
     * Evaluates a wildcard filter expression
     * @param expr
     * @param schema
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_wildcard_filter(ast::FilterExpr* expr, int32_t schema) -> bool;

    /**
     * Evaluates a int filter expression
     * @param op
     * @param column_id
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_int_filter(
            ast::FilterOperation op,
            int32_t column_id,
            std::shared_ptr<ast::Literal> const& operand
    ) -> bool;

    /**
     * Evaluates a int filter expression
     * @param op
     * @param value
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    static auto evaluate_int_filter_core(ast::FilterOperation op, int64_t value, int64_t operand)
            -> bool;

    /**
     * Evaluates a float filter expression
     * @param op
     * @param column_id
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_float_filter(
            ast::FilterOperation op,
            int32_t column_id,
            std::shared_ptr<ast::Literal> const& operand
    ) -> bool;

    /**
     * Evaluates the core of a float filter expression
     * @param op
     * @param value
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    static auto evaluate_float_filter_core(ast::FilterOperation op, double value, double operand)
            -> bool;

    /**
     * Evaluates a clp string filter expression
     * @param op
     * @param q
     * @param readers
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_clp_string_filter(
            ast::FilterOperation op,
            clp::Query* q,
            std::vector<ClpStringColumnReader*> const& readers
    ) const -> bool;

    /**
     * Evaluates a var string filter expression
     * @param op
     * @param readers
     * @param matching_vars
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_var_string_filter(
            ast::FilterOperation op,
            std::vector<VariableStringColumnReader*> const& readers,
            std::unordered_set<int64_t>* matching_vars
    ) const -> bool;

    /**
     * Evaluates a epoch date string filter expression
     * @param op
     * @param reader
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_epoch_date_filter(
            ast::FilterOperation op,
            DeprecatedDateStringColumnReader* reader,
            std::shared_ptr<ast::Literal>& operand
    ) -> bool;

    /**
     * Evaluates a timestamp filter.
     * @param op
     * @param reader
     * @param operand
     * @return Whether the filter evaluates to true.
     */
    auto evaluate_timestamp_filter(
            ast::FilterOperation op,
            TimestampColumnReader* reader,
            std::shared_ptr<ast::Literal>& operand
    ) -> bool;

    /**
     * Evaluates an array filter expression
     * @param op
     * @param unresolved_tokens
     * @param value
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_array_filter(
            ast::FilterOperation op,
            ast::DescriptorList const& unresolved_tokens,
            std::string& value,
            std::shared_ptr<ast::Literal> const& operand
    ) -> bool;

    /**
     * Evaluates a filter expression on a single value for precise array search.
     * @param item
     * @param op
     * @param unresolved_tokens
     * @param cur_idx
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    inline auto evaluate_array_filter_value(
            simdjson::ondemand::value& item,
            ast::FilterOperation op,
            ast::DescriptorList const& unresolved_tokens,
            size_t cur_idx,
            std::shared_ptr<ast::Literal> const& operand
    ) const -> bool;

    /**
     * Evaluates a filter expression on an array (top level or nested) for precise array search.
     * @param array
     * @param op
     * @param unresolved_tokens
     * @param cur_idx
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_array_filter_array(
            simdjson::ondemand::array& array,
            ast::FilterOperation op,
            ast::DescriptorList const& unresolved_tokens,
            size_t cur_idx,
            std::shared_ptr<ast::Literal> const& operand
    ) const -> bool;

    /**
     * Evaluates a filter expression on an object inside of an array for precise array search.
     * @param object
     * @param op
     * @param unresolved_tokens
     * @param cur_idx
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_array_filter_object(
            simdjson::ondemand::object& object,
            ast::FilterOperation op,
            ast::DescriptorList const& unresolved_tokens,
            size_t cur_idx,
            std::shared_ptr<ast::Literal> const& operand
    ) const -> bool;

    /**
     * Evaluates a wildcard array filter expression
     * @param op
     * @param value
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_wildcard_array_filter(
            ast::FilterOperation op,
            std::string& value,
            std::shared_ptr<ast::Literal> const& operand
    ) -> bool;

    /**
     * The implementation of evaluate_wildcard_array_filter
     * @param array
     * @param op
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_wildcard_array_filter(
            simdjson::ondemand::array& array,
            ast::FilterOperation op,
            std::shared_ptr<ast::Literal> const& operand
    ) const -> bool;

    /**
     * The implementation of evaluate_wildcard_array_filter
     * @param object
     * @param op
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_wildcard_array_filter(
            simdjson::ondemand::object& object,
            ast::FilterOperation op,
            std::shared_ptr<ast::Literal> const& operand
    ) const -> bool;

    /**
     * Evaluates a bool filter expression
     * @param op
     * @param column_id
     * @param operand
     * @return true if the expression evaluates to true, false otherwise
     */
    auto evaluate_bool_filter(
            ast::FilterOperation op,
            int32_t column_id,
            std::shared_ptr<ast::Literal> const& operand
    ) -> bool;

    /**
     * Populates the string queries
     * @param expr
     */
    void populate_string_queries(std::shared_ptr<ast::Expression> const& expr);

    /**
     * Populates the set of internal columns that get ignored during dynamic wildcard expansion.
     */
    void populate_internal_columns();

    /**
     * Constant propagates an expression
     * @param expr
     * @return EvaluatedValue::True if the expression evaluates to true, EvaluatedValue::False
     * if the expression evaluates to false, EvaluatedValue::Unknown otherwise
     */
    auto constant_propagate(std::shared_ptr<ast::Expression> const& expr) -> EvaluatedValue;

    /**
     * Populates searched wildcard columns
     * @param expr
     */
    void populate_searched_wildcard_columns(std::shared_ptr<ast::Expression> const& expr);

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
    auto get_cached_decompressed_unstructured_array(int32_t column_id) -> std::string&;
};
}  // namespace clp_s::search
#endif
