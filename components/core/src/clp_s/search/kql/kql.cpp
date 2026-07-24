#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <antlr4-runtime.h>
#include <spdlog/spdlog.h>
#include <string_utils/string_utils.hpp>

#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/FunctionCall.hpp>
#include <clp_s/search/ast/Integral.hpp>
#include <clp_s/search/ast/Literal.hpp>
#include <clp_s/search/ast/Value.hpp>
#include <clp_s/timestamp_parser/TimestampParser.hpp>
#include <clpp/Defs.hpp>

#include "../../archive_constants.hpp"
#include "../antlr_common/ErrorListener.hpp"
#include "../ast/AndExpr.hpp"
#include "../ast/BooleanLiteral.hpp"
#include "../ast/ColumnDescriptor.hpp"
#include "../ast/FilterExpr.hpp"
#include "../ast/FilterOperation.hpp"
#include "../ast/NullLiteral.hpp"
#include "../ast/OrExpr.hpp"
#include "../ast/SearchUtils.hpp"
#include "../ast/StringLiteral.hpp"
#include "../ast/TimestampLiteral.hpp"
#include "KqlBaseVisitor.h"
#include "KqlLexer.h"
#include "KqlParser.h"

using namespace antlr4;
using clp_s::search::antlr_common::ErrorListener;

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::BooleanLiteral;
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::DescriptorList;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::Integral;
using clp_s::search::ast::Literal;
using clp_s::search::ast::NullLiteral;
using clp_s::search::ast::OrExpr;
using clp_s::search::ast::StringLiteral;
using clp_s::search::ast::TimestampLiteral;

namespace clp_s::search::kql {
using generated::KqlBaseVisitor;
using generated::KqlLexer;
using generated::KqlParser;

namespace {
/**
 * Extracts a ColumnDescriptor from a parsed KQL value, handling FunctionCall unwrapping.
 *
 * For filter functions, sets the subtree_type on the extracted column.
 *
 * @param parsed The result of visiting a column context (either ColumnDescriptor or FunctionCall).
 * @return The extracted ColumnDescriptor, or nullptr if the value is invalid.
 */
auto extract_column_from_parsed_value(std::any const& parsed) -> std::shared_ptr<ColumnDescriptor>;

/**
 * Tokenizes a column name string into a ColumnDescriptor.
 *
 * @param column_text The raw column text (possibly quoted).
 * @return A ColumnDescriptor on success, nullptr on failure.
 */
auto parse_column_literal(std::string const& column_text) -> std::shared_ptr<ColumnDescriptor>;

auto extract_column_from_parsed_value(std::any const& parsed) -> std::shared_ptr<ColumnDescriptor> {
    if (auto const* value{std::any_cast<std::shared_ptr<ast::Value>>(&parsed)}) {
        if (auto func_call{std::dynamic_pointer_cast<ast::FunctionCall>(*value)}) {
            auto const& function_name{func_call->get_function_name()};
            auto const& args{func_call->get_args()};
            if (args.size() != 1) {
                SPDLOG_ERROR("Function '{}' requires exactly one column argument", function_name);
                return nullptr;
            }
            auto column{std::dynamic_pointer_cast<ColumnDescriptor>(args.at(0))};
            if (!column) {
                SPDLOG_ERROR("Function '{}' argument must be a column", function_name);
                return nullptr;
            }
            if (function_name == clpp::cShapeFunction) {
                column->set_subtree_type(std::string{clpp::cShapeFunction});
            } else {
                SPDLOG_ERROR("Function '{}' is not supported in filter expressions", function_name);
                return nullptr;
            }
            return column;
        }
        if (auto column{std::dynamic_pointer_cast<ColumnDescriptor>(*value)}) {
            return column;
        }
    }
    return std::any_cast<std::shared_ptr<ColumnDescriptor>>(parsed);
}

auto parse_column_literal(std::string const& column_text) -> std::shared_ptr<ColumnDescriptor> {
    std::vector<std::string> descriptor_tokens;
    std::string descriptor_namespace;
    if (false
        == clp_s::search::ast::tokenize_column_descriptor(
                column_text,
                descriptor_tokens,
                descriptor_namespace
        ))
    {
        SPDLOG_ERROR("Can not tokenize invalid column: \"{}\"", column_text);
        return nullptr;
    }
    return ColumnDescriptor::create_from_escaped_tokens(descriptor_tokens, descriptor_namespace);
}

class ParseTreeVisitor : public KqlBaseVisitor {
private:
    static void prepend_column(
            std::shared_ptr<ColumnDescriptor> const& desc,
            std::shared_ptr<ColumnDescriptor> const& prefix
    ) {
        desc->insert(desc->get_descriptor_list().begin(), prefix->get_descriptor_list());
        if (false == desc->get_namespace().empty()) {
            throw std::runtime_error{"Invalid descriptor."};
        }

        desc->set_namespace(prefix->get_namespace());
    }

