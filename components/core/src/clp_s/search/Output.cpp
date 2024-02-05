#include "Output.hpp"

#include <regex>
#include <stack>

#include "../FileWriter.hpp"
#include "../ReaderUtils.hpp"
#include "../Utils.hpp"
#include "AndExpr.hpp"
#include "clp_search/EncodedVariableInterpreter.hpp"
#include "clp_search/Grep.hpp"
#include "EvaluateTimestampIndex.hpp"
#include "FilterExpr.hpp"
#include "OrExpr.hpp"
#include "SearchUtils.hpp"

#define eval(op, a, b) (((op) == FilterOperation::EQ) ? ((a) == (b)) : ((a) != (b)))

namespace clp_s::search {
void Output::filter() {
    auto top_level_expr = m_expr;

    for (auto const& archive : ReaderUtils::get_archives(m_archives_dir)) {
        std::vector<int32_t> matched_schemas;
        bool has_array = false;
        bool has_array_search = false;
        for (int32_t schema_id : ReaderUtils::get_schemas(archive)) {
            if (m_match.schema_matched(schema_id)) {
                matched_schemas.push_back(schema_id);
                if (m_match.has_array(schema_id)) {
                    has_array = true;
                }
                if (m_match.has_array_search(schema_id)) {
                    has_array_search = true;
                }
            }
        }

        // Skip decompressing segment if it contains no
        // relevant schemas
        if (matched_schemas.empty()) {
            continue;
        }

        // Skip decompressing sub-archive if it won't match based on the timestamp
        // range index
        EvaluateTimestampIndex timestamp_index(ReaderUtils::read_local_timestamp_dictionary(archive)
        );
        if (timestamp_index.run(top_level_expr) == EvaluatedValue::False) {
            continue;
        }

        m_var_dict = ReaderUtils::get_variable_dictionary_reader(archive);
        m_log_dict = ReaderUtils::get_log_type_dictionary_reader(archive);
        //        array_dict_ = GetArrayDictionaryReader(archive);
        m_var_dict->read_new_entries();
        m_log_dict->read_new_entries();

        if (has_array) {
            m_array_dict = ReaderUtils::get_array_dictionary_reader(archive);
            if (has_array_search) {
                m_array_dict->read_new_entries();
            } else {
                m_array_dict->read_new_entries(true);
            }
        }

        m_string_query_map.clear();
        m_string_var_match_map.clear();
        populate_string_queries(top_level_expr);

        std::string message;
        for (int32_t schema_id : matched_schemas) {
            m_expr_clp_query.clear();
            m_expr_var_match_map.clear();
            m_expr = m_match.get_query_for_schema(schema_id)->copy();
            m_wildcard_to_searched_columns.clear();
            m_wildcard_to_searched_clpstrings.clear();
            m_wildcard_to_searched_varstrings.clear();
            m_wildcard_to_searched_datestrings.clear();
            m_wildcard_to_searched_floatdatestrings.clear();
            m_schema = schema_id;

            populate_searched_wildcard_columns(m_expr);

            m_expression_value = constant_propagate(m_expr, schema_id);

            if (m_expression_value == EvaluatedValue::False) {
                continue;
            }

            add_wildcard_columns_to_searched_columns();

            SchemaReader reader(m_schema_tree, schema_id);
            reader.open(archive + "/encoded_messages/" + std::to_string(schema_id));
            ReaderUtils::append_reader_columns(
                    &reader,
                    (*m_schemas)[schema_id],
                    m_schema_tree,
                    m_var_dict,
                    m_log_dict,
                    m_array_dict,
                    m_timestamp_dict,
                    m_output_handler->should_output_timestamp()
            );
            reader.load();

            reader.initialize_filter(this);

            if (m_output_handler->should_output_timestamp()) {
                epochtime_t timestamp;
                while (reader.get_next_message_with_timestamp(message, timestamp, this)) {
                    m_output_handler->write(message, timestamp);
                }
            } else {
                while (reader.get_next_message(message, this)) {
                    m_output_handler->write(message);
                }
            }

            reader.close();
        }

        m_output_handler->flush();

        m_var_dict->close();
        m_log_dict->close();

        if (has_array) {
            m_array_dict->close();
        }
    }
}

void Output::init(
        SchemaReader* reader,
        int32_t schema_id,
        std::unordered_map<int32_t, BaseColumnReader*>& columns
) {
    m_reader = reader;
    m_schema = schema_id;

    m_searched_columns.clear();
    m_other_columns.clear();

    for (auto& column : columns) {
        ClpStringColumnReader* clp_reader = dynamic_cast<ClpStringColumnReader*>(column.second);
        VariableStringColumnReader* var_reader
                = dynamic_cast<VariableStringColumnReader*>(column.second);
        if (m_match.schema_searches_against_column(schema_id, column.first)) {
            if (clp_reader != nullptr && clp_reader->get_type() == NodeType::CLPSTRING) {
                m_clp_string_readers[column.first] = clp_reader;
                m_other_columns.push_back(column.second);
            } else if (var_reader != nullptr && var_reader->get_type() == NodeType::VARSTRING) {
                m_var_string_readers[column.first] = var_reader;
                m_other_columns.push_back(column.second);
            } else if (auto date_column_reader = dynamic_cast<DateStringColumnReader*>(column.second))
            {
                m_datestring_readers[column.first] = date_column_reader;
                m_other_columns.push_back(column.second);
            } else if (auto float_date_column_reader = dynamic_cast<FloatDateStringColumnReader*>(column.second))
            {
                m_floatdatestring_readers[column.first] = float_date_column_reader;
                m_other_columns.push_back(column.second);
            } else {
                m_searched_columns.push_back(column.second);
            }
        } else {
            m_other_columns.push_back(column.second);
        }
    }
}

bool Output::filter(
        uint64_t cur_message,
        std::map<int32_t, std::variant<int64_t, double, std::string, uint8_t>>& extracted_values
) {
    m_cur_message = cur_message;
    m_cached_string_columns.clear();
    for (auto* column : m_searched_columns) {
        extracted_values[column->get_id()] = column->extract_value(cur_message);
    }

    // filter
    if (false == evaluate(m_expr.get(), m_schema, extracted_values)) {
        return false;
    }

    for (auto* column : m_other_columns) {
        if (m_cached_string_columns.find(column->get_id()) == m_cached_string_columns.end()) {
            extracted_values[column->get_id()] = column->extract_value(cur_message);
        }
    }

    return true;
}

enum CurExpr {
    AND,
    OR,
    FILTER
};

bool Output::evaluate(
        Expression* expr,
        int32_t schema,
        std::map<int32_t, std::variant<int64_t, double, std::string, uint8_t>>& extracted_values
) {
    if (m_expression_value == EvaluatedValue::True) {
        return true;
    }

    std::stack<CurExpr, std::vector<CurExpr>> parent_type;
    std::stack<OpList::iterator, std::vector<OpList::iterator>> parent_it;

    Expression* cur = expr;
    CurExpr cur_type = CurExpr::FILTER;
    bool ret = false;

    if (dynamic_cast<AndExpr*>(cur)) {
        cur_type = CurExpr::AND;
        parent_type.push(CurExpr::AND);
        parent_it.push(cur->op_begin());
        ret = true;
    } else if (dynamic_cast<OrExpr*>(cur)) {
        cur_type = CurExpr::OR;
        parent_type.push(CurExpr::OR);
        parent_it.push(cur->op_begin());
        ret = false;
    }

    do {
        switch (cur_type) {
            case CurExpr::AND:
                if (false == ret || parent_it.top() == cur->op_end()) {
                    parent_type.pop();
                    parent_it.pop();
                    break;
                } else {
                    cur = static_cast<Expression*>((parent_it.top()++)->get());
                    if (dynamic_cast<FilterExpr*>(cur)) {
                        cur_type = CurExpr::FILTER;
                    } else {
                        // must be an OR-expr because AST would have been simplified
                        // to eliminate nested AND
                        cur_type = CurExpr::OR;
                        parent_type.push(CurExpr::OR);
                        parent_it.push(cur->op_begin());
                        ret = false;
                    }
                    continue;
                }
            case CurExpr::FILTER:
                if (static_cast<FilterExpr*>(cur)->get_column()->is_pure_wildcard()) {
                    ret = evaluate_wildcard_filter(
                            static_cast<FilterExpr*>(cur),
                            schema,
                            extracted_values
                    );
                } else {
                    ret = evaluate_filter(static_cast<FilterExpr*>(cur), schema, extracted_values);
                }
                break;
            case CurExpr::OR:
                if (ret || parent_it.top() == cur->op_end()) {
                    parent_type.pop();
                    parent_it.pop();
                    break;
                } else {
                    cur = static_cast<Expression*>((parent_it.top()++)->get());
                    if (dynamic_cast<FilterExpr*>(cur)) {
                        cur_type = CurExpr::FILTER;
                    } else {
                        // must be an AND-expr because AST would have been simplified
                        // to eliminate nested OR
                        cur_type = CurExpr::AND;
                        parent_type.push(CurExpr::AND);
                        parent_it.push(cur->op_begin());
                        ret = true;
                    }
                    continue;
                }
        }

        ret = cur->is_inverted() ? !ret : ret;
        if (false == parent_type.empty()) {
            cur_type = parent_type.top();
        }
        cur = cur->get_parent();
    } while (cur != nullptr);

    return ret;
}

bool Output::evaluate_wildcard_filter(
        FilterExpr* expr,
        int32_t schema,
        std::map<int32_t, std::variant<int64_t, double, std::string, uint8_t>>& extracted_values
) {
    auto literal = expr->get_operand();
    auto* column = expr->get_column().get();
    Query* q = m_expr_clp_query[expr];
    std::unordered_set<int64_t>* matching_vars = m_expr_var_match_map[expr];
    auto op = expr->get_operation();
    for (int32_t column_id : m_wildcard_to_searched_clpstrings[column]) {
        if (evaluate_clp_string_filter(op, q, column_id, literal, extracted_values)) {
            return true;
        }
    }

    for (int32_t column_id : m_wildcard_to_searched_varstrings[column]) {
        if (evaluate_var_string_filter(op, m_var_string_readers[column_id], matching_vars, literal))
        {
            return true;
        }
    }

    for (int32_t column_id : m_wildcard_to_searched_datestrings[column]) {
        if (evaluate_epoch_date_filter(op, m_datestring_readers[column_id], literal)) {
            return true;
        }
    }

    for (int32_t column_id : m_wildcard_to_searched_floatdatestrings[column]) {
        if (evaluate_float_date_filter(op, m_floatdatestring_readers[column_id], literal)) {
            return true;
        }
    }

    m_maybe_number = expr->get_column()->matches_type(LiteralType::FloatT);
    for (int32_t column_id : m_wildcard_to_searched_columns[column]) {
        bool ret = false;
        switch (node_to_literal_type(m_schema_tree->get_node(column_id)->get_type())) {
            case LiteralType::IntegerT:
                ret = evaluate_int_filter(
                        op,
                        std::get<int64_t>(extracted_values[column_id]),
                        literal
                );
                break;
            case LiteralType::FloatT:
                ret = evaluate_float_filter(
                        op,
                        std::get<double>(extracted_values[column_id]),
                        literal
                );
                break;
            case LiteralType::BooleanT:
                ret = evaluate_bool_filter(
                        op,
                        std::get<uint8_t>(extracted_values[column_id]),
                        literal
                );
                break;
            case LiteralType::ArrayT:
                ret = evaluate_wildcard_array_filter(
                        op,
                        std::get<std::string>(extracted_values[column_id]),
                        literal
                );
                break;
        }

        if (ret) {
            return true;
        }
    }

    return false;
}

bool Output::evaluate_filter(
        FilterExpr* expr,
        int32_t schema,
        std::map<int32_t, std::variant<int64_t, double, std::string, uint8_t>>& extracted_values
) {
    auto column = expr->get_column().get();
    int32_t column_id = column->get_column_id();
    auto literal = expr->get_operand();
    Query* q = nullptr;
    ClpStringColumnReader* clp_reader = nullptr;
    VariableStringColumnReader* var_reader = nullptr;
    std::unordered_set<int64_t>* matching_vars = nullptr;
    switch (column->get_literal_type()) {
        case LiteralType::IntegerT:
            return evaluate_int_filter(
                    expr->get_operation(),
                    std::get<int64_t>(extracted_values[column_id]),
                    literal
            );
        case LiteralType::FloatT:
            return evaluate_float_filter(
                    expr->get_operation(),
                    std::get<double>(extracted_values[column_id]),
                    literal
            );
        case LiteralType::ClpStringT:
            q = m_expr_clp_query[expr];
            clp_reader = m_clp_string_readers[column_id];
            return evaluate_clp_string_filter(
                    expr->get_operation(),
                    q,
                    column_id,
                    literal,
                    extracted_values
            );
        case LiteralType::VarStringT:
            var_reader = m_var_string_readers[column_id];
            matching_vars = m_expr_var_match_map.at(expr);
            return evaluate_var_string_filter(
                    expr->get_operation(),
                    var_reader,
                    matching_vars,
                    literal
            );
        case LiteralType::BooleanT:
            return evaluate_bool_filter(
                    expr->get_operation(),
                    std::get<uint8_t>(extracted_values[column_id]),
                    literal
            );
        case LiteralType::ArrayT:
            return evaluate_array_filter(
                    expr->get_operation(),
                    column->get_unresolved_tokens(),
                    std::get<std::string>(extracted_values[column_id]),
                    literal
            );
        case LiteralType::EpochDateT:
            return evaluate_epoch_date_filter(
                    expr->get_operation(),
                    m_datestring_readers[column_id],
                    literal
            );
        case LiteralType::FloatDateT:
            return evaluate_float_date_filter(
                    expr->get_operation(),
                    m_floatdatestring_readers[column_id],
                    literal
            );
            // case LiteralType::NullT:
            //  null checks are always turned into existence operators --
            //  no need to evaluate here
        default:
            return false;
    }
}

bool Output::evaluate_int_filter(
        FilterOperation op,
        int64_t value,
        std::shared_ptr<Literal> const& operand
) {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    int64_t op_value;
    if (false == operand->as_int(op_value, op)) {
        return false;
    }

    switch (op) {
        case FilterOperation::EQ:
            return value == op_value;
        case FilterOperation::NEQ:
            return value != op_value;
        case FilterOperation::LT:
            return value < op_value;
        case FilterOperation::GT:
            return value > op_value;
        case FilterOperation::LTE:
            return value <= op_value;
        case FilterOperation::GTE:
            return value >= op_value;
        default:
            return false;
    }
}

bool Output::evaluate_float_filter(
        FilterOperation op,
        double value,
        std::shared_ptr<Literal> const& operand
) {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    double op_value;
    if (false == operand->as_float(op_value, op)) {
        return false;
    }

    switch (op) {
        case FilterOperation::EQ:
            return value == op_value;
        case FilterOperation::NEQ:
            return value != op_value;
        case FilterOperation::LT:
            return value < op_value;
        case FilterOperation::GT:
            return value > op_value;
        case FilterOperation::LTE:
            return value <= op_value;
        case FilterOperation::GTE:
            return value >= op_value;
        default:
            return false;
    }
}

bool Output::evaluate_clp_string_filter(
        FilterOperation op,
        Query* q,
        int32_t column_id,
        std::shared_ptr<Literal> const& operand,
        std::map<int32_t, std::variant<int64_t, double, std::string, uint8_t>>& extracted_values
) {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    if (op != FilterOperation::EQ && op != FilterOperation::NEQ) {
        return false;
    }

    auto* reader = m_clp_string_readers[column_id];
    int64_t id = reader->get_encoded_id(m_cur_message);
    bool matched = false;

    if (q->search_string_matches_all()) {
        return op == FilterOperation::EQ;
    }

    auto vars = reader->get_encoded_vars(m_cur_message);
    for (auto const& subquery : q->get_sub_queries()) {
        if (subquery.matches_logtype(id) && subquery.matches_vars(vars)) {
            matched = true;

            if (subquery.wildcard_match_required()) {
                std::string decompressed_message
                        = std::get<std::string>(reader->extract_value(m_cur_message));
                matched = StringUtils::wildcard_match_unsafe(
                        decompressed_message,
                        q->get_search_string(),
                        !q->get_ignore_case()
                );
                matched = (op == FilterOperation::EQ) == matched;
                if (matched) {
                    extracted_values[column_id] = std::move(decompressed_message);
                    m_cached_string_columns.insert(column_id);
                }
                return matched;
            }

            break;
        }
    }

    return (op == FilterOperation::EQ) == matched;
}

bool Output::evaluate_var_string_filter(
        FilterOperation op,
        VariableStringColumnReader* reader,
        std::unordered_set<int64_t>* matching_vars,
        std::shared_ptr<Literal> const& operand
) const {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    int64_t id = reader->get_variable_id(m_cur_message);
    bool matched = matching_vars->count(id);
    switch (op) {
        case FilterOperation::EQ:
            return matched;
        case FilterOperation::NEQ:
            return !matched;
        default:
            return false;
    }
}

bool Output::evaluate_array_filter(
        FilterOperation op,
        DescriptorList const& unresolved_tokens,
        std::string& value,
        std::shared_ptr<Literal> const& operand
) {
    if (value.capacity() < (value.size() + simdjson::SIMDJSON_PADDING)) {
        value.reserve(value.size() + simdjson::SIMDJSON_PADDING);
    }
    auto obj = m_array_parser.iterate(value);
    ondemand::array array = obj.get_array();

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

bool Output::evaluate_array_filter_value(
        ondemand::value& item,
        FilterOperation op,
        DescriptorList const& unresolved_tokens,
        size_t cur_idx,
        std::shared_ptr<Literal> const& operand
) const {
    bool match = false;
    switch (item.type()) {
        case ondemand::json_type::object: {
            ondemand::object nested_object = item.get_object();
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
        case ondemand::json_type::array: {
            ondemand::array nested_array = item.get_array();
            if (evaluate_array_filter_array(nested_array, op, unresolved_tokens, cur_idx, operand))
            {
                match = true;
            }
        } break;
        case ondemand::json_type::string: {
            if (true == m_maybe_string && unresolved_tokens.size() == cur_idx
                && wildcard_match(item.get_string().value(), m_array_search_string))
            {
                match = op == FilterOperation::EQ;
            }
        } break;
        case ondemand::json_type::number: {
            if (false == m_maybe_number || unresolved_tokens.size() != cur_idx) {
                break;
            }
            ondemand::number number = item.get_number();
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
        case ondemand::json_type::boolean: {
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
        case ondemand::json_type::null: {
            if (op != FilterOperation::EXISTS && op != FilterOperation::NEXISTS
                && operand->as_null(op))
            {
                match = op == FilterOperation::EQ;
            }
        } break;
    }
    return match;
}

bool Output::evaluate_array_filter_array(
        ondemand::array& array,
        FilterOperation op,
        DescriptorList const& unresolved_tokens,
        size_t cur_idx,
        std::shared_ptr<Literal> const& operand
) const {
    for (ondemand::value item : array) {
        if (evaluate_array_filter_value(item, op, unresolved_tokens, cur_idx, operand)) {
            return true;
        }
    }
    return false;
}

bool Output::evaluate_array_filter_object(
        ondemand::object& object,
        FilterOperation op,
        DescriptorList const& unresolved_tokens,
        size_t cur_idx,
        std::shared_ptr<Literal> const& operand
) const {
    if (cur_idx >= unresolved_tokens.size()) {
        return false;
    }

    for (auto field : object) {
        // Note: field.key() yields the escaped JSON key, so the descriptor tokens passed to search
        // must likewise be escaped.
        if (field.key() != unresolved_tokens[cur_idx].get_token()) {
            continue;
        }

        cur_idx += 1;
        if (cur_idx == unresolved_tokens.size()
            && (op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS))
        {
            return op == FilterOperation::EXISTS;
        }

        ondemand::value item = field.value();
        return evaluate_array_filter_value(item, op, unresolved_tokens, cur_idx, operand);
    }
    return false;
}

bool Output::evaluate_wildcard_array_filter(
        FilterOperation op,
        std::string& value,
        std::shared_ptr<Literal> const& operand
) {
    if (value.capacity() < (value.size() + simdjson::SIMDJSON_PADDING)) {
        value.reserve(value.size() + simdjson::SIMDJSON_PADDING);
    }
    auto obj = m_array_parser.iterate(value);
    ondemand::array array = obj.get_array();

    // pre-evaluate whether we can match strings or numbers to eliminate
    // duplicate effort on every item
    m_maybe_string = operand->as_var_string(m_array_search_string, op)
                     || operand->as_clp_string(m_array_search_string, op);

    return evaluate_wildcard_array_filter(array, op, operand);
}

bool Output::evaluate_wildcard_array_filter(
        ondemand::array& array,
        FilterOperation op,
        std::shared_ptr<Literal> const& operand
) const {
    bool match = false;
    for (auto item : array) {
        switch (item.type()) {
            case ondemand::json_type::object: {
                ondemand::object nested_object = item.get_object();
                if (evaluate_wildcard_array_filter(nested_object, op, operand)) {
                    match = true;
                }
            } break;
            case ondemand::json_type::array: {
                ondemand::array nested_array = item.get_array();
                if (evaluate_wildcard_array_filter(nested_array, op, operand)) {
                    match = true;
                }
            } break;
            case ondemand::json_type::string: {
                if (false == m_maybe_string) {
                    break;
                }
                if (wildcard_match(item.get_string().value(), m_array_search_string)) {
                    match |= op == FilterOperation::EQ;
                }
                break;
            } break;
            case ondemand::json_type::number: {
                if (false == m_maybe_number) {
                    break;
                }
                ondemand::number number = item.get_number();
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
            case ondemand::json_type::boolean: {
                bool tmp;
                if (operand->as_bool(tmp, op) && eval(op, item.get_bool(), tmp)) {
                    match = true;
                }
            } break;
            case ondemand::json_type::null:
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

bool Output::evaluate_wildcard_array_filter(
        ondemand::object& object,
        FilterOperation op,
        std::shared_ptr<Literal> const& operand
) const {
    bool match = false;
    for (auto field : object) {
        ondemand::value item = field.value();
        switch (item.type()) {
            case ondemand::json_type::object: {
                ondemand::object nested_object = item.get_object();
                if (evaluate_wildcard_array_filter(nested_object, op, operand)) {
                    match = true;
                }
            } break;
            case ondemand::json_type::array: {
                ondemand::array nested_array = item.get_array();
                if (evaluate_wildcard_array_filter(nested_array, op, operand)) {
                    match = true;
                }
            } break;
            case ondemand::json_type::string: {
                if (false == m_maybe_string) {
                    break;
                }
                if (wildcard_match(item.get_string().value(), m_array_search_string)) {
                    match |= op == FilterOperation::EQ;
                }
                break;
            } break;
            case ondemand::json_type::number: {
                if (false == m_maybe_number) {
                    break;
                }
                ondemand::number number = item.get_number();
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
            case ondemand::json_type::boolean: {
                bool tmp;
                if (operand->as_bool(tmp, op) && eval(op, item.get_bool(), tmp)) {
                    match = true;
                }
            } break;
            case ondemand::json_type::null:
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

bool Output::evaluate_bool_filter(
        FilterOperation op,
        bool value,
        std::shared_ptr<Literal> const& operand
) {
    if (FilterOperation::EXISTS == op || FilterOperation::NEXISTS == op) {
        return true;
    }

    bool op_value;
    if (false == operand->as_bool(op_value, op)) {
        return false;
    }

    switch (op) {
        case FilterOperation::EQ:
            return value == op_value;
        case FilterOperation::NEQ:
            return value != op_value;
        default:
            return false;
    }
}

void Output::populate_string_queries(std::shared_ptr<Expression> const& expr) {
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
            Query& q = m_string_query_map[query_string];
            if (query_string.find("*") != std::string::npos
                || filter->get_column()->matches_type(LiteralType::VarStringT))
            {
                // if it matches VarStringT then it contains no space, so we
                // don't't add more wildcards. Likewise if it already contains some wildcards
                // we do not add more
                Grep::process_raw_query(m_log_dict, m_var_dict, query_string, false, q, false);
            } else {
                Grep::process_raw_query(m_log_dict, m_var_dict, query_string, false, q);
            }
        }
        SubQuery sub_query;
        if (filter->get_column()->matches_type(LiteralType::VarStringT)) {
            std::string query_string;
            filter->get_operand()->as_var_string(query_string, filter->get_operation());
            if (m_string_var_match_map.count(query_string)) {
                return;
            }

            std::unordered_set<int64_t>& matching_vars = m_string_var_match_map[query_string];
            if (query_string.find('*') == std::string::npos) {
                auto entry = m_var_dict->get_entry_matching_value(query_string, false);

                if (entry != nullptr) {
                    matching_vars.insert(entry->get_id());
                }
            } else if (EncodedVariableInterpreter::
                               wildcard_search_dictionary_and_get_encoded_matches(
                                       query_string,
                                       *m_var_dict,
                                       false,
                                       sub_query
                               ))
            {
                for (auto& var : sub_query.get_vars()) {
                    if (var.is_precise_var()) {
                        auto entry = var.get_var_dict_entry();
                        if (entry != nullptr) {
                            matching_vars.insert(entry->get_id());
                        }
                    } else {
                        for (auto entry : var.get_possible_var_dict_entries()) {
                            matching_vars.insert(entry->get_id());
                        }
                    }
                }
            }
        }
    }
}

void Output::populate_searched_wildcard_columns(std::shared_ptr<Expression> const& expr) {
    if (expr->has_only_expression_operands()) {
        for (auto const& op : expr->get_op_list()) {
            populate_searched_wildcard_columns(std::static_pointer_cast<Expression>(op));
        }
    } else if (auto filter = std::dynamic_pointer_cast<FilterExpr>(expr)) {
        auto col = filter->get_column().get();
        if (false == col->is_pure_wildcard()) {
            return;
        }
        for (int32_t node : (*m_schemas)[m_schema]) {
            auto tree_node_type = m_schema_tree->get_node(node)->get_type();
            if (col->matches_type(node_to_literal_type(tree_node_type))) {
                if (tree_node_type == NodeType::CLPSTRING) {
                    m_wildcard_to_searched_clpstrings[col].push_back(node);
                } else if (tree_node_type == NodeType::VARSTRING) {
                    m_wildcard_to_searched_varstrings[col].push_back(node);
                } else if (tree_node_type == NodeType::DATESTRING) {
                    m_wildcard_to_searched_datestrings[col].push_back(node);
                } else if (tree_node_type == NodeType::FLOATDATESTRING) {
                    m_wildcard_to_searched_floatdatestrings[col].push_back(node);
                } else {
                    // Arrays and basic types
                    m_wildcard_to_searched_columns[col].push_back(node);
                }
            }
        }
    }
}

void Output::add_wildcard_columns_to_searched_columns() {
    for (auto& e : m_wildcard_to_searched_clpstrings) {
        for (int32_t node : e.second) {
            m_match.add_searched_column_to_schema(m_schema, node);
        }
    }

    for (auto& e : m_wildcard_to_searched_varstrings) {
        for (int32_t node : e.second) {
            m_match.add_searched_column_to_schema(m_schema, node);
        }
    }

    for (auto& e : m_wildcard_to_searched_datestrings) {
        for (int32_t node : e.second) {
            m_match.add_searched_column_to_schema(m_schema, node);
        }
    }

    for (auto& e : m_wildcard_to_searched_floatdatestrings) {
        for (int32_t node : e.second) {
            m_match.add_searched_column_to_schema(m_schema, node);
        }
    }

    for (auto& e : m_wildcard_to_searched_columns) {
        for (int32_t node : e.second) {
            m_match.add_searched_column_to_schema(m_schema, node);
        }
    }
}

EvaluatedValue
Output::constant_propagate(std::shared_ptr<Expression> const& expr, int32_t schema_id) {
    if (std::dynamic_pointer_cast<OrExpr>(expr)) {
        bool any_unknown = false;
        std::vector<OpList::iterator> to_delete;
        for (auto it = expr->op_begin(); it != expr->op_end(); it++) {
            auto sub_expr = std::static_pointer_cast<Expression>(*it);
            EvaluatedValue ret = constant_propagate(sub_expr, schema_id);
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

            EvaluatedValue ret = constant_propagate(subExpr, schema_id);

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
        } else if (filter->get_column()->is_pure_wildcard() && filter->get_column()->matches_any(LiteralType::ClpStringT | LiteralType::VarStringT))
        {
            auto wildcard = filter->get_column().get();
            bool has_var_string = false;
            bool matches_var_string = false;
            bool has_clp_string = false;
            bool matches_clp_string = false;
            bool has_other = !m_wildcard_to_searched_columns[wildcard].empty()
                             || !m_wildcard_to_searched_datestrings[wildcard].empty()
                             || !m_wildcard_to_searched_floatdatestrings[wildcard].empty();
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
                m_expr_clp_query[expr.get()] = &m_string_query_map.at(filter_string);
                has_clp_string = !m_wildcard_to_searched_clpstrings[wildcard].empty();
                matches_clp_string
                        = !m_expr_clp_query.at(expr.get())->get_sub_queries().empty()
                          || m_expr_clp_query.at(expr.get())->search_string_matches_all();
            }
            if (filter->get_column()->matches_type(LiteralType::VarStringT)) {
                m_expr_var_match_map[expr.get()] = &m_string_var_match_map.at(filter_string);
                has_var_string = !m_wildcard_to_searched_varstrings[wildcard].empty();
                matches_var_string = !m_expr_var_match_map.at(expr.get())->empty();
            }

            if (filter->get_operation() == FilterOperation::EQ) {
                if (false == matches_clp_string) {
                    m_wildcard_to_searched_clpstrings[wildcard].clear();
                }
                if (false == matches_var_string) {
                    m_wildcard_to_searched_varstrings[wildcard].clear();
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
                if (has_clp_string && !matches_clp_string || has_var_string && !matches_var_string)
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
            m_expr_clp_query[expr.get()] = &m_string_query_map.at(filter_string);

            // use string queries to potentially propagate known result
            if (m_expr_clp_query.at(expr.get())->get_sub_queries().empty()
                && !m_expr_clp_query.at(expr.get())->search_string_matches_all())
            {
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

bool Output::evaluate_epoch_date_filter(
        FilterOperation op,
        DateStringColumnReader* reader,
        std::shared_ptr<Literal>& operand
) {
    return evaluate_int_filter(op, reader->get_encoded_time(m_cur_message), operand);
}

bool Output::evaluate_float_date_filter(
        FilterOperation op,
        FloatDateStringColumnReader* reader,
        std::shared_ptr<Literal>& operand
) {
    return evaluate_float_filter(op, reader->get_encoded_time(m_cur_message), operand);
}
}  // namespace clp_s::search
