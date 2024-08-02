#include <any>
#include <iostream>
#include <string>

#include <antlr4-runtime.h>
#include <spdlog/spdlog.h>

#include "../EmptyExpr.hpp"
#include "SqlBaseVisitor.h"
#include "SqlLexer.h"
#include "SqlParser.h"

using namespace antlr4;
using namespace sql;

namespace clp_s::search::sql {
class ErrorListener : public BaseErrorListener {
public:
    void syntaxError(
            Recognizer* recognizer,
            Token* offending_symbol,
            size_t line,
            size_t char_position_in_line,
            std::string const& msg,
            std::exception_ptr e
    ) override {
        m_error = true;
        m_error_message = msg;
    }

    bool error() const { return m_error; }

    std::string const& message() const { return m_error_message; }

private:
    bool m_error{false};
    std::string m_error_message;
};

class ParseTreeVisitor : public SqlBaseVisitor {
public:
    std::any visitStart(SqlParser::StartContext* ctx) override { return EmptyExpr::create(); }
};

std::shared_ptr<Expression> parse_sql_expression(std::istream& in) {
    ErrorListener lexer_error_listener;
    ErrorListener parser_error_listener;

    ANTLRInputStream input(in);
    SqlLexer lexer(&input);
    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexer_error_listener);
    CommonTokenStream tokens(&lexer);
    SqlParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&parser_error_listener);
    SqlParser::StartContext* tree = parser.start();

    if (lexer_error_listener.error()) {
        SPDLOG_ERROR("Lexer error: {}", lexer_error_listener.message());
        return {};
    }
    if (parser_error_listener.error()) {
        SPDLOG_ERROR("Parser error: {}", parser_error_listener.message());
        return {};
    }

    ParseTreeVisitor visitor;
    return std::any_cast<std::shared_ptr<Expression>>(visitor.visitStart(tree));
}
}  // namespace clp_s::search::sql