    void prepend_column(
            std::shared_ptr<Expression> const& expr,
            std::shared_ptr<ColumnDescriptor> const& prefix
    ) {
        for (auto const& op : expr->get_op_list()) {
            if (auto col = std::dynamic_pointer_cast<ColumnDescriptor>(op)) {
                prepend_column(col, prefix);
            } else if (auto subexpr = std::dynamic_pointer_cast<Expression>(op)) {
                prepend_column(subexpr, prefix);
            }
        }
    }

    static std::shared_ptr<Literal> create_timestamp_literal(
            KqlParser::Timestamp_expressionContext const& ctx
    ) {
        if (nullptr == ctx.timestamp) {
            return nullptr;
        }

        auto const timestamp_str{ctx.timestamp->getText()};
        std::string generated_pattern;
        if (nullptr != ctx.pattern) {
            auto const pattern_str{ctx.pattern->getText()};
            auto const pattern_result{timestamp_parser::TimestampPattern::create(pattern_str)};
            if (pattern_result.has_error()) {
                SPDLOG_ERROR(
                        "Invalid timestamp pattern {} - {}",
                        pattern_str,
                        pattern_result.error().message()
                );
                return nullptr;
            }

            auto const timestamp_result{timestamp_parser::parse_timestamp(
                    timestamp_str,
                    pattern_result.value(),
                    true,
                    generated_pattern
            )};
            if (timestamp_result.has_error()) {
                SPDLOG_ERROR(
                        "Failed to parse timestamp {} using pattern {} - {}",
                        timestamp_str,
                        pattern_str,
                        timestamp_result.error().message()
                );
                return nullptr;
            }
            return TimestampLiteral::create(timestamp_result.value().first);
        }

        auto const quoted_patterns_result{
                timestamp_parser::get_all_default_quoted_timestamp_patterns()
        };
        if (quoted_patterns_result.has_error()) {
            SPDLOG_ERROR(
                    "Unexpected error while trying to load default timestamp patterns - {}",
                    quoted_patterns_result.error().message()
            );
            return nullptr;
        }

        auto const optional_timestamp{timestamp_parser::search_known_timestamp_patterns(
                timestamp_str,
                quoted_patterns_result.value(),
                true,
                generated_pattern
        )};
        if (false == optional_timestamp.has_value()) {
            SPDLOG_ERROR(
                    "Failed to parse timestamp {} using default timestamp patterns.",
                    timestamp_str
            );
            return nullptr;
        }
        return TimestampLiteral::create(optional_timestamp->first);
    }

public:
    static std::string unquote_string(std::string const& text) {
        if (false == text.empty() && '"' == text.at(0)) {
            return text.substr(1, text.length() - 2);
        } else {
            return text;
        }
    }

    static std::shared_ptr<Literal> create_literal(std::string const& text) {
        std::string token;
        if (false == clp_s::search::ast::unescape_kql_value(text, token)) {
            SPDLOG_ERROR("Can not parse invalid literal: {}", text);
            throw std::runtime_error{"Invalid literal."};
        }

        if (auto ret = Integral::create_from_string(token)) {
            return ret;
        } else if (auto ret = BooleanLiteral::create_from_string(token)) {
            return ret;
        } else if (auto ret = NullLiteral::create_from_string(token)) {
            return ret;
        } else {
            return StringLiteral::create(clp::string_utils::clean_up_wildcard_search_string(token));
        }
    }

    std::any visitLiteral(KqlParser::LiteralContext* ctx) override {
        if (nullptr != ctx->QUOTED_STRING()) {
            return create_literal(unquote_string(ctx->QUOTED_STRING()->getText()));
        } else {
            return create_literal(ctx->UNQUOTED_LITERAL()->getText());
        }
    }

    std::any visitStart(KqlParser::StartContext* ctx) override {
        // only go through first child (query) and avoid default
        // behaviour of returning result from last child (EOF in this case)
        return ctx->children[0]->accept(this);
    }

