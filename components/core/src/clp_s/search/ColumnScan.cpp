#include "ColumnScan.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <string_utils/string_utils.hpp>

#include <clp/Query.hpp>
#include <clp_s/ColumnReader.hpp>

#include "ast/AndExpr.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/FilterOperation.hpp"
#include "ast/Literal.hpp"
#include "ast/OrExpr.hpp"

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::LiteralType;
using clp_s::search::ast::OrExpr;

namespace clp_s::search {
namespace {
/**
 * Compares a value against an operand using the given filter operation.
 * @param operation Comparison operation to apply.
 * @param value Value read from a column.
 * @param operand Operand from the filter expression.
 * @return The result of the comparison.
 */
template <typename T>
[[nodiscard]] auto compare(FilterOperation operation, T value, T operand) -> bool;

/**
 * Checks whether the given filter operation is equality-based.
 * @param operation Operation to check.
 * @return true for equality and inequality operations, false otherwise.
 */
[[nodiscard]] auto is_equality_operation(FilterOperation operation) -> bool;

/**
 * Inverts a bitmap in place.
 * @param bitmap Bitmap to invert.
 */
auto invert(ColumnScan::Bitmap& bitmap) -> void;

/**
 * Builds a bitmap for a filter over a basic typed column.
 * @param num_messages Number of messages represented by the bitmap.
 * @param reader_map Column readers keyed by column ID.
 * @param column_id ID of the column to scan.
 * @param operation Filter operation to apply.
 * @param operand Operand from the filter expression.
 * @return A bitmap indexed by message number, with nonzero entries for matching messages.
 */
template <typename T>
[[nodiscard]] auto build_basic_filter(
        uint64_t num_messages,
        ColumnScan::BasicReaderMap const& reader_map,
        int32_t column_id,
        FilterOperation operation,
        T operand
) -> ColumnScan::Bitmap;

/**
 * Checks whether a CLP string value matches a query.
 * @param reader Column reader containing the value to check.
 * @param query Query to match against.
 * @param message_index Message index to check.
 * @return Whether the message index's string value matches the query.
 */
[[nodiscard]] auto
clp_string_matches(ClpStringColumnReader* reader, clp::Query const& query, uint64_t message_index)
        -> bool;

/**
 * Builds a bitmap for a filter over a CLP string column.
 * @param num_messages Number of messages represented by the bitmap.
 * @param reader_map Column readers keyed by column ID.
 * @param column_id ID of the column to scan.
 * @param operation Equality operation to apply.
 * @param query Query to match against.
 * @return A bitmap indexed by message number, with nonzero entries for matching messages.
 */
[[nodiscard]] auto build_clp_string_filter(
        uint64_t num_messages,
        ColumnScan::ClpStringReaderMap const& reader_map,
        int32_t column_id,
        FilterOperation operation,
        clp::Query* query
) -> ColumnScan::Bitmap;

/**
 * Builds a bitmap for a filter over a variable string column.
 * @param num_messages Number of messages represented by the bitmap.
 * @param reader_map Column readers keyed by column ID.
 * @param column_id ID of the column to scan.
 * @param operation Equality operation to apply.
 * @param matching_vars Set of variable IDs that match the filter.
 * @return A bitmap indexed by message number, with nonzero entries for matching messages.
 */
[[nodiscard]] auto build_var_string_filter(
        uint64_t num_messages,
        ColumnScan::VarStringReaderMap const& reader_map,
        int32_t column_id,
        FilterOperation operation,
        std::unordered_set<int64_t> const& matching_vars
) -> ColumnScan::Bitmap;

template <typename T>
[[nodiscard]] auto compare(FilterOperation operation, T value, T operand) -> bool {
    switch (operation) {
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
        case FilterOperation::EXISTS:
        case FilterOperation::NEXISTS:
            return true;
    }
    return false;
}

[[nodiscard]] auto is_equality_operation(FilterOperation operation) -> bool {
    return FilterOperation::EQ == operation || FilterOperation::NEQ == operation;
}

auto invert(ColumnScan::Bitmap& bitmap) -> void {
    for (auto& value : bitmap) {
        value ^= 1;
    }
}

template <typename T>
[[nodiscard]] auto build_basic_filter(
        uint64_t num_messages,
        ColumnScan::BasicReaderMap const& reader_map,
        int32_t column_id,
        FilterOperation operation,
        T operand
) -> ColumnScan::Bitmap {
    ColumnScan::Bitmap bitmap(num_messages, 0);
    auto const readers = reader_map.find(column_id);
    if (reader_map.end() == readers) {
        return bitmap;
    }
    for (auto* reader : readers->second) {
        for (uint64_t message_index{0}; message_index < num_messages; ++message_index) {
            auto const value = std::get<T>(reader->extract_value(message_index));
            bitmap[message_index] |= compare(operation, value, operand) ? 1 : 0;
        }
    }
    return bitmap;
}

[[nodiscard]] auto
clp_string_matches(ClpStringColumnReader* reader, clp::Query const& query, uint64_t message_index)
        -> bool {
    auto const value = std::get<std::string>(reader->extract_value(message_index));
    auto const matches_wildcard = [&query, &value]() -> bool {
        return clp::string_utils::wildcard_match_unsafe(
                value,
                query.get_search_string(),
                false == query.get_ignore_case()
        );
    };

    if (false == query.contains_sub_queries()) {
        return matches_wildcard();
    }

    auto const encoded_id = reader->get_encoded_id(message_index);
    auto const encoded_vars = reader->get_encoded_vars(message_index);
    return std::ranges::any_of(query.get_sub_queries(), [&](auto const& subquery) {
        if (false == subquery.matches_logtype(encoded_id)
            || false == subquery.matches_vars(encoded_vars))
        {
            return false;
        }
        if (false == subquery.wildcard_match_required()) {
            return true;
        }
        return matches_wildcard();
    });
}

[[nodiscard]] auto build_clp_string_filter(
        uint64_t num_messages,
        ColumnScan::ClpStringReaderMap const& reader_map,
        int32_t column_id,
        FilterOperation operation,
        clp::Query* query
) -> ColumnScan::Bitmap {
    ColumnScan::Bitmap bitmap(num_messages, 0);
    if (nullptr == query) {
        std::fill(bitmap.begin(), bitmap.end(), FilterOperation::NEQ == operation ? 1 : 0);
        return bitmap;
    }
    if (query->search_string_matches_all()) {
        std::fill(bitmap.begin(), bitmap.end(), FilterOperation::EQ == operation ? 1 : 0);
        return bitmap;
    }
    auto const readers = reader_map.find(column_id);
    if (reader_map.end() == readers) {
        return bitmap;
    }
    for (auto* reader : readers->second) {
        for (uint64_t message_index{0}; message_index < num_messages; ++message_index) {
            auto const matched = clp_string_matches(reader, *query, message_index);
            bitmap[message_index] |= ((FilterOperation::EQ == operation) == matched) ? 1 : 0;
        }
    }
    return bitmap;
}

[[nodiscard]] auto build_var_string_filter(
        uint64_t num_messages,
        ColumnScan::VarStringReaderMap const& reader_map,
        int32_t column_id,
        FilterOperation operation,
        std::unordered_set<int64_t> const& matching_vars
) -> ColumnScan::Bitmap {
    ColumnScan::Bitmap bitmap(num_messages, 0);
    auto const readers = reader_map.find(column_id);
    if (reader_map.end() == readers) {
        return bitmap;
    }
    for (auto* reader : readers->second) {
        for (uint64_t message_index{0}; message_index < num_messages; ++message_index) {
            auto const matched = matching_vars.contains(
                    static_cast<int64_t>(reader->get_variable_id(message_index))
            );
            bitmap[message_index] |= ((FilterOperation::EQ == operation) == matched) ? 1 : 0;
        }
    }
    return bitmap;
}
}  // namespace

auto ColumnScan::try_create(
        std::shared_ptr<ast::Expression> const& expression,
        BasicReaderMap const& basic_readers,
        ClpStringReaderMap const& clp_string_readers,
        VarStringReaderMap const& var_string_readers,
        ClpQueryMap const& clp_queries,
        VarMatchMap const& var_matches,
        uint64_t num_messages
) -> std::optional<ColumnScan> {
    if (false == can_build_node(expression.get(), clp_queries, var_matches)) {
        return std::nullopt;
    }

    return ColumnScan{
            expression.get(),
            basic_readers,
            clp_string_readers,
            var_string_readers,
            clp_queries,
            var_matches,
            num_messages
    };
}

auto ColumnScan::filter(uint64_t cur_message) -> bool {
    return 0 != m_matches[cur_message];
}

ColumnScan::ColumnScan(
        ast::Expression* expression,
        BasicReaderMap const& basic_readers,
        ClpStringReaderMap const& clp_string_readers,
        VarStringReaderMap const& var_string_readers,
        ClpQueryMap const& clp_queries,
        VarMatchMap const& var_matches,
        uint64_t num_messages
)
        : m_num_messages{num_messages},
          m_matches{build_node(
                  expression,
                  basic_readers,
                  clp_string_readers,
                  var_string_readers,
                  clp_queries,
                  var_matches
          )} {}

auto ColumnScan::can_build_node(
        ast::Expression* expr,
        ClpQueryMap const& clp_queries,
        VarMatchMap const& var_matches
) -> bool {
    if (nullptr == expr) {
        return false;
    }

    std::vector<ast::Expression*> pending{expr};
    while (false == pending.empty()) {
        auto* cur_expr = pending.back();
        pending.pop_back();

        if (auto* and_expr = dynamic_cast<AndExpr*>(cur_expr); nullptr != and_expr) {
            for (auto const& operand : and_expr->get_op_list()) {
                auto* child = dynamic_cast<ast::Expression*>(operand.get());
                pending.push_back(child);
            }
            continue;
        }
        if (auto* or_expr = dynamic_cast<OrExpr*>(cur_expr); nullptr != or_expr) {
            for (auto const& operand : or_expr->get_op_list()) {
                auto* child = dynamic_cast<ast::Expression*>(operand.get());
                pending.push_back(child);
            }
            continue;
        }
        if (auto* filter = dynamic_cast<FilterExpr*>(cur_expr); nullptr != filter) {
            if (false == can_build_filter(filter, clp_queries, var_matches)) {
                return false;
            }
            continue;
        }
        return false;
    }
    return true;
}

auto ColumnScan::can_build_filter(
        FilterExpr* filter,
        ClpQueryMap const& clp_queries,
        VarMatchMap const& var_matches
) -> bool {
    auto const column = filter->get_column();
    if (column->is_pure_wildcard() || column->is_unresolved_descriptor()
        || column->has_unresolved_tokens())
    {
        return false;
    }
    auto const operation = filter->get_operation();
    if (FilterOperation::EXISTS == operation || FilterOperation::NEXISTS == operation) {
        return true;
    }
    switch (column->get_literal_type()) {
        case LiteralType::IntegerT:
        case LiteralType::FloatT:
            return true;
        case LiteralType::BooleanT:
            return is_equality_operation(operation);
        case LiteralType::ClpStringT:
            return is_equality_operation(operation) && clp_queries.contains(filter);
        case LiteralType::VarStringT:
            return is_equality_operation(operation) && var_matches.contains(filter);
        case LiteralType::ArrayT:
        case LiteralType::NullT:
        case LiteralType::TimestampT:
        case LiteralType::UnknownT:
        case LiteralType::TypesEnd:
            return false;
    }
    return false;
}

// can_build_node validates the AST iteratively before this recursive bitmap construction runs.
// NOLINTBEGIN(misc-no-recursion)
auto ColumnScan::build_node(
        ast::Expression* expr,
        BasicReaderMap const& basic_readers,
        ClpStringReaderMap const& clp_string_readers,
        VarStringReaderMap const& var_string_readers,
        ClpQueryMap const& clp_queries,
        VarMatchMap const& var_matches
) const -> Bitmap {
    Bitmap result;
    if (auto* and_expr = dynamic_cast<AndExpr*>(expr); nullptr != and_expr) {
        result = Bitmap(m_num_messages, 1);
        for (auto const& operand : and_expr->get_op_list()) {
            auto* child_expr = dynamic_cast<ast::Expression*>(operand.get());
            auto child = build_node(
                    child_expr,
                    basic_readers,
                    clp_string_readers,
                    var_string_readers,
                    clp_queries,
                    var_matches
            );
            uint8_t any_set{0};
            for (size_t i{0}; i < result.size(); ++i) {
                result[i] &= child[i];
                any_set |= result[i];
            }
            if (0 == any_set) {
                break;
            }
        }
    } else if (auto* or_expr = dynamic_cast<OrExpr*>(expr); nullptr != or_expr) {
        result = Bitmap(m_num_messages, 0);
        for (auto const& operand : or_expr->get_op_list()) {
            auto* child_expr = dynamic_cast<ast::Expression*>(operand.get());
            auto child = build_node(
                    child_expr,
                    basic_readers,
                    clp_string_readers,
                    var_string_readers,
                    clp_queries,
                    var_matches
            );
            uint8_t all_set{1};
            for (size_t i{0}; i < result.size(); ++i) {
                result[i] |= child[i];
                all_set &= result[i];
            }
            if (0 != all_set) {
                break;
            }
        }
    } else if (auto* filter = dynamic_cast<FilterExpr*>(expr); nullptr != filter) {
        result = build_filter(
                filter,
                basic_readers,
                clp_string_readers,
                var_string_readers,
                clp_queries,
                var_matches
        );
    }

    if (expr->is_inverted()) {
        invert(result);
    }
    return result;
}

// NOLINTEND(misc-no-recursion)

auto ColumnScan::build_filter(
        FilterExpr* filter,
        BasicReaderMap const& basic_readers,
        ClpStringReaderMap const& clp_string_readers,
        VarStringReaderMap const& var_string_readers,
        ClpQueryMap const& clp_queries,
        VarMatchMap const& var_matches
) const -> Bitmap {
    Bitmap bitmap(m_num_messages, 0);
    auto const column = filter->get_column();
    auto const operation = filter->get_operation();
    if (FilterOperation::EXISTS == operation || FilterOperation::NEXISTS == operation) {
        std::fill(bitmap.begin(), bitmap.end(), 1);
        return bitmap;
    }

    auto const column_id = column->get_column_id();
    auto const& operand = filter->get_operand();

    switch (column->get_literal_type()) {
        case LiteralType::IntegerT: {
            int64_t operand_value{};
            if (false == operand->as_int(operand_value, operation)) {
                return bitmap;
            }
            return build_basic_filter(
                    m_num_messages,
                    basic_readers,
                    column_id,
                    operation,
                    operand_value
            );
        }
        case LiteralType::FloatT: {
            double operand_value{};
            if (false == operand->as_float(operand_value, operation)) {
                return bitmap;
            }
            return build_basic_filter(
                    m_num_messages,
                    basic_readers,
                    column_id,
                    operation,
                    operand_value
            );
        }
        case LiteralType::BooleanT: {
            bool operand_value{};
            if (false == operand->as_bool(operand_value, operation)) {
                return bitmap;
            }
            return build_basic_filter(
                    m_num_messages,
                    basic_readers,
                    column_id,
                    operation,
                    static_cast<uint8_t>(operand_value)
            );
        }
        case LiteralType::ClpStringT: {
            auto* const query = clp_queries.at(filter);
            return build_clp_string_filter(
                    m_num_messages,
                    clp_string_readers,
                    column_id,
                    operation,
                    query
            );
        }
        case LiteralType::VarStringT: {
            auto const* matching_vars = var_matches.at(filter);
            return build_var_string_filter(
                    m_num_messages,
                    var_string_readers,
                    column_id,
                    operation,
                    *matching_vars
            );
        }
        case LiteralType::ArrayT:
        case LiteralType::NullT:
        case LiteralType::TimestampT:
        case LiteralType::UnknownT:
        case LiteralType::TypesEnd:
            return bitmap;
    }
    return bitmap;
}
}  // namespace clp_s::search
