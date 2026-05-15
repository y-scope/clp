#include "ColumnScan.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
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
            return false;
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
        for (uint64_t row{0}; row < num_messages; ++row) {
            auto const value = std::get<T>(reader->extract_value(row));
            bitmap[row] |= compare(operation, value, operand) ? 1 : 0;
        }
    }
    return bitmap;
}

[[nodiscard]] auto
clp_string_matches(ClpStringColumnReader* reader, clp::Query const& query, uint64_t row) -> bool {
    auto const encoded_id = reader->get_encoded_id(row);
    auto const encoded_vars = reader->get_encoded_vars(row);
    if (query.contains_sub_queries()) {
        for (auto const& subquery : query.get_sub_queries()) {
            if (false == subquery.matches_logtype(encoded_id)
                || false == subquery.matches_vars(encoded_vars))
            {
                continue;
            }
            if (subquery.wildcard_match_required()) {
                return clp::string_utils::wildcard_match_unsafe(
                        std::get<std::string>(reader->extract_value(row)),
                        query.get_search_string(),
                        false == query.get_ignore_case()
                );
            }
            return true;
        }
        return false;
    }

    return clp::string_utils::wildcard_match_unsafe(
            std::get<std::string>(reader->extract_value(row)),
            query.get_search_string(),
            false == query.get_ignore_case()
    );
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
        for (uint64_t row{0}; row < num_messages; ++row) {
            auto const matched = clp_string_matches(reader, *query, row);
            bitmap[row] |= ((FilterOperation::EQ == operation) == matched) ? 1 : 0;
        }
    }
    return bitmap;
}

[[nodiscard]] auto build_var_string_filter(
        uint64_t num_messages,
        ColumnScan::VarStringReaderMap const& reader_map,
        int32_t column_id,
        FilterOperation operation,
        std::unordered_set<int64_t> const* matching_vars
) -> ColumnScan::Bitmap {
    ColumnScan::Bitmap bitmap(num_messages, 0);
    auto const readers = reader_map.find(column_id);
    if (nullptr == matching_vars || reader_map.end() == readers) {
        return bitmap;
    }
    for (auto* reader : readers->second) {
        for (uint64_t row{0}; row < num_messages; ++row) {
            auto const matched
                    = matching_vars->contains(static_cast<int64_t>(reader->get_variable_id(row)));
            bitmap[row] |= ((FilterOperation::EQ == operation) == matched) ? 1 : 0;
        }
    }
    return bitmap;
}
}  // namespace

auto ColumnScan::try_create(
        std::shared_ptr<ast::Expression> expression,
        BasicReaderMap const& basic_readers,
        ClpStringReaderMap const& clp_string_readers,
        VarStringReaderMap const& var_string_readers,
        ClpQueryMap const& clp_queries,
        VarMatchMap const& var_matches,
        uint64_t num_messages
) -> std::optional<ColumnScan> {
    ColumnScan scan{
            std::move(expression),
            basic_readers,
            clp_string_readers,
            var_string_readers,
            clp_queries,
            var_matches,
            num_messages
    };

    if (false == scan.can_build_node(scan.m_expression.get())) {
        return std::nullopt;
    }

    scan.m_matches = scan.build_node(scan.m_expression.get());
    return std::move(scan);
}

ColumnScan::ColumnScan(
        std::shared_ptr<ast::Expression> expression,
        BasicReaderMap const& basic_readers,
        ClpStringReaderMap const& clp_string_readers,
        VarStringReaderMap const& var_string_readers,
        ClpQueryMap const& clp_queries,
        VarMatchMap const& var_matches,
        uint64_t num_messages
)
        : m_expression{std::move(expression)},
          m_basic_readers{&basic_readers},
          m_clp_string_readers{&clp_string_readers},
          m_var_string_readers{&var_string_readers},
          m_clp_queries{&clp_queries},
          m_var_matches{&var_matches},
          m_num_messages{num_messages} {}

auto ColumnScan::filter(uint64_t cur_message) -> bool {
    return 0 != m_matches[cur_message];
}

auto ColumnScan::can_build_node(ast::Expression* expr) const -> bool {
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
                if (nullptr == child) {
                    return false;
                }
                pending.push_back(child);
            }
            continue;
        }
        if (auto* or_expr = dynamic_cast<OrExpr*>(cur_expr); nullptr != or_expr) {
            for (auto const& operand : or_expr->get_op_list()) {
                auto* child = dynamic_cast<ast::Expression*>(operand.get());
                if (nullptr == child) {
                    return false;
                }
                pending.push_back(child);
            }
            continue;
        }
        if (false == can_build_filter(dynamic_cast<FilterExpr*>(cur_expr))) {
            return false;
        }
    }
    return true;
}

auto ColumnScan::can_build_filter(FilterExpr* filter) const -> bool {
    if (nullptr == filter) {
        return false;
    }
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
            return is_equality_operation(operation) && m_clp_queries->contains(filter);
        case LiteralType::VarStringT:
            return is_equality_operation(operation) && m_var_matches->contains(filter);
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
auto ColumnScan::build_node(ast::Expression* expr) const -> Bitmap {
    assert(nullptr != expr);

    Bitmap result;
    if (auto* and_expr = dynamic_cast<AndExpr*>(expr); nullptr != and_expr) {
        result = Bitmap(m_num_messages, 1);
        for (auto const& operand : and_expr->get_op_list()) {
            auto* child_expr = dynamic_cast<ast::Expression*>(operand.get());
            assert(nullptr != child_expr);
            auto child = build_node(child_expr);
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
            assert(nullptr != child_expr);
            auto child = build_node(child_expr);
            uint8_t all_set{1};
            for (size_t i{0}; i < result.size(); ++i) {
                result[i] |= child[i];
                all_set &= result[i];
            }
            if (0 != all_set) {
                break;
            }
        }
    } else {
        auto* filter = dynamic_cast<FilterExpr*>(expr);
        assert(nullptr != filter);
        if (nullptr == filter) {
            Bitmap empty(m_num_messages, 0);
            return empty;
        }
        result = build_filter(filter);
    }

    if (expr->is_inverted()) {
        invert(result);
    }
    return result;
}

// NOLINTEND(misc-no-recursion)

auto ColumnScan::build_filter(FilterExpr* filter) const -> Bitmap {
    assert(nullptr != filter);

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
                    *m_basic_readers,
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
                    *m_basic_readers,
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
                    *m_basic_readers,
                    column_id,
                    operation,
                    static_cast<uint8_t>(operand_value)
            );
        }
        case LiteralType::ClpStringT: {
            auto* const query = m_clp_queries->at(filter);
            return build_clp_string_filter(
                    m_num_messages,
                    *m_clp_string_readers,
                    column_id,
                    operation,
                    query
            );
        }
        case LiteralType::VarStringT: {
            auto const* matching_vars = m_var_matches->at(filter);
            return build_var_string_filter(
                    m_num_messages,
                    *m_var_string_readers,
                    column_id,
                    operation,
                    matching_vars
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