    std::any visitColumn(KqlParser::ColumnContext* ctx) override {
        if (nullptr != ctx->function_call()) {
            return ctx->function_call()->accept(this);
        }
        auto column{parse_column_literal(unquote_string(ctx->literal()->getText()))};
        if (nullptr == column) {
            return std::any{};
        }
        return column;
    }

    std::any visitColumn_literal(KqlParser::Column_literalContext* ctx) override {
        auto column{parse_column_literal(unquote_string(ctx->literal()->getText()))};
        if (nullptr == column) {
            return std::any{};
        }
        return column;
    }

    std::any visitFunction_call(KqlParser::Function_callContext* ctx) override {
        auto const function_name{ctx->func->getText()};
        if (clpp::cShapeFunction != function_name && clpp::cDecomposeFunction != function_name) {
            throw std::runtime_error{fmt::format("Unknown function in query: '{}'", function_name)};
        }

        std::any column_any{ctx->column_literal()->accept(this)};
        if (false == column_any.has_value()) {
            SPDLOG_ERROR("Failed to parse column argument for function '{}'", function_name);
            return std::any{};
        }
        auto column{std::any_cast<std::shared_ptr<ColumnDescriptor>>(column_any)};
        return std::any{std::static_pointer_cast<clp_s::search::ast::Value>(
                clp_s::search::ast::FunctionCall::create(std::string{function_name}, column)
        )};
    }

    std::any visitProjection_column(KqlParser::Projection_columnContext* ctx) override {
        if (nullptr != ctx->function_call()) {
            return ctx->function_call()->accept(this);
        }
        auto column{parse_column_literal(unquote_string(ctx->literal()->getText()))};
        if (nullptr == column) {
            return std::any{};
        }
        return std::any{std::static_pointer_cast<clp_s::search::ast::Value>(column)};
    }

    std::any visitNestedQuery(KqlParser::NestedQueryContext* ctx) override {
        auto descriptor = std::any_cast<std::shared_ptr<ColumnDescriptor>>(ctx->col->accept(this));

        auto nested_expr = std::any_cast<std::shared_ptr<Expression>>(ctx->q->accept(this));
        prepend_column(nested_expr, descriptor);

        return nested_expr;
    }

    std::any visitOrAndQuery(KqlParser::OrAndQueryContext* ctx) override {
        auto lhs = std::any_cast<std::shared_ptr<Expression>>(ctx->lhs->accept(this));
        auto rhs = std::any_cast<std::shared_ptr<Expression>>(ctx->rhs->accept(this));
        if (ctx->op->getType() == KqlParser::AND) {
            return AndExpr::create(lhs, rhs);
        } else {
            return OrExpr::create(lhs, rhs);
        }
    }

    std::any visitNotQuery(KqlParser::NotQueryContext* ctx) override {
        auto q = std::any_cast<std::shared_ptr<Expression>>(ctx->q->accept(this));
        q->invert();
        return q;
    }

    std::any visitSubQuery(KqlParser::SubQueryContext* ctx) override {
        return ctx->q->accept(this);
    }

    auto visitColumn_value_expression(KqlParser::Column_value_expressionContext* ctx)
            -> std::any override {
        auto parsed = ctx->col->accept(this);
        auto descriptor{extract_column_from_parsed_value(parsed)};
        if (!descriptor) {
            return std::any{};
        }

        if (nullptr != ctx->lit) {
            auto lit{std::any_cast<std::shared_ptr<Literal>>(ctx->lit->accept(this))};
            return FilterExpr::create(descriptor, FilterOperation::EQ, lit);
        } else if (nullptr != ctx->timestamp) {
            auto lit{create_timestamp_literal(*ctx->timestamp)};
            if (nullptr == lit) {
                return std::any{};
            }
            return FilterExpr::create(descriptor, FilterOperation::EQ, lit);
        } else if (nullptr != ctx->list) {
            auto list_expr = std::any_cast<std::shared_ptr<Expression>>(ctx->list->accept(this));
            prepend_column(list_expr, descriptor);
            return list_expr;
        } else {
            return std::any{};
        }
    }

