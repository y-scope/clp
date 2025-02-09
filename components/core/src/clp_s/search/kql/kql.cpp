#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <antlr4-runtime.h>
#include <spdlog/spdlog.h>

#include "../../Utils.hpp"
#include "../AndExpr.hpp"
#include "../antlr_common/ErrorListener.hpp"
#include "../BooleanLiteral.hpp"
#include "../ColumnDescriptor.hpp"
#include "../DateLiteral.hpp"
#include "../EmptyExpr.hpp"
#include "../FilterExpr.hpp"
#include "../Integral.hpp"
#include "../NullLiteral.hpp"
#include "../OrExpr.hpp"
#include "../StringLiteral.hpp"
#include "KqlBaseVisitor.h"
#include "KqlLexer.h"
#include "KqlParser.h"

using namespace antlr4;
using namespace kql;
using clp_s::search::antlr_common::ErrorListener;

namespace clp_s::search::kql {
namespace {
class ParseTreeVisitor : public KqlBaseVisitor {
private:
    static void
    prepend_column(std::shared_ptr<ColumnDescriptor> const& desc, DescriptorList const& prefix) {
        desc->insert(desc->get_descriptor_list().begin(), prefix);
    }

    void prepend_column(std::shared_ptr<Expression> const& expr, DescriptorList const& prefix) {
        for (auto const& op : expr->get_op_list()) {
            if (auto col = std::dynamic_pointer_cast<ColumnDescriptor>(op)) {
                prepend_column(col, prefix);
            } else if (auto subexpr = std::dynamic_pointer_cast<Expression>(op)) {
                prepend_column(subexpr, prefix);
            }
        }
    }

public:
    static std::string unquote_string(std::string const& text) {
        if (false == text.empty() && '"' == text.at(0)) {
            return text.substr(1, text.length() - 2);
        } else {
            return text;
        }
    }

    static std::string unquote_date_string(std::string const& text) {
        // date(...)
        // 012345
        return unquote_string(text.substr(5, text.size() - 6));
    }

    static std::shared_ptr<Literal> unquote_literal(std::string const& text) {
        std::string token;
        if (false == StringUtils::unescape_kql_value(unquote_string(text), token)) {
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
            return StringLiteral::create(StringUtils::clean_up_wildcard_search_string(token));
        }
    }

    static std::shared_ptr<Literal> unquote_date_literal(std::string const& text) {
        std::string token;
        if (false == StringUtils::unescape_kql_value(unquote_date_string(text), token)) {
            SPDLOG_ERROR("Can not parse invalid date literal: {}", text);
            throw std::runtime_error{"Invalid date literal."};
        }

        return DateLiteral::create_from_string(token);
    }

    std::any visitStart(KqlParser::StartContext* ctx) override {
        // only go through first child (query) and avoid default
        // behaviour of returning result from last child (EOF in this case)
        return ctx->children[0]->accept(this);
    }

    std::any visitColumn(KqlParser::ColumnContext* ctx) override {
        std::string column = unquote_string(ctx->LITERAL()->getText());

        std::vector<std::string> descriptor_tokens;
        if (false == StringUtils::tokenize_column_descriptor(column, descriptor_tokens)) {
            SPDLOG_ERROR("Can not tokenize invalid column: \"{}\"", column);
            return nullptr;
        }

        return ColumnDescriptor::create_from_escaped_tokens(descriptor_tokens);
    }

    std::any visitNestedQuery(KqlParser::NestedQueryContext* ctx) override {
        auto descriptor = std::any_cast<std::shared_ptr<ColumnDescriptor>>(ctx->col->accept(this));
        DescriptorList prefix = descriptor->get_descriptor_list();

        auto nested_expr = std::any_cast<std::shared_ptr<Expression>>(ctx->q->accept(this));
        prepend_column(nested_expr, prefix);

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

    std::any visitColumn_value_expression(KqlParser::Column_value_expressionContext* ctx) override {
        auto descriptor = std::any_cast<std::shared_ptr<ColumnDescriptor>>(ctx->col->accept(this));

        if (ctx->lit) {
            auto lit = unquote_literal(ctx->lit->getText());
            return FilterExpr::create(descriptor, FilterOperation::EQ, lit);
        } else if (ctx->date_lit) {
            auto lit = unquote_date_literal(ctx->date_lit->getText());
            return FilterExpr::create(descriptor, FilterOperation::EQ, lit);
        } else /*if (ctx->list) */ {
            auto list_expr = std::any_cast<std::shared_ptr<Expression>>(ctx->list->accept(this));
            DescriptorList prefix = descriptor->get_descriptor_list();
            prepend_column(list_expr, prefix);
            return list_expr;
        }
    }

    std::any visitColumn_range_expression(KqlParser::Column_range_expressionContext* ctx) override {
        auto descriptor = std::any_cast<std::shared_ptr<ColumnDescriptor>>(ctx->col->accept(this));
        std::shared_ptr<Literal> lit;
        if (ctx->lit) {
            lit = unquote_literal(ctx->lit->getText());
        } else /*if (ctx->date_lit)*/ {
            lit = unquote_date_literal(ctx->date_lit->getText());
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
        auto lit = unquote_literal(ctx->LITERAL()->getText());
        auto descriptor = ColumnDescriptor::create_from_escaped_token("*");
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

        auto empty_descriptor = ColumnDescriptor::create_from_descriptors(DescriptorList());
        for (auto token : ctx->literals) {
            auto literal = unquote_literal(token->getText());
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
}  // namespace clp_s::search::kql
