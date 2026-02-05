#include "QueryRunner.hpp"

#include <memory>
#include <vector>

#include <log_surgeon/Lexer.hpp>
#include <string_utils/string_utils.hpp>

#include "../../clp/Defs.h"
#include "../../clp/GrepCore.hpp"
#include "../../clp/Query.hpp"
#include "../../clp/type_utils.hpp"
#include "../SchemaTree.hpp"
#include "../Utils.hpp"
#include "ast/AndExpr.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/FilterOperation.hpp"
#include "ast/Literal.hpp"
#include "ast/OrExpr.hpp"
#include "ast/SearchUtils.hpp"
#include "EvaluateTimestampIndex.hpp"

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::DescriptorList;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::Literal;
using clp_s::search::ast::literal_type_bitmask_t;
using clp_s::search::ast::LiteralType;
using clp_s::search::ast::OpList;
using clp_s::search::ast::OrExpr;

#define eval(op, a, b) (((op) == FilterOperation::EQ) ? ((a) == (b)) : ((a) != (b)))

namespace clp_s::search {
void QueryRunner::global_init() {
    populate_internal_columns();
    populate_string_queries(m_expr);
}

auto QueryRunner::schema_init(int32_t schema_id) -> EvaluatedValue {
    m_expr_clp_query.clear();
    m_expr_var_match_map.clear();
    m_wildcard_to_searched_basic_columns.clear();
    m_wildcard_columns.clear();
    m_expr = m_match->get_query_for_schema(schema_id)->copy();
    m_schema = schema_id;
    populate_searched_wildcard_columns(m_expr);

    m_expression_value = constant_propagate(m_expr);
    if (m_expression_value == EvaluatedValue::False) {
        return m_expression_value;
    }

    add_wildcard_columns_to_searched_columns();
    return m_expression_value;
}

void QueryRunner::clear_readers() {
    m_clp_string_readers.clear();
    m_var_string_readers.clear();
    m_datestring_readers.clear();
    m_basic_readers.clear();
}

void QueryRunner::initialize_reader(int32_t column_id, BaseColumnReader* column_reader) {
    if ((0
         != (m_wildcard_type_mask
             & node_to_literal_type(m_schema_tree->get_node(column_id).get_type())))
        || m_match->schema_searches_against_column(m_schema, column_id))
    {
        auto* clp_reader = dynamic_cast<ClpStringColumnReader*>(column_reader);
        auto* var_reader = dynamic_cast<VariableStringColumnReader*>(column_reader);
        auto* date_reader = dynamic_cast<DateStringColumnReader*>(column_reader);
        if (nullptr != clp_reader
            && (NodeType::ClpString == clp_reader->get_type()
                || NodeType::LogType == clp_reader->get_type()))
        {
            m_clp_string_readers[column_id].push_back(clp_reader);
        } else if (nullptr != var_reader && var_reader->get_type() == NodeType::VarString) {
            m_var_string_readers[column_id].push_back(var_reader);
        } else if (nullptr != date_reader) {
            // Datestring readers with a given column ID are guaranteed not to repeat
            m_datestring_readers.emplace(column_id, date_reader);
        } else {
            m_basic_readers[column_id].push_back(column_reader);
        }
    }
}

void QueryRunner::init(SchemaReader* reader, std::vector<BaseColumnReader*> const& column_readers) {
    m_reader = reader;

    clear_readers();

    for (auto column_reader : column_readers) {
        auto column_id = column_reader->get_id();
        initialize_reader(column_id, column_reader);
    }
}

std::string& QueryRunner::get_cached_decompressed_unstructured_array(int32_t column_id) {
    auto it = m_extracted_unstructured_arrays.find(column_id);
    if (m_extracted_unstructured_arrays.end() != it) {
        return it->second;
    }

    // Unstructured arrays with the same column id can not appear multiple times in one schema
    // in the current implementation.
    auto rit = m_extracted_unstructured_arrays.emplace(
            column_id,
            std::get<std::string>(m_basic_readers[column_id][0]->extract_value(m_cur_message))
    );
    return rit.first->second;
}

bool QueryRunner::filter(uint64_t cur_message) {
    m_cur_message = cur_message;
    m_extracted_unstructured_arrays.clear();
    return evaluate(m_expr.get(), m_schema);
}

bool QueryRunner::evaluate(Expression* expr, int32_t schema) {
    if (m_expression_value == EvaluatedValue::True) {
        return true;
    }

    Expression* cur = expr;
    ExpressionType cur_type = ExpressionType::Filter;
    bool ret = false;

    if (dynamic_cast<AndExpr*>(cur)) {
        cur_type = ExpressionType::And;
        m_expression_state.emplace(cur_type, cur->op_begin());
        ret = true;
    } else if (dynamic_cast<OrExpr*>(cur)) {
        cur_type = ExpressionType::Or;
        m_expression_state.emplace(cur_type, cur->op_begin());
        ret = false;
    }

    do {
        switch (cur_type) {
            case ExpressionType::And:
                if (false == ret || m_expression_state.top().second == cur->op_end()) {
                    m_expression_state.pop();
                    break;
                } else {
                    cur = static_cast<Expression*>((m_expression_state.top().second++)->get());
                    if (dynamic_cast<FilterExpr*>(cur)) {
                        cur_type = ExpressionType::Filter;
                    } else {
                        // must be an OR-expr because AST would have been simplified
                        // to eliminate nested AND
                        cur_type = ExpressionType::Or;
                        m_expression_state.emplace(cur_type, cur->op_begin());
                        ret = false;
                    }
                    continue;
                }
            case ExpressionType::Filter:
                if (static_cast<FilterExpr*>(cur)->get_column()->is_pure_wildcard()) {
                    ret = evaluate_wildcard_filter(static_cast<FilterExpr*>(cur), schema);
                } else {
                    ret = evaluate_filter(static_cast<FilterExpr*>(cur), schema);
                }
                break;
            case ExpressionType::Or:
                if (ret || m_expression_state.top().second == cur->op_end()) {
                    m_expression_state.pop();
                    break;
                } else {
                    cur = static_cast<Expression*>((m_expression_state.top().second++)->get());
                    if (dynamic_cast<FilterExpr*>(cur)) {
                        cur_type = ExpressionType::Filter;
                    } else {
                        // must be an AND-expr because AST would have been simplified
                        // to eliminate nested OR
                        cur_type = ExpressionType::And;
                        m_expression_state.emplace(cur_type, cur->op_begin());
                        ret = true;
                    }
                    continue;
                }
        }

        ret = cur->is_inverted() ? !ret : ret;
        if (false == m_expression_state.empty()) {
            cur_type = m_expression_state.top().first;
        }
        cur = cur->get_parent();
    } while (cur != nullptr);

    return ret;
}

bool QueryRunner::evaluate_wildcard_filter(FilterExpr* expr, int32_t schema) {
    auto literal = expr->get_operand();
    auto* column = expr->get_column().get();
    auto op = expr->get_operation();
    auto const& subtree_type{column->get_subtree_type()};
    auto const matches_metadata{
            subtree_type.has_value() && constants::cMetadataSubtreeType == subtree_type.value()
    };
    if (column->matches_type(LiteralType::ClpStringT)) {
        auto* q = m_expr_clp_query.at(expr);
        for (auto const& entry : m_clp_string_readers) {
            if (false == matches_metadata && m_metadata_columns.contains(entry.first)) {
                continue;
            }
            if (evaluate_clp_string_filter(op, q, entry.second)) {
                return true;
            }
        }
    }

    if (column->matches_type(LiteralType::VarStringT)) {
        std::unordered_set<int64_t>* matching_vars = m_expr_var_match_map[expr];
        for (auto const& entry : m_var_string_readers) {
            if (false == matches_metadata && m_metadata_columns.contains(entry.first)) {
                continue;
            }
            if (evaluate_var_string_filter(op, entry.second, matching_vars)) {
                return true;
            }
        }
    }

    if (column->matches_type(LiteralType::TimestampT)) {
        for (auto entry : m_datestring_readers) {
            if (false == matches_metadata && m_metadata_columns.contains(entry.first)) {
                continue;
            }
            if (evaluate_epoch_date_filter(op, entry.second, literal)) {
                return true;
            }
        }
    }

    m_maybe_number = expr->get_column()->matches_type(LiteralType::FloatT);
    for (int32_t column_id : m_wildcard_to_searched_basic_columns[column]) {
        if (false == matches_metadata && m_metadata_columns.contains(column_id)) {
            continue;
        }
        bool ret = false;
        switch (node_to_literal_type(m_schema_tree->get_node(column_id).get_type())) {
            case LiteralType::IntegerT:
                ret = evaluate_int_filter(op, column_id, literal);
                break;
            case LiteralType::FloatT:
                ret = evaluate_float_filter(op, column_id, literal);
                break;
            case LiteralType::BooleanT:
                ret = evaluate_bool_filter(op, column_id, literal);
                break;
            case LiteralType::ArrayT:
                ret = evaluate_wildcard_array_filter(
                        op,
                        get_cached_decompressed_unstructured_array(column_id),
                        literal
                );
                break;
            default:
                break;
        }

        if (ret) {
            return true;
        }
    }

    return false;
}

bool QueryRunner::evaluate_filter(FilterExpr* expr, int32_t schema) {
    auto* column = expr->get_column().get();
    int32_t column_id = column->get_column_id();
    auto literal = expr->get_operand();
    clp::Query* q = nullptr;
    std::unordered_set<int64_t>* matching_vars = nullptr;
    switch (column->get_literal_type()) {
        case LiteralType::IntegerT:
            return evaluate_int_filter(expr->get_operation(), column_id, literal);
        case LiteralType::FloatT:
            return evaluate_float_filter(expr->get_operation(), column_id, literal);
        case LiteralType::ClpStringT:
            q = m_expr_clp_query.at(expr);
            return evaluate_clp_string_filter(
                    expr->get_operation(),
                    q,
                    m_clp_string_readers[column_id]
            );
        case LiteralType::VarStringT:
            matching_vars = m_expr_var_match_map.at(expr);
            return evaluate_var_string_filter(
                    expr->get_operation(),
                    m_var_string_readers[column_id],
                    matching_vars
            );
        case LiteralType::BooleanT:
            return evaluate_bool_filter(expr->get_operation(), column_id, literal);
        case LiteralType::ArrayT:
            return evaluate_array_filter(
                    expr->get_operation(),
                    column->get_unresolved_tokens(),
                    get_cached_decompressed_unstructured_array(column_id),
                    literal
            );
        case LiteralType::TimestampT:
            return evaluate_epoch_date_filter(
                    expr->get_operation(),
                    m_datestring_readers[column_id],
                    literal
            );
            // case LiteralType::NullT:
            //  null checks are always turned into existence operators --
            //  no need to evaluate here
        default:
            return false;
    }
}

bool QueryRunner::evaluate_int_filter(
        FilterOperation op,
        int32_t column_id,
        std::shared_ptr<Literal> const& operand
) {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    int64_t op_value;
    if (false == operand->as_int(op_value, op)) {
        return false;
    }

    for (BaseColumnReader* reader : m_basic_readers[column_id]) {
        int64_t value = std::get<int64_t>(reader->extract_value(m_cur_message));
        if (evaluate_int_filter_core(op, value, op_value)) {
            return true;
        }
    }
    return false;
}

bool QueryRunner::evaluate_int_filter_core(FilterOperation op, int64_t value, int64_t operand) {
    switch (op) {
        case FilterOperation::EQ:
            return value == operand;
        case FilterOperation::NEQ:
            return value != operand;
        case FilterOperation::LT:
            return value < operand;
        case FilterOperation::GT:
            return value > operand;
        case FilterOperation::LTE:
            return value <= operand;
        case FilterOperation::GTE:
            return value >= operand;
        default:
            return false;
    }
}

bool QueryRunner::evaluate_float_filter(
        FilterOperation op,
        int32_t column_id,
        std::shared_ptr<Literal> const& operand
) {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    double op_value;
    if (false == operand->as_float(op_value, op)) {
        return false;
    }

    for (BaseColumnReader* reader : m_basic_readers[column_id]) {
        double value = std::get<double>(reader->extract_value(m_cur_message));
        if (evaluate_float_filter_core(op, value, op_value)) {
            return true;
        }
    }
    return false;
}

bool QueryRunner::evaluate_float_filter_core(FilterOperation op, double value, double operand) {
    switch (op) {
        case FilterOperation::EQ:
            return value == operand;
        case FilterOperation::NEQ:
            return value != operand;
        case FilterOperation::LT:
            return value < operand;
        case FilterOperation::GT:
            return value > operand;
        case FilterOperation::LTE:
            return value <= operand;
        case FilterOperation::GTE:
            return value >= operand;
        default:
            return false;
    }
}

bool QueryRunner::evaluate_clp_string_filter(
        FilterOperation op,
        clp::Query* q,
        std::vector<ClpStringColumnReader*> const& readers
) const {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    if (op != FilterOperation::EQ && op != FilterOperation::NEQ) {
        return false;
    }

    if (nullptr == q) {
        return op == FilterOperation::NEQ;
    }

    if (q->search_string_matches_all()) {
        return op == FilterOperation::EQ;
    }

    bool matched = false;
    for (ClpStringColumnReader* reader : readers) {
        int64_t id = reader->get_encoded_id(m_cur_message);
        auto vars = reader->get_encoded_vars(m_cur_message);
        if (q->contains_sub_queries()) {
            for (auto const& subquery : q->get_sub_queries()) {
                if (subquery.matches_logtype(id) && subquery.matches_vars(vars)) {
                    if (subquery.wildcard_match_required()) {
                        matched = clp::string_utils::wildcard_match_unsafe(
                                std::get<std::string>(reader->extract_value(m_cur_message)),
                                q->get_search_string(),
                                !q->get_ignore_case()
                        );
                    } else {
                        matched = true;
                    }
                    break;
                }
            }
        } else {
            matched = clp::string_utils::wildcard_match_unsafe(
                    std::get<std::string>(reader->extract_value(m_cur_message)),
                    q->get_search_string(),
                    !q->get_ignore_case()
            );
        }

        if ((op == FilterOperation::EQ) == matched) {
            return true;
        }
    }
    return false;
}

bool QueryRunner::evaluate_var_string_filter(
        FilterOperation op,
        std::vector<VariableStringColumnReader*> const& readers,
        std::unordered_set<int64_t>* matching_vars
) const {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    if (FilterOperation::EQ != op && FilterOperation::NEQ != op) {
        return false;
    }

    for (VariableStringColumnReader* reader : readers) {
        int64_t id = reader->get_variable_id(m_cur_message);
        bool matched = matching_vars->count(id) > 0;

        if ((FilterOperation::EQ == op) == matched) {
            return true;
        }
    }
    return false;
}

bool QueryRunner::evaluate_array_filter(
        FilterOperation op,
        DescriptorList const& unresolved_tokens,
        std::string& value,
        std::shared_ptr<Literal> const& operand
) {
    if (value.capacity() < (value.size() + simdjson::SIMDJSON_PADDING)) {
        value.reserve(value.size() + simdjson::SIMDJSON_PADDING);
    }
    auto obj = m_array_parser.iterate(value);
    simdjson::ondemand::array array = obj.get_array();

    // pre-evaluate whether we can match strings or numbers to eliminate
    // duplicate effort on every item
    m_maybe_string = !(op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS)
                     && (operand->as_var_string(m_array_search_string, op)
                         || operand->as_clp_string(m_array_search_string, op));
    double tmp_double;
    int64_t tmp_int;
    m_maybe_number = !(op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS)
                     && (operand->as_float(tmp_double, op) || operand->as_int(tmp_int, op));

    return evaluate_array_filter_array(array, op, unresolved_tokens, 0, operand);
}

bool QueryRunner::evaluate_array_filter_value(
        simdjson::ondemand::value& item,
        FilterOperation op,
        DescriptorList const& unresolved_tokens,
        size_t cur_idx,
        std::shared_ptr<Literal> const& operand
) const {
    bool match = false;
    switch (item.type()) {
        case simdjson::ondemand::json_type::object: {
            simdjson::ondemand::object nested_object = item.get_object();
            if (evaluate_array_filter_object(
                        nested_object,
                        op,
                        unresolved_tokens,
                        cur_idx,
                        operand
                ))
            {
                match = true;
            }
        } break;
        case simdjson::ondemand::json_type::array: {
            simdjson::ondemand::array nested_array = item.get_array();
            if (evaluate_array_filter_array(nested_array, op, unresolved_tokens, cur_idx, operand))
            {
                match = true;
            }
        } break;
        case simdjson::ondemand::json_type::string: {
            if (true == m_maybe_string && unresolved_tokens.size() == cur_idx
                && clp::string_utils::wildcard_match_unsafe(
                        item.get_string().value(),
                        m_array_search_string,
                        false == m_ignore_case
                ))
            {
                match = op == FilterOperation::EQ;
            }
        } break;
        case simdjson::ondemand::json_type::number: {
            if (false == m_maybe_number || unresolved_tokens.size() != cur_idx) {
                break;
            }
            simdjson::ondemand::number number = item.get_number();
            if (number.is_double()) {
                double tmp_double;
                operand->as_float(tmp_double, op);
                match = eval(op, number.get_double(), tmp_double);
            } else if (number.is_uint64()) {
                int64_t tmp_int;
                operand->as_int(tmp_int, op);
                match = eval(op, number.get_uint64(), tmp_int);
            } else {
                int64_t tmp_int;
                operand->as_int(tmp_int, op);
                // TODO: once we properly support unsigned at at least the AST level we should
                // replace this with something like operand->as_uint(tmp_uint)
                uint64_t tmp_uint = bit_cast<uint64_t, int64_t>(tmp_int);
                match = eval(op, number.get_int64(), tmp_uint);
            }
        } break;
        case simdjson::ondemand::json_type::boolean: {
            if (unresolved_tokens.size() != cur_idx || op == FilterOperation::EXISTS
                || op == FilterOperation::NEXISTS)
            {
                break;
            }
            bool tmp_bool;
            if (operand->as_bool(tmp_bool, op) && eval(op, item.get_bool(), tmp_bool)) {
                match = true;
            }
        } break;
        case simdjson::ondemand::json_type::null: {
            if (op != FilterOperation::EXISTS && op != FilterOperation::NEXISTS
                && operand->as_null(op))
            {
                match = op == FilterOperation::EQ;
            }
        } break;
    }
    return match;
}

bool QueryRunner::evaluate_array_filter_array(
        simdjson::ondemand::array& array,
        FilterOperation op,
        DescriptorList const& unresolved_tokens,
        size_t cur_idx,
        std::shared_ptr<Literal> const& operand
) const {
    for (simdjson::ondemand::value item : array) {
        if (evaluate_array_filter_value(item, op, unresolved_tokens, cur_idx, operand)) {
            return true;
        }
    }
    return false;
}

bool QueryRunner::evaluate_array_filter_object(
        simdjson::ondemand::object& object,
        FilterOperation op,
        DescriptorList const& unresolved_tokens,
        size_t cur_idx,
        std::shared_ptr<Literal> const& operand
) const {
    if (cur_idx >= unresolved_tokens.size()) {
        return false;
    }

    for (auto field : object) {
        if (field.unescaped_key(true).value() != unresolved_tokens[cur_idx].get_token()) {
            continue;
        }

        cur_idx += 1;
        if (cur_idx == unresolved_tokens.size()
            && (op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS))
        {
            return op == FilterOperation::EXISTS;
        }

        simdjson::ondemand::value item = field.value();
        return evaluate_array_filter_value(item, op, unresolved_tokens, cur_idx, operand);
    }
    return false;
}

bool QueryRunner::evaluate_wildcard_array_filter(
        FilterOperation op,
        std::string& value,
        std::shared_ptr<Literal> const& operand
) {
    if (value.capacity() < (value.size() + simdjson::SIMDJSON_PADDING)) {
        value.reserve(value.size() + simdjson::SIMDJSON_PADDING);
    }
    auto obj = m_array_parser.iterate(value);
    simdjson::ondemand::array array = obj.get_array();

    // pre-evaluate whether we can match strings or numbers to eliminate
    // duplicate effort on every item
    m_maybe_string = operand->as_var_string(m_array_search_string, op)
                     || operand->as_clp_string(m_array_search_string, op);

    return evaluate_wildcard_array_filter(array, op, operand);
}

bool QueryRunner::evaluate_wildcard_array_filter(
        simdjson::ondemand::array& array,
        FilterOperation op,
        std::shared_ptr<Literal> const& operand
) const {
    bool match = false;
    for (auto item : array) {
        switch (item.type()) {
            case simdjson::ondemand::json_type::object: {
                simdjson::ondemand::object nested_object = item.get_object();
                if (evaluate_wildcard_array_filter(nested_object, op, operand)) {
                    match = true;
                }
            } break;
            case simdjson::ondemand::json_type::array: {
                simdjson::ondemand::array nested_array = item.get_array();
                if (evaluate_wildcard_array_filter(nested_array, op, operand)) {
                    match = true;
                }
            } break;
            case simdjson::ondemand::json_type::string: {
                if (false == m_maybe_string) {
                    break;
                }
                if (clp::string_utils::wildcard_match_unsafe(
                            item.get_string().value(),
                            m_array_search_string,
                            false == m_ignore_case
                    ))
                {
                    match |= op == FilterOperation::EQ;
                }
                break;
            } break;
            case simdjson::ondemand::json_type::number: {
                if (false == m_maybe_number) {
                    break;
                }
                simdjson::ondemand::number number = item.get_number();
                if (number.is_double()) {
                    double tmp_double;
                    operand->as_float(tmp_double, op);
                    match |= eval(op, number.get_double(), tmp_double);
                } else if (number.is_uint64()) {
                    int64_t tmp_int;
                    operand->as_int(tmp_int, op);
                    match |= eval(op, number.get_uint64(), tmp_int);
                } else {
                    int64_t tmp_int;
                    operand->as_int(tmp_int, op);
                    match |= eval(op, number.get_int64(), tmp_int);
                }
            } break;
            case simdjson::ondemand::json_type::boolean: {
                bool tmp;
                if (operand->as_bool(tmp, op) && eval(op, item.get_bool(), tmp)) {
                    match = true;
                }
            } break;
            case simdjson::ondemand::json_type::null:
                if (operand->as_null(op)) {
                    match |= op == FilterOperation::EQ;
                }
                break;
        }

        if (match) {
            return true;
        }
    }
    return false;
}

bool QueryRunner::evaluate_wildcard_array_filter(
        simdjson::ondemand::object& object,
        FilterOperation op,
        std::shared_ptr<Literal> const& operand
) const {
    bool match = false;
    for (auto field : object) {
        simdjson::ondemand::value item = field.value();
        switch (item.type()) {
            case simdjson::ondemand::json_type::object: {
                simdjson::ondemand::object nested_object = item.get_object();
                if (evaluate_wildcard_array_filter(nested_object, op, operand)) {
                    match = true;
                }
            } break;
            case simdjson::ondemand::json_type::array: {
                simdjson::ondemand::array nested_array = item.get_array();
                if (evaluate_wildcard_array_filter(nested_array, op, operand)) {
                    match = true;
                }
            } break;
            case simdjson::ondemand::json_type::string: {
                if (false == m_maybe_string) {
                    break;
                }
                if (clp::string_utils::wildcard_match_unsafe(
                            item.get_string().value(),
                            m_array_search_string,
                            false == m_ignore_case
                    ))
                {
                    match |= op == FilterOperation::EQ;
                }
                break;
            } break;
            case simdjson::ondemand::json_type::number: {
                if (false == m_maybe_number) {
                    break;
                }
                simdjson::ondemand::number number = item.get_number();
                if (number.is_double()) {
                    double tmp_double;
                    operand->as_float(tmp_double, op);
                    match |= eval(op, number.get_double(), tmp_double);
                } else if (number.is_uint64()) {
                    int64_t tmp_int;
                    operand->as_int(tmp_int, op);
                    match |= eval(op, number.get_uint64(), tmp_int);
                } else {
                    int64_t tmp_int;
                    operand->as_int(tmp_int, op);
                    match |= eval(op, number.get_int64(), tmp_int);
                }
            } break;
            case simdjson::ondemand::json_type::boolean: {
                bool tmp;
                if (operand->as_bool(tmp, op) && eval(op, item.get_bool(), tmp)) {
                    match = true;
                }
            } break;
            case simdjson::ondemand::json_type::null:
                if (operand->as_null(op)) {
                    match |= op == FilterOperation::EQ;
                }
                break;
        }

        if (match) {
            return true;
        }
    }
    return false;
}

bool QueryRunner::evaluate_bool_filter(
        FilterOperation op,
        int32_t column_id,
        std::shared_ptr<Literal> const& operand
) {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    bool op_value;
    if (false == operand->as_bool(op_value, op)) {
        return false;
    }

    bool rvalue = false;
    for (BaseColumnReader* reader : m_basic_readers[column_id]) {
        bool value = std::get<uint8_t>(reader->extract_value(m_cur_message));
        switch (op) {
            case FilterOperation::EQ:
                rvalue = value == op_value;
                break;
            case FilterOperation::NEQ:
                rvalue = value != op_value;
                break;
            default:
                rvalue = false;
                break;
        }
        if (rvalue) {
            return true;
        }
    }
    return false;
}

void QueryRunner::populate_string_queries(std::shared_ptr<Expression> const& expr) {
    if (expr->has_only_expression_operands()) {
        for (auto const& op : expr->get_op_list()) {
            populate_string_queries(std::static_pointer_cast<Expression>(op));
        }
        return;
    }

    auto filter = std::dynamic_pointer_cast<FilterExpr>(expr);
    if (filter != nullptr
        && !(filter->get_operation() == FilterOperation::EXISTS
             || filter->get_operation() == FilterOperation::NEXISTS))
    {
        if (filter->get_column()->matches_type(LiteralType::ClpStringT)) {
            std::string query_string;
            filter->get_operand()->as_clp_string(query_string, filter->get_operation());

            if (m_string_query_map.count(query_string)) {
                return;
            }

            // search on log type dictionary
            clp::epochtime_t placeholder_timestamp{};
            log_surgeon::lexers::ByteLexer placeholder_lexer;
            m_string_query_map.emplace(
                    query_string,
                    clp::GrepCore::process_raw_query(
                            *m_log_dict,
                            *m_var_dict,
                            query_string,
                            placeholder_timestamp,
                            placeholder_timestamp,
                            m_ignore_case,
                            placeholder_lexer,
                            true
                    )
            );
        }

        if (filter->get_column()->matches_type(LiteralType::VarStringT)) {
            std::string query_string;
            filter->get_operand()->as_var_string(query_string, filter->get_operation());
            if (m_string_var_match_map.count(query_string)) {
                return;
            }

            std::unordered_set<int64_t>& matching_vars = m_string_var_match_map[query_string];
            if (false == ast::has_unescaped_wildcards(query_string)) {
                auto const unescaped_query_string{clp::string_utils::unescape_string(query_string)};
                auto const entries = m_var_dict->get_entry_matching_value(
                        unescaped_query_string,
                        m_ignore_case
                );

                for (auto const& entry : entries) {
                    matching_vars.insert(entry->get_id());
                }
            } else {
                std::unordered_set<VariableDictionaryEntry const*> matching_entries;
                m_var_dict->get_entries_matching_wildcard_string(
                        query_string,
                        m_ignore_case,
                        matching_entries
                );
                for (auto const& entry : matching_entries) {
                    matching_vars.emplace(entry->get_id());
                }
            }
        }
    }
}

void QueryRunner::populate_internal_columns() {
    int32_t metadata_subtree_root_node_id = m_schema_tree->get_metadata_subtree_node_id();
    if (-1 == metadata_subtree_root_node_id) {
        return;
    }

    // This code assumes that the metadata subtree contains no nested structures
    auto& metadata_node = m_schema_tree->get_node(metadata_subtree_root_node_id);
    for (auto child_id : metadata_node.get_children_ids()) {
        m_metadata_columns.insert(child_id);
    }
}

void QueryRunner::populate_searched_wildcard_columns(std::shared_ptr<Expression> const& expr) {
    if (expr->has_only_expression_operands()) {
        for (auto const& op : expr->get_op_list()) {
            populate_searched_wildcard_columns(std::static_pointer_cast<Expression>(op));
        }
    } else if (auto filter = std::dynamic_pointer_cast<FilterExpr>(expr)) {
        auto col = filter->get_column().get();
        if (false == col->is_pure_wildcard()) {
            return;
        }
        m_wildcard_columns.push_back(col);
        literal_type_bitmask_t matching_types{0};
        for (int32_t node : (*m_schemas)[m_schema]) {
            if (Schema::schema_entry_is_unordered_object(node)) {
                continue;
            }
            if (0 != m_metadata_columns.count(node)) {
                continue;
            }
            auto tree_node_type = m_schema_tree->get_node(node).get_type();
            if (col->matches_type(node_to_literal_type(tree_node_type))) {
                auto literal_type = node_to_literal_type(tree_node_type);
                matching_types |= literal_type;
                if (NodeType::ClpString != tree_node_type && NodeType::LogType != tree_node_type
                    && NodeType::VarString != tree_node_type
                    && NodeType::DateString != tree_node_type)
                {
                    m_wildcard_to_searched_basic_columns[col].insert(node);
                }
            }
        }
        col->set_matching_types(matching_types);
    }
}

void QueryRunner::add_wildcard_columns_to_searched_columns() {
    m_wildcard_type_mask = 0;
    for (ColumnDescriptor* wildcard : m_wildcard_columns) {
        m_wildcard_type_mask |= wildcard->get_matching_types();
    }
}

EvaluatedValue QueryRunner::constant_propagate(std::shared_ptr<Expression> const& expr) {
    if (std::dynamic_pointer_cast<OrExpr>(expr)) {
        bool any_unknown = false;
        std::vector<OpList::iterator> to_delete;
        for (auto it = expr->op_begin(); it != expr->op_end(); it++) {
            auto sub_expr = std::static_pointer_cast<Expression>(*it);
            EvaluatedValue ret = constant_propagate(sub_expr);
            if (ret == EvaluatedValue::True) {
                return expr->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
            } else if (ret == EvaluatedValue::False) {
                // no need to add this sub expression to used expression set
                // but mark it for deletion
                to_delete.push_back(it);
            } else /*if (ret == EvaluatedValue::Unknown)*/ {
                any_unknown = true;
            }
        }

        if (any_unknown) {
            // some unknowns -- delete guaranteed false entries, and
            // propagate unknown
            for (OpList::iterator& it : to_delete) {
                expr->get_op_list().erase(it);
            }
            return EvaluatedValue::Unknown;
        } else {
            // no unknowns, and didn't early exit, so before inversion the evaluated
            // value must be False
            return expr->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
        }
    } else if (std::dynamic_pointer_cast<AndExpr>(expr)) {
        bool any_unknown = true;
        std::vector<OpList::iterator> to_delete;
        for (auto it = expr->op_begin(); it != expr->op_end(); it++) {
            auto subExpr = std::static_pointer_cast<Expression>(*it);

            EvaluatedValue ret = constant_propagate(subExpr);

            if (ret == EvaluatedValue::False) {
                return expr->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
            } else if (ret == EvaluatedValue::True) {
                // no need to add this sub expression to used expression set
                // but mark it for deletion
                to_delete.push_back(it);
            } else /*if (ret == EvaluatedValue::Unknown)*/ {
                any_unknown = true;
            }
        }

        if (any_unknown) {
            // some unknowns -- delete guaranteed true entries, and
            // propagate unknown
            for (OpList::iterator& it : to_delete) {
                expr->get_op_list().erase(it);
            }
            return EvaluatedValue::Unknown;
        } else {
            // no unknowns, and didn't early exit, so before inversion the evaluated
            // value must be True
            return expr->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
        }
        return EvaluatedValue::Unknown;
    } else if (auto filter = std::dynamic_pointer_cast<FilterExpr>(expr)) {
        if ((filter->get_operation() == FilterOperation::EXISTS
             || filter->get_operation() == FilterOperation::NEXISTS)
            && (!filter->get_column()->has_unresolved_tokens()
                || filter->get_column()->is_pure_wildcard()
                || !filter->get_column()->matches_exactly(LiteralType::ArrayT)))
        {
            // semantics of previous passes means that EXISTS and NEXISTS are
            // trivially matching
            // FIXME: have an edgecase to handle with NEXISTS on pure wildcard columns
            return EvaluatedValue::True;
        } else if (filter->get_column()->is_pure_wildcard()
                   && filter->get_column()->matches_any(
                           LiteralType::ClpStringT | LiteralType::VarStringT
                   ))
        {
            auto wildcard = filter->get_column().get();
            bool has_var_string = false;
            bool matches_var_string = false;
            bool has_clp_string = false;
            bool matches_clp_string = false;
            constexpr literal_type_bitmask_t other_types
                    = LiteralType::ArrayT | ast::cIntegralTypes | LiteralType::NullT
                      | LiteralType::BooleanT | LiteralType::TimestampT;
            bool has_other = wildcard->matches_any(other_types);
            std::string filter_string;
            bool valid
                    = filter->get_operand()->as_var_string(filter_string, filter->get_operation())
                      || filter->get_operand()->as_clp_string(
                              filter_string,
                              filter->get_operation()
                      );
            if (false == valid) {
                // FIXME: throw
                return EvaluatedValue::False;
            }
            if (filter->get_column()->matches_type(LiteralType::ClpStringT)) {
                auto& query_processing_result = m_string_query_map.at(filter_string);
                if (query_processing_result.has_value()) {
                    m_expr_clp_query[expr.get()] = &(query_processing_result.value());
                    matches_clp_string = true;
                } else {
                    m_expr_clp_query[expr.get()] = nullptr;
                }
                has_clp_string = wildcard->matches_type(LiteralType::ClpStringT);
            }
            if (filter->get_column()->matches_type(LiteralType::VarStringT)) {
                m_expr_var_match_map[expr.get()] = &m_string_var_match_map.at(filter_string);
                has_var_string = wildcard->matches_type(LiteralType::VarStringT);
                matches_var_string = !m_expr_var_match_map.at(expr.get())->empty();
            }

            if (filter->get_operation() == FilterOperation::EQ) {
                if (false == matches_clp_string) {
                    wildcard->remove_matching_type(LiteralType::ClpStringT);
                }
                if (false == matches_var_string) {
                    wildcard->remove_matching_type(LiteralType::VarStringT);
                }

                if (has_other) {
                    return EvaluatedValue::Unknown;
                }

                if (has_clp_string || has_var_string) {
                    if ((!has_clp_string || (has_clp_string && !matches_clp_string))
                        && (!has_var_string || (has_var_string && !matches_var_string)))
                    {
                        return filter->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
                    }
                }
            } else if (filter->get_operation() == FilterOperation::NEQ) {
                if ((has_clp_string && !matches_clp_string)
                    || (has_var_string && !matches_var_string))
                {
                    return filter->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
                } else if (false == has_clp_string && false == has_var_string && !has_other) {
                    return EvaluatedValue::False;
                }
            } else {
                // FIXME: throw
                return EvaluatedValue::False;
            }
            return EvaluatedValue::Unknown;
        } else if (filter->get_column()->matches_type(LiteralType::ClpStringT)) {
            std::string filter_string;
            filter->get_operand()->as_clp_string(filter_string, filter->get_operation());

            // set up string query for this filter
            auto& query_processing_result = m_string_query_map.at(filter_string);
            if (query_processing_result.has_value()) {
                m_expr_clp_query[expr.get()] = &(query_processing_result.value());
                return EvaluatedValue::Unknown;
            } else {
                m_expr_clp_query[expr.get()] = nullptr;
                // If filter can not match then return it's guaranteed value based on
                // whether the filter is inverted and whether the operation was == or !=
                if (filter->get_operation() == FilterOperation::EQ) {
                    return filter->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
                } else if (filter->get_operation() == FilterOperation::NEQ) {
                    return filter->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
                }
                // FIXME: throw
                return EvaluatedValue::False;
            }
        } else if (filter->get_column()->matches_type(LiteralType::VarStringT)) {
            std::string filter_string;
            filter->get_operand()->as_var_string(filter_string, filter->get_operation());

            // set up string query for this filter
            m_expr_var_match_map[expr.get()] = &m_string_var_match_map.at(filter_string);

            // use string queries to potentially propagate known result
            if (m_expr_var_match_map.at(expr.get())->empty()) {
                // If filter can not match then return it's guaranteed value based on
                // whether the filter is inverted and whether the operation was == or !=
                if (filter->get_operation() == FilterOperation::EQ) {
                    return filter->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
                } else if (filter->get_operation() == FilterOperation::NEQ) {
                    return filter->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
                }
                // FIXME: throw
                return EvaluatedValue::False;
            } else {
                return EvaluatedValue::Unknown;
            }
        } else {
            return EvaluatedValue::Unknown;
        }
    }

    return EvaluatedValue::Unknown;
}

bool QueryRunner::evaluate_epoch_date_filter(
        FilterOperation op,
        DateStringColumnReader* reader,
        std::shared_ptr<Literal>& operand
) {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    int64_t op_value;
    if (false == operand->as_int(op_value, op)) {
        return false;
    }

    return evaluate_int_filter_core(op, reader->get_encoded_time(m_cur_message), op_value);
}
}  // namespace clp_s::search
