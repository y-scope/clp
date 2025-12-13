
// Generated from clp_s/search/kql/Kql.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace clp_s::search::kql::generated {


class  KqlParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    AND = 8, OR = 9, NOT = 10, QUOTED_STRING = 11, UNQUOTED_LITERAL = 12, 
    RANGE_OPERATOR = 13, SPACE = 14
  };

  enum {
    RuleStart = 0, RuleQuery = 1, RuleExpression = 2, RuleColumn_range_expression = 3, 
    RuleColumn_value_expression = 4, RuleColumn = 5, RuleValue_expression = 6, 
    RuleList_of_values = 7, RuleTimestamp_expression = 8, RuleLiteral = 9
  };

  explicit KqlParser(antlr4::TokenStream *input);

  KqlParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~KqlParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class StartContext;
  class QueryContext;
  class ExpressionContext;
  class Column_range_expressionContext;
  class Column_value_expressionContext;
  class ColumnContext;
  class Value_expressionContext;
  class List_of_valuesContext;
  class Timestamp_expressionContext;
  class LiteralContext; 

  class  StartContext : public antlr4::ParserRuleContext {
  public:
    StartContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    QueryContext *query();
    antlr4::tree::TerminalNode *EOF();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  StartContext* start();

  class  QueryContext : public antlr4::ParserRuleContext {
  public:
    QueryContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    QueryContext() = default;
    void copyFrom(QueryContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ExprContext : public QueryContext {
  public:
    ExprContext(QueryContext *ctx);

    ExpressionContext *expression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  NestedQueryContext : public QueryContext {
  public:
    NestedQueryContext(QueryContext *ctx);

    KqlParser::ColumnContext *col = nullptr;
    KqlParser::QueryContext *q = nullptr;
    ColumnContext *column();
    QueryContext *query();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  NotQueryContext : public QueryContext {
  public:
    NotQueryContext(QueryContext *ctx);

    KqlParser::QueryContext *q = nullptr;
    antlr4::tree::TerminalNode *NOT();
    QueryContext *query();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  SubQueryContext : public QueryContext {
  public:
    SubQueryContext(QueryContext *ctx);

    KqlParser::QueryContext *q = nullptr;
    QueryContext *query();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  OrAndQueryContext : public QueryContext {
  public:
    OrAndQueryContext(QueryContext *ctx);

    KqlParser::QueryContext *lhs = nullptr;
    antlr4::Token *op = nullptr;
    KqlParser::QueryContext *rhs = nullptr;
    std::vector<QueryContext *> query();
    QueryContext* query(size_t i);
    antlr4::tree::TerminalNode *OR();
    antlr4::tree::TerminalNode *AND();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  QueryContext* query();
  QueryContext* query(int precedence);
  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Column_range_expressionContext *column_range_expression();
    Column_value_expressionContext *column_value_expression();
    Value_expressionContext *value_expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionContext* expression();

  class  Column_range_expressionContext : public antlr4::ParserRuleContext {
  public:
    KqlParser::ColumnContext *col = nullptr;
    KqlParser::Timestamp_expressionContext *timestamp = nullptr;
    KqlParser::LiteralContext *lit = nullptr;
    Column_range_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *RANGE_OPERATOR();
    ColumnContext *column();
    Timestamp_expressionContext *timestamp_expression();
    LiteralContext *literal();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Column_range_expressionContext* column_range_expression();

  class  Column_value_expressionContext : public antlr4::ParserRuleContext {
  public:
    KqlParser::ColumnContext *col = nullptr;
    KqlParser::List_of_valuesContext *list = nullptr;
    KqlParser::Timestamp_expressionContext *timestamp = nullptr;
    KqlParser::LiteralContext *lit = nullptr;
    Column_value_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ColumnContext *column();
    List_of_valuesContext *list_of_values();
    Timestamp_expressionContext *timestamp_expression();
    LiteralContext *literal();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Column_value_expressionContext* column_value_expression();

  class  ColumnContext : public antlr4::ParserRuleContext {
  public:
    ColumnContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LiteralContext *literal();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ColumnContext* column();

  class  Value_expressionContext : public antlr4::ParserRuleContext {
  public:
    Value_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    LiteralContext *literal();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Value_expressionContext* value_expression();

  class  List_of_valuesContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *condition = nullptr;
    KqlParser::LiteralContext *literalContext = nullptr;
    std::vector<LiteralContext *> literals;
    List_of_valuesContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<LiteralContext *> literal();
    LiteralContext* literal(size_t i);
    antlr4::tree::TerminalNode *AND();
    antlr4::tree::TerminalNode *OR();
    antlr4::tree::TerminalNode *NOT();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  List_of_valuesContext* list_of_values();

  class  Timestamp_expressionContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *timestamp = nullptr;
    antlr4::Token *pattern = nullptr;
    Timestamp_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> QUOTED_STRING();
    antlr4::tree::TerminalNode* QUOTED_STRING(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Timestamp_expressionContext* timestamp_expression();

  class  LiteralContext : public antlr4::ParserRuleContext {
  public:
    LiteralContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *QUOTED_STRING();
    antlr4::tree::TerminalNode *UNQUOTED_LITERAL();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  LiteralContext* literal();


  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool querySempred(QueryContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

}  // namespace clp_s::search::kql::generated
