
// Generated from clp_s/search/kql/Kql.g4 by ANTLR 4.13.2


#include "KqlVisitor.h"

#include "KqlParser.h"


using namespace antlrcpp;
using namespace clp_s::search::kql::generated;

using namespace antlr4;

namespace {

struct KqlParserStaticData final {
  KqlParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  KqlParserStaticData(const KqlParserStaticData&) = delete;
  KqlParserStaticData(KqlParserStaticData&&) = delete;
  KqlParserStaticData& operator=(const KqlParserStaticData&) = delete;
  KqlParserStaticData& operator=(KqlParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag kqlParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<KqlParserStaticData> kqlParserStaticData = nullptr;

void kqlParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (kqlParserStaticData != nullptr) {
    return;
  }
#else
  assert(kqlParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<KqlParserStaticData>(
    std::vector<std::string>{
      "start", "query", "expression", "column_range_expression", "column_value_expression", 
      "function_call", "column", "column_literal", "projection_column", 
      "value_expression", "list_of_values", "timestamp_expression", "literal"
    },
    std::vector<std::string>{
      "", "':'", "'{'", "'}'", "'('", "')'", "'timestamp('", "','"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "AND", "OR", "NOT", "QUOTED_STRING", 
      "UNQUOTED_LITERAL", "RANGE_OPERATOR", "SPACE"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,14,111,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,1,0,1,0,1,0,1,1,1,
  	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,44,8,1,1,1,1,1,
  	1,1,5,1,49,8,1,10,1,12,1,52,9,1,1,2,1,2,1,2,3,2,57,8,2,1,3,1,3,1,3,1,
  	3,3,3,63,8,3,1,4,1,4,1,4,1,4,1,4,3,4,70,8,4,1,5,1,5,1,5,1,5,1,5,1,6,1,
  	6,3,6,79,8,6,1,7,1,7,1,8,1,8,3,8,85,8,8,1,9,1,9,1,10,1,10,3,10,91,8,10,
  	1,10,5,10,94,8,10,10,10,12,10,97,9,10,1,10,1,10,1,11,1,11,1,11,1,11,3,
  	11,105,8,11,1,11,1,11,1,12,1,12,1,12,0,1,2,13,0,2,4,6,8,10,12,14,16,18,
  	20,22,24,0,3,1,0,8,9,1,0,8,10,1,0,11,12,111,0,26,1,0,0,0,2,43,1,0,0,0,
  	4,56,1,0,0,0,6,58,1,0,0,0,8,64,1,0,0,0,10,71,1,0,0,0,12,78,1,0,0,0,14,
  	80,1,0,0,0,16,84,1,0,0,0,18,86,1,0,0,0,20,88,1,0,0,0,22,100,1,0,0,0,24,
  	108,1,0,0,0,26,27,3,2,1,0,27,28,5,0,0,1,28,1,1,0,0,0,29,30,6,1,-1,0,30,
  	31,3,12,6,0,31,32,5,1,0,0,32,33,5,2,0,0,33,34,3,2,1,0,34,35,5,3,0,0,35,
  	44,1,0,0,0,36,37,5,4,0,0,37,38,3,2,1,0,38,39,5,5,0,0,39,44,1,0,0,0,40,
  	41,5,10,0,0,41,44,3,2,1,3,42,44,3,4,2,0,43,29,1,0,0,0,43,36,1,0,0,0,43,
  	40,1,0,0,0,43,42,1,0,0,0,44,50,1,0,0,0,45,46,10,2,0,0,46,47,7,0,0,0,47,
  	49,3,2,1,3,48,45,1,0,0,0,49,52,1,0,0,0,50,48,1,0,0,0,50,51,1,0,0,0,51,
  	3,1,0,0,0,52,50,1,0,0,0,53,57,3,6,3,0,54,57,3,8,4,0,55,57,3,18,9,0,56,
  	53,1,0,0,0,56,54,1,0,0,0,56,55,1,0,0,0,57,5,1,0,0,0,58,59,3,12,6,0,59,
  	62,5,13,0,0,60,63,3,22,11,0,61,63,3,24,12,0,62,60,1,0,0,0,62,61,1,0,0,
  	0,63,7,1,0,0,0,64,65,3,12,6,0,65,69,5,1,0,0,66,70,3,20,10,0,67,70,3,22,
  	11,0,68,70,3,24,12,0,69,66,1,0,0,0,69,67,1,0,0,0,69,68,1,0,0,0,70,9,1,
  	0,0,0,71,72,5,12,0,0,72,73,5,4,0,0,73,74,3,14,7,0,74,75,5,5,0,0,75,11,
  	1,0,0,0,76,79,3,10,5,0,77,79,3,24,12,0,78,76,1,0,0,0,78,77,1,0,0,0,79,
  	13,1,0,0,0,80,81,3,24,12,0,81,15,1,0,0,0,82,85,3,10,5,0,83,85,3,24,12,
  	0,84,82,1,0,0,0,84,83,1,0,0,0,85,17,1,0,0,0,86,87,3,24,12,0,87,19,1,0,
  	0,0,88,90,5,4,0,0,89,91,7,1,0,0,90,89,1,0,0,0,90,91,1,0,0,0,91,95,1,0,
  	0,0,92,94,3,24,12,0,93,92,1,0,0,0,94,97,1,0,0,0,95,93,1,0,0,0,95,96,1,
  	0,0,0,96,98,1,0,0,0,97,95,1,0,0,0,98,99,5,5,0,0,99,21,1,0,0,0,100,101,
  	5,6,0,0,101,104,5,11,0,0,102,103,5,7,0,0,103,105,5,11,0,0,104,102,1,0,
  	0,0,104,105,1,0,0,0,105,106,1,0,0,0,106,107,5,5,0,0,107,23,1,0,0,0,108,
  	109,7,2,0,0,109,25,1,0,0,0,10,43,50,56,62,69,78,84,90,95,104
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  kqlParserStaticData = std::move(staticData);
}

}

KqlParser::KqlParser(TokenStream *input) : KqlParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

KqlParser::KqlParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  KqlParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *kqlParserStaticData->atn, kqlParserStaticData->decisionToDFA, kqlParserStaticData->sharedContextCache, options);
}

KqlParser::~KqlParser() {
  delete _interpreter;
}

const atn::ATN& KqlParser::getATN() const {
  return *kqlParserStaticData->atn;
}

std::string KqlParser::getGrammarFileName() const {
  return "Kql.g4";
}

const std::vector<std::string>& KqlParser::getRuleNames() const {
  return kqlParserStaticData->ruleNames;
}

const dfa::Vocabulary& KqlParser::getVocabulary() const {
  return kqlParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView KqlParser::getSerializedATN() const {
  return kqlParserStaticData->serializedATN;
}


//----------------- StartContext ------------------------------------------------------------------

KqlParser::StartContext::StartContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

KqlParser::QueryContext* KqlParser::StartContext::query() {
  return getRuleContext<KqlParser::QueryContext>(0);
}

tree::TerminalNode* KqlParser::StartContext::EOF() {
  return getToken(KqlParser::EOF, 0);
}


size_t KqlParser::StartContext::getRuleIndex() const {
  return KqlParser::RuleStart;
}


std::any KqlParser::StartContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitStart(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::StartContext* KqlParser::start() {
  StartContext *_localctx = _tracker.createInstance<StartContext>(_ctx, getState());
  enterRule(_localctx, 0, KqlParser::RuleStart);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(26);
    query(0);
    setState(27);
    match(KqlParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QueryContext ------------------------------------------------------------------

KqlParser::QueryContext::QueryContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t KqlParser::QueryContext::getRuleIndex() const {
  return KqlParser::RuleQuery;
}

void KqlParser::QueryContext::copyFrom(QueryContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExprContext ------------------------------------------------------------------

KqlParser::ExpressionContext* KqlParser::ExprContext::expression() {
  return getRuleContext<KqlParser::ExpressionContext>(0);
}

KqlParser::ExprContext::ExprContext(QueryContext *ctx) { copyFrom(ctx); }


std::any KqlParser::ExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NestedQueryContext ------------------------------------------------------------------

KqlParser::ColumnContext* KqlParser::NestedQueryContext::column() {
  return getRuleContext<KqlParser::ColumnContext>(0);
}

KqlParser::QueryContext* KqlParser::NestedQueryContext::query() {
  return getRuleContext<KqlParser::QueryContext>(0);
}

KqlParser::NestedQueryContext::NestedQueryContext(QueryContext *ctx) { copyFrom(ctx); }


std::any KqlParser::NestedQueryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitNestedQuery(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NotQueryContext ------------------------------------------------------------------

tree::TerminalNode* KqlParser::NotQueryContext::NOT() {
  return getToken(KqlParser::NOT, 0);
}

KqlParser::QueryContext* KqlParser::NotQueryContext::query() {
  return getRuleContext<KqlParser::QueryContext>(0);
}

KqlParser::NotQueryContext::NotQueryContext(QueryContext *ctx) { copyFrom(ctx); }


std::any KqlParser::NotQueryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitNotQuery(this);
  else
    return visitor->visitChildren(this);
}
//----------------- SubQueryContext ------------------------------------------------------------------

KqlParser::QueryContext* KqlParser::SubQueryContext::query() {
  return getRuleContext<KqlParser::QueryContext>(0);
}

KqlParser::SubQueryContext::SubQueryContext(QueryContext *ctx) { copyFrom(ctx); }


std::any KqlParser::SubQueryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitSubQuery(this);
  else
    return visitor->visitChildren(this);
}
//----------------- OrAndQueryContext ------------------------------------------------------------------

std::vector<KqlParser::QueryContext *> KqlParser::OrAndQueryContext::query() {
  return getRuleContexts<KqlParser::QueryContext>();
}

KqlParser::QueryContext* KqlParser::OrAndQueryContext::query(size_t i) {
  return getRuleContext<KqlParser::QueryContext>(i);
}

tree::TerminalNode* KqlParser::OrAndQueryContext::OR() {
  return getToken(KqlParser::OR, 0);
}

tree::TerminalNode* KqlParser::OrAndQueryContext::AND() {
  return getToken(KqlParser::AND, 0);
}

KqlParser::OrAndQueryContext::OrAndQueryContext(QueryContext *ctx) { copyFrom(ctx); }


std::any KqlParser::OrAndQueryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitOrAndQuery(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::QueryContext* KqlParser::query() {
   return query(0);
}

KqlParser::QueryContext* KqlParser::query(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  KqlParser::QueryContext *_localctx = _tracker.createInstance<QueryContext>(_ctx, parentState);
  KqlParser::QueryContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 2;
  enterRecursionRule(_localctx, 2, KqlParser::RuleQuery, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(43);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<NestedQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(30);
      antlrcpp::downCast<NestedQueryContext *>(_localctx)->col = column();
      setState(31);
      match(KqlParser::T__0);
      setState(32);
      match(KqlParser::T__1);
      setState(33);
      antlrcpp::downCast<NestedQueryContext *>(_localctx)->q = query(0);
      setState(34);
      match(KqlParser::T__2);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SubQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(36);
      match(KqlParser::T__3);
      setState(37);
      antlrcpp::downCast<SubQueryContext *>(_localctx)->q = query(0);
      setState(38);
      match(KqlParser::T__4);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<NotQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(40);
      match(KqlParser::NOT);
      setState(41);
      antlrcpp::downCast<NotQueryContext *>(_localctx)->q = query(3);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(42);
      expression();
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(50);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        auto newContext = _tracker.createInstance<OrAndQueryContext>(_tracker.createInstance<QueryContext>(parentContext, parentState));
        _localctx = newContext;
        newContext->lhs = previousContext;
        pushNewRecursionContext(newContext, startState, RuleQuery);
        setState(45);

        if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
        setState(46);
        antlrcpp::downCast<OrAndQueryContext *>(_localctx)->op = _input->LT(1);
        _la = _input->LA(1);
        if (!(_la == KqlParser::AND

        || _la == KqlParser::OR)) {
          antlrcpp::downCast<OrAndQueryContext *>(_localctx)->op = _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(47);
        antlrcpp::downCast<OrAndQueryContext *>(_localctx)->rhs = query(3); 
      }
      setState(52);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

KqlParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

KqlParser::Column_range_expressionContext* KqlParser::ExpressionContext::column_range_expression() {
  return getRuleContext<KqlParser::Column_range_expressionContext>(0);
}

KqlParser::Column_value_expressionContext* KqlParser::ExpressionContext::column_value_expression() {
  return getRuleContext<KqlParser::Column_value_expressionContext>(0);
}

KqlParser::Value_expressionContext* KqlParser::ExpressionContext::value_expression() {
  return getRuleContext<KqlParser::Value_expressionContext>(0);
}


size_t KqlParser::ExpressionContext::getRuleIndex() const {
  return KqlParser::RuleExpression;
}


std::any KqlParser::ExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitExpression(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::ExpressionContext* KqlParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 4, KqlParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(56);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(53);
      column_range_expression();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(54);
      column_value_expression();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(55);
      value_expression();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Column_range_expressionContext ------------------------------------------------------------------

KqlParser::Column_range_expressionContext::Column_range_expressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* KqlParser::Column_range_expressionContext::RANGE_OPERATOR() {
  return getToken(KqlParser::RANGE_OPERATOR, 0);
}

KqlParser::ColumnContext* KqlParser::Column_range_expressionContext::column() {
  return getRuleContext<KqlParser::ColumnContext>(0);
}

KqlParser::Timestamp_expressionContext* KqlParser::Column_range_expressionContext::timestamp_expression() {
  return getRuleContext<KqlParser::Timestamp_expressionContext>(0);
}

KqlParser::LiteralContext* KqlParser::Column_range_expressionContext::literal() {
  return getRuleContext<KqlParser::LiteralContext>(0);
}


size_t KqlParser::Column_range_expressionContext::getRuleIndex() const {
  return KqlParser::RuleColumn_range_expression;
}


std::any KqlParser::Column_range_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitColumn_range_expression(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::Column_range_expressionContext* KqlParser::column_range_expression() {
  Column_range_expressionContext *_localctx = _tracker.createInstance<Column_range_expressionContext>(_ctx, getState());
  enterRule(_localctx, 6, KqlParser::RuleColumn_range_expression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(58);
    antlrcpp::downCast<Column_range_expressionContext *>(_localctx)->col = column();
    setState(59);
    match(KqlParser::RANGE_OPERATOR);
    setState(62);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case KqlParser::T__5: {
        setState(60);
        antlrcpp::downCast<Column_range_expressionContext *>(_localctx)->timestamp = timestamp_expression();
        break;
      }

      case KqlParser::QUOTED_STRING:
      case KqlParser::UNQUOTED_LITERAL: {
        setState(61);
        antlrcpp::downCast<Column_range_expressionContext *>(_localctx)->lit = literal();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Column_value_expressionContext ------------------------------------------------------------------

KqlParser::Column_value_expressionContext::Column_value_expressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

KqlParser::ColumnContext* KqlParser::Column_value_expressionContext::column() {
  return getRuleContext<KqlParser::ColumnContext>(0);
}

KqlParser::List_of_valuesContext* KqlParser::Column_value_expressionContext::list_of_values() {
  return getRuleContext<KqlParser::List_of_valuesContext>(0);
}

KqlParser::Timestamp_expressionContext* KqlParser::Column_value_expressionContext::timestamp_expression() {
  return getRuleContext<KqlParser::Timestamp_expressionContext>(0);
}

KqlParser::LiteralContext* KqlParser::Column_value_expressionContext::literal() {
  return getRuleContext<KqlParser::LiteralContext>(0);
}


size_t KqlParser::Column_value_expressionContext::getRuleIndex() const {
  return KqlParser::RuleColumn_value_expression;
}


std::any KqlParser::Column_value_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitColumn_value_expression(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::Column_value_expressionContext* KqlParser::column_value_expression() {
  Column_value_expressionContext *_localctx = _tracker.createInstance<Column_value_expressionContext>(_ctx, getState());
  enterRule(_localctx, 8, KqlParser::RuleColumn_value_expression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(64);
    antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->col = column();
    setState(65);
    match(KqlParser::T__0);
    setState(69);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case KqlParser::T__3: {
        setState(66);
        antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->list = list_of_values();
        break;
      }

      case KqlParser::T__5: {
        setState(67);
        antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->timestamp = timestamp_expression();
        break;
      }

      case KqlParser::QUOTED_STRING:
      case KqlParser::UNQUOTED_LITERAL: {
        setState(68);
        antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->lit = literal();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Function_callContext ------------------------------------------------------------------

KqlParser::Function_callContext::Function_callContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

KqlParser::Column_literalContext* KqlParser::Function_callContext::column_literal() {
  return getRuleContext<KqlParser::Column_literalContext>(0);
}

tree::TerminalNode* KqlParser::Function_callContext::UNQUOTED_LITERAL() {
  return getToken(KqlParser::UNQUOTED_LITERAL, 0);
}


size_t KqlParser::Function_callContext::getRuleIndex() const {
  return KqlParser::RuleFunction_call;
}


std::any KqlParser::Function_callContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitFunction_call(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::Function_callContext* KqlParser::function_call() {
  Function_callContext *_localctx = _tracker.createInstance<Function_callContext>(_ctx, getState());
  enterRule(_localctx, 10, KqlParser::RuleFunction_call);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(71);
    antlrcpp::downCast<Function_callContext *>(_localctx)->func = match(KqlParser::UNQUOTED_LITERAL);
    setState(72);
    match(KqlParser::T__3);
    setState(73);
    column_literal();
    setState(74);
    match(KqlParser::T__4);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ColumnContext ------------------------------------------------------------------

KqlParser::ColumnContext::ColumnContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

KqlParser::Function_callContext* KqlParser::ColumnContext::function_call() {
  return getRuleContext<KqlParser::Function_callContext>(0);
}

KqlParser::LiteralContext* KqlParser::ColumnContext::literal() {
  return getRuleContext<KqlParser::LiteralContext>(0);
}


size_t KqlParser::ColumnContext::getRuleIndex() const {
  return KqlParser::RuleColumn;
}


std::any KqlParser::ColumnContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitColumn(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::ColumnContext* KqlParser::column() {
  ColumnContext *_localctx = _tracker.createInstance<ColumnContext>(_ctx, getState());
  enterRule(_localctx, 12, KqlParser::RuleColumn);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(78);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 5, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(76);
      function_call();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(77);
      literal();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Column_literalContext ------------------------------------------------------------------

KqlParser::Column_literalContext::Column_literalContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

KqlParser::LiteralContext* KqlParser::Column_literalContext::literal() {
  return getRuleContext<KqlParser::LiteralContext>(0);
}


size_t KqlParser::Column_literalContext::getRuleIndex() const {
  return KqlParser::RuleColumn_literal;
}


std::any KqlParser::Column_literalContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitColumn_literal(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::Column_literalContext* KqlParser::column_literal() {
  Column_literalContext *_localctx = _tracker.createInstance<Column_literalContext>(_ctx, getState());
  enterRule(_localctx, 14, KqlParser::RuleColumn_literal);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(80);
    literal();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Projection_columnContext ------------------------------------------------------------------

KqlParser::Projection_columnContext::Projection_columnContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

KqlParser::Function_callContext* KqlParser::Projection_columnContext::function_call() {
  return getRuleContext<KqlParser::Function_callContext>(0);
}

KqlParser::LiteralContext* KqlParser::Projection_columnContext::literal() {
  return getRuleContext<KqlParser::LiteralContext>(0);
}


size_t KqlParser::Projection_columnContext::getRuleIndex() const {
  return KqlParser::RuleProjection_column;
}


std::any KqlParser::Projection_columnContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitProjection_column(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::Projection_columnContext* KqlParser::projection_column() {
  Projection_columnContext *_localctx = _tracker.createInstance<Projection_columnContext>(_ctx, getState());
  enterRule(_localctx, 16, KqlParser::RuleProjection_column);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(84);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(82);
      function_call();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(83);
      literal();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Value_expressionContext ------------------------------------------------------------------

KqlParser::Value_expressionContext::Value_expressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

KqlParser::LiteralContext* KqlParser::Value_expressionContext::literal() {
  return getRuleContext<KqlParser::LiteralContext>(0);
}


size_t KqlParser::Value_expressionContext::getRuleIndex() const {
  return KqlParser::RuleValue_expression;
}


std::any KqlParser::Value_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitValue_expression(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::Value_expressionContext* KqlParser::value_expression() {
  Value_expressionContext *_localctx = _tracker.createInstance<Value_expressionContext>(_ctx, getState());
  enterRule(_localctx, 18, KqlParser::RuleValue_expression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(86);
    literal();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- List_of_valuesContext ------------------------------------------------------------------

KqlParser::List_of_valuesContext::List_of_valuesContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<KqlParser::LiteralContext *> KqlParser::List_of_valuesContext::literal() {
  return getRuleContexts<KqlParser::LiteralContext>();
}

KqlParser::LiteralContext* KqlParser::List_of_valuesContext::literal(size_t i) {
  return getRuleContext<KqlParser::LiteralContext>(i);
}

tree::TerminalNode* KqlParser::List_of_valuesContext::AND() {
  return getToken(KqlParser::AND, 0);
}

tree::TerminalNode* KqlParser::List_of_valuesContext::OR() {
  return getToken(KqlParser::OR, 0);
}

tree::TerminalNode* KqlParser::List_of_valuesContext::NOT() {
  return getToken(KqlParser::NOT, 0);
}


size_t KqlParser::List_of_valuesContext::getRuleIndex() const {
  return KqlParser::RuleList_of_values;
}


std::any KqlParser::List_of_valuesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitList_of_values(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::List_of_valuesContext* KqlParser::list_of_values() {
  List_of_valuesContext *_localctx = _tracker.createInstance<List_of_valuesContext>(_ctx, getState());
  enterRule(_localctx, 20, KqlParser::RuleList_of_values);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(88);
    match(KqlParser::T__3);
    setState(90);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1792) != 0)) {
      setState(89);
      antlrcpp::downCast<List_of_valuesContext *>(_localctx)->condition = _input->LT(1);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1792) != 0))) {
        antlrcpp::downCast<List_of_valuesContext *>(_localctx)->condition = _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
    }
    setState(95);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == KqlParser::QUOTED_STRING

    || _la == KqlParser::UNQUOTED_LITERAL) {
      setState(92);
      antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literalContext = literal();
      antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literals.push_back(antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literalContext);
      setState(97);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(98);
    match(KqlParser::T__4);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Timestamp_expressionContext ------------------------------------------------------------------

KqlParser::Timestamp_expressionContext::Timestamp_expressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> KqlParser::Timestamp_expressionContext::QUOTED_STRING() {
  return getTokens(KqlParser::QUOTED_STRING);
}

tree::TerminalNode* KqlParser::Timestamp_expressionContext::QUOTED_STRING(size_t i) {
  return getToken(KqlParser::QUOTED_STRING, i);
}


size_t KqlParser::Timestamp_expressionContext::getRuleIndex() const {
  return KqlParser::RuleTimestamp_expression;
}


std::any KqlParser::Timestamp_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitTimestamp_expression(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::Timestamp_expressionContext* KqlParser::timestamp_expression() {
  Timestamp_expressionContext *_localctx = _tracker.createInstance<Timestamp_expressionContext>(_ctx, getState());
  enterRule(_localctx, 22, KqlParser::RuleTimestamp_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(100);
    match(KqlParser::T__5);
    setState(101);
    antlrcpp::downCast<Timestamp_expressionContext *>(_localctx)->timestamp = match(KqlParser::QUOTED_STRING);
    setState(104);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == KqlParser::T__6) {
      setState(102);
      match(KqlParser::T__6);
      setState(103);
      antlrcpp::downCast<Timestamp_expressionContext *>(_localctx)->pattern = match(KqlParser::QUOTED_STRING);
    }
    setState(106);
    match(KqlParser::T__4);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LiteralContext ------------------------------------------------------------------

KqlParser::LiteralContext::LiteralContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* KqlParser::LiteralContext::QUOTED_STRING() {
  return getToken(KqlParser::QUOTED_STRING, 0);
}

tree::TerminalNode* KqlParser::LiteralContext::UNQUOTED_LITERAL() {
  return getToken(KqlParser::UNQUOTED_LITERAL, 0);
}


size_t KqlParser::LiteralContext::getRuleIndex() const {
  return KqlParser::RuleLiteral;
}


std::any KqlParser::LiteralContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<KqlVisitor*>(visitor))
    return parserVisitor->visitLiteral(this);
  else
    return visitor->visitChildren(this);
}

KqlParser::LiteralContext* KqlParser::literal() {
  LiteralContext *_localctx = _tracker.createInstance<LiteralContext>(_ctx, getState());
  enterRule(_localctx, 24, KqlParser::RuleLiteral);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(108);
    _la = _input->LA(1);
    if (!(_la == KqlParser::QUOTED_STRING

    || _la == KqlParser::UNQUOTED_LITERAL)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool KqlParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 1: return querySempred(antlrcpp::downCast<QueryContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool KqlParser::querySempred(QueryContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

void KqlParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  kqlParserInitialize();
#else
  ::antlr4::internal::call_once(kqlParserOnceFlag, kqlParserInitialize);
#endif
}