    auto visitColumn_range_expression(KqlParser::Column_range_expressionContext* ctx)
            -> std::any override {
        auto parsed = ctx->col->accept(this);
        auto descriptor{extract_column_from_parsed_value(parsed)};
        if (!descriptor) {
            return std::any{};
        }
        std::shared_ptr<Literal> lit;
        if (ctx->lit) {
            lit = std::any_cast<std::shared_ptr<Literal>>(ctx->lit->accept(this));
        } else if (nullptr != ctx->timestamp) {
            lit = create_timestamp_literal(*ctx->timestamp);
        }

        if (nullptr == lit) {
            return std::any{};
        }

        std::string range_op = ctx->RANGE_OPERATOR()->getText();
        FilterOperation op = FilterOperation::EQ;
        if (range_op == "<=") {
            op = FilterOperation::LTE;
        } else if (range_op == ">=") {
            op = FilterOperation::GTE;
        } else if (range_op == "<") {
            op = FilterOperation::LT;
        } else if (range_op == ">") {
            op = FilterOperation::GT;
        }

        return FilterExpr::create(descriptor, op, lit);
    }

    std::any visitValue_expression(KqlParser::Value_expressionContext* ctx) override {
        auto lit{std::any_cast<std::shared_ptr<Literal>>(ctx->literal()->accept(this))};

        // TODO: consider if this should somehow be allowed to match all namespaces. "*" namespace?
        auto descriptor
                = ColumnDescriptor::create_from_escaped_tokens({"*"}, constants::cDefaultNamespace);
        return FilterExpr::create(descriptor, FilterOperation::EQ, lit);
    }

    std::any visitList_of_values(KqlParser::List_of_valuesContext* ctx) override {
        std::shared_ptr<Expression> base(nullptr);
        bool invert_each_filter = false;
        if (ctx->condition) {
            if (ctx->AND()) {
                base = AndExpr::create();
            } else if (ctx->OR()) {
                base = OrExpr::create();
            } else if (ctx->NOT()) {
                invert_each_filter = true;
                base = AndExpr::create();
            }
        } else {
            base = OrExpr::create();
        }

        auto empty_descriptor = ColumnDescriptor::create_from_descriptors(
                DescriptorList(),
                constants::cDefaultNamespace
        );
        for (auto token : ctx->literals) {
            auto literal{std::any_cast<std::shared_ptr<Literal>>(token->accept(this))};
            auto expr = FilterExpr::create(
                    empty_descriptor,
                    FilterOperation::EQ,
                    literal,
                    invert_each_filter
            );
            base->add_operand(expr);
        }
        return base;
    }
};
}  // namespace

std::shared_ptr<Expression> parse_kql_expression(std::istream& in) {
    ErrorListener lexer_error_listener;
    ErrorListener parser_error_listener;

    ANTLRInputStream input(in);
    KqlLexer lexer(&input);
    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexer_error_listener);
    CommonTokenStream tokens(&lexer);
    KqlParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&parser_error_listener);
    KqlParser::StartContext* tree = parser.start();

    if (lexer_error_listener.error()) {
        SPDLOG_ERROR("Lexer error: {}", lexer_error_listener.message());
        return {};
    } else if (parser_error_listener.error()) {
        SPDLOG_ERROR("Parser error: {}", parser_error_listener.message());
        return {};
    }

    ParseTreeVisitor visitor;
    try {
        return std::any_cast<std::shared_ptr<Expression>>(visitor.visitStart(tree));
    } catch (std::exception& e) {
        return {};
    }
}

auto parse_projection_column(std::string_view column_str)
        -> std::shared_ptr<clp_s::search::ast::Value> {
    ErrorListener lexer_error_listener;
    ErrorListener parser_error_listener;

    ANTLRInputStream input(std::string{column_str});
    KqlLexer lexer(&input);
    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexer_error_listener);
    CommonTokenStream tokens(&lexer);
    KqlParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&parser_error_listener);
    KqlParser::Projection_columnContext* tree{parser.projection_column()};

    if (lexer_error_listener.error()) {
        SPDLOG_ERROR("Lexer error: {}", lexer_error_listener.message());
        return {};
    }
    if (parser_error_listener.error()) {
        SPDLOG_ERROR("Parser error: {}", parser_error_listener.message());
        return {};
    }

    ParseTreeVisitor visitor;
    try {
        return std::any_cast<std::shared_ptr<clp_s::search::ast::Value>>(
                visitor.visitProjection_column(tree)
        );
    } catch (std::exception const& e) {
        SPDLOG_ERROR("parse_projection_column exception: {}", e.what());
        return {};
    }
}
}  // namespace clp_s::search::kql
