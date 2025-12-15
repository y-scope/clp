
// Generated from clp_s/search/kql/Kql.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "KqlParser.h"


namespace clp_s::search::kql::generated {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by KqlParser.
 */
class  KqlVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by KqlParser.
   */
    virtual std::any visitStart(KqlParser::StartContext *context) = 0;

    virtual std::any visitExpr(KqlParser::ExprContext *context) = 0;

    virtual std::any visitNestedQuery(KqlParser::NestedQueryContext *context) = 0;

    virtual std::any visitNotQuery(KqlParser::NotQueryContext *context) = 0;

    virtual std::any visitSubQuery(KqlParser::SubQueryContext *context) = 0;

    virtual std::any visitOrAndQuery(KqlParser::OrAndQueryContext *context) = 0;

    virtual std::any visitExpression(KqlParser::ExpressionContext *context) = 0;

    virtual std::any visitColumn_range_expression(KqlParser::Column_range_expressionContext *context) = 0;

    virtual std::any visitColumn_value_expression(KqlParser::Column_value_expressionContext *context) = 0;

    virtual std::any visitColumn(KqlParser::ColumnContext *context) = 0;

    virtual std::any visitValue_expression(KqlParser::Value_expressionContext *context) = 0;

    virtual std::any visitList_of_values(KqlParser::List_of_valuesContext *context) = 0;

    virtual std::any visitTimestamp_expression(KqlParser::Timestamp_expressionContext *context) = 0;

    virtual std::any visitLiteral(KqlParser::LiteralContext *context) = 0;


};

}  // namespace clp_s::search::kql::generated
