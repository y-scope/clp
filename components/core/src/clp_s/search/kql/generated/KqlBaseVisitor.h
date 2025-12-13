
// Generated from clp_s/search/kql/Kql.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "KqlVisitor.h"


namespace clp_s::search::kql::generated {

/**
 * This class provides an empty implementation of KqlVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  KqlBaseVisitor : public KqlVisitor {
public:

  virtual std::any visitStart(KqlParser::StartContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpr(KqlParser::ExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNestedQuery(KqlParser::NestedQueryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNotQuery(KqlParser::NotQueryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubQuery(KqlParser::SubQueryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOrAndQuery(KqlParser::OrAndQueryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpression(KqlParser::ExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumn_range_expression(KqlParser::Column_range_expressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumn_value_expression(KqlParser::Column_value_expressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitColumn(KqlParser::ColumnContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValue_expression(KqlParser::Value_expressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitList_of_values(KqlParser::List_of_valuesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTimestamp_expression(KqlParser::Timestamp_expressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLiteral(KqlParser::LiteralContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace clp_s::search::kql::generated
