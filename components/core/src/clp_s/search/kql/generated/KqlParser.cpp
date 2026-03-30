
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
      "column", "value_expression", "list_of_values", "timestamp_expression", 
      "literal"
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
  	4,1,14,92,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,7,
  	7,7,2,8,7,8,2,9,7,9,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  	1,1,1,1,1,1,1,1,1,3,1,38,8,1,1,1,1,1,1,1,5,1,43,8,1,10,1,12,1,46,9,1,
  	1,2,1,2,1,2,3,2,51,8,2,1,3,1,3,1,3,1,3,3,3,57,8,3,1,4,1,4,1,4,1,4,1,4,
  	3,4,64,8,4,1,5,1,5,1,6,1,6,1,7,1,7,3,7,72,8,7,1,7,5,7,75,8,7,10,7,12,
  	7,78,9,7,1,7,1,7,1,8,1,8,1,8,1,8,3,8,86,8,8,1,8,1,8,1,9,1,9,1,9,0,1,2,
  	10,0,2,4,6,8,10,12,14,16,18,0,3,1,0,8,9,1,0,8,10,1,0,11,12,93,0,20,1,
  	0,0,0,2,37,1,0,0,0,4,50,1,0,0,0,6,52,1,0,0,0,8,58,1,0,0,0,10,65,1,0,0,
  	0,12,67,1,0,0,0,14,69,1,0,0,0,16,81,1,0,0,0,18,89,1,0,0,0,20,21,3,2,1,
  	0,21,22,5,0,0,1,22,1,1,0,0,0,23,24,6,1,-1,0,24,25,3,10,5,0,25,26,5,1,
  	0,0,26,27,5,2,0,0,27,28,3,2,1,0,28,29,5,3,0,0,29,38,1,0,0,0,30,31,5,4,
  	0,0,31,32,3,2,1,0,32,33,5,5,0,0,33,38,1,0,0,0,34,35,5,10,0,0,35,38,3,
  	2,1,3,36,38,3,4,2,0,37,23,1,0,0,0,37,30,1,0,0,0,37,34,1,0,0,0,37,36,1,
  	0,0,0,38,44,1,0,0,0,39,40,10,2,0,0,40,41,7,0,0,0,41,43,3,2,1,3,42,39,
  	1,0,0,0,43,46,1,0,0,0,44,42,1,0,0,0,44,45,1,0,0,0,45,3,1,0,0,0,46,44,
  	1,0,0,0,47,51,3,6,3,0,48,51,3,8,4,0,49,51,3,12,6,0,50,47,1,0,0,0,50,48,
  	1,0,0,0,50,49,1,0,0,0,51,5,1,0,0,0,52,53,3,10,5,0,53,56,5,13,0,0,54,57,
  	3,16,8,0,55,57,3,18,9,0,56,54,1,0,0,0,56,55,1,0,0,0,57,7,1,0,0,0,58,59,
  	3,10,5,0,59,63,5,1,0,0,60,64,3,14,7,0,61,64,3,16,8,0,62,64,3,18,9,0,63,
  	60,1,0,0,0,63,61,1,0,0,0,63,62,1,0,0,0,64,9,1,0,0,0,65,66,3,18,9,0,66,
  	11,1,0,0,0,67,68,3,18,9,0,68,13,1,0,0,0,69,71,5,4,0,0,70,72,7,1,0,0,71,
  	70,1,0,0,0,71,72,1,0,0,0,72,76,1,0,0,0,73,75,3,18,9,0,74,73,1,0,0,0,75,
  	78,1,0,0,0,76,74,1,0,0,0,76,77,1,0,0,0,77,79,1,0,0,0,78,76,1,0,0,0,79,
  	80,5,5,0,0,80,15,1,0,0,0,81,82,5,6,0,0,82,85,5,11,0,0,83,84,5,7,0,0,84,
  	86,5,11,0,0,85,83,1,0,0,0,85,86,1,0,0,0,86,87,1,0,0,0,87,88,5,5,0,0,88,
  	17,1,0,0,0,89,90,7,2,0,0,90,19,1,0,0,0,8,37,44,50,56,63,71,76,85
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
    setState(20);
    query(0);
    setState(21);
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
    setState(37);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<NestedQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(24);
      antlrcpp::downCast<NestedQueryContext *>(_localctx)->col = column();
      setState(25);
      match(KqlParser::T__0);
      setState(26);
      match(KqlParser::T__1);
      setState(27);
      antlrcpp::downCast<NestedQueryContext *>(_localctx)->q = query(0);
      setState(28);
      match(KqlParser::T__2);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SubQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(30);
      match(KqlParser::T__3);
      setState(31);
      antlrcpp::downCast<SubQueryContext *>(_localctx)->q = query(0);
      setState(32);
      match(KqlParser::T__4);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<NotQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(34);
      match(KqlParser::NOT);
      setState(35);
      antlrcpp::downCast<NotQueryContext *>(_localctx)->q = query(3);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(36);
      expression();
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(44);
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
        setState(39);

        if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
        setState(40);
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
        setState(41);
        antlrcpp::downCast<OrAndQueryContext *>(_localctx)->rhs = query(3); 
      }
      setState(46);
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
    setState(50);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(47);
      column_range_expression();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(48);
      column_value_expression();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(49);
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
    setState(52);
    antlrcpp::downCast<Column_range_expressionContext *>(_localctx)->col = column();
    setState(53);
    match(KqlParser::RANGE_OPERATOR);
    setState(56);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case KqlParser::T__5: {
        setState(54);
        antlrcpp::downCast<Column_range_expressionContext *>(_localctx)->timestamp = timestamp_expression();
        break;
      }

      case KqlParser::QUOTED_STRING:
      case KqlParser::UNQUOTED_LITERAL: {
        setState(55);
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
    setState(58);
    antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->col = column();
    setState(59);
    match(KqlParser::T__0);
    setState(63);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case KqlParser::T__3: {
        setState(60);
        antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->list = list_of_values();
        break;
      }

      case KqlParser::T__5: {
        setState(61);
        antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->timestamp = timestamp_expression();
        break;
      }

      case KqlParser::QUOTED_STRING:
      case KqlParser::UNQUOTED_LITERAL: {
        setState(62);
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

//----------------- ColumnContext ------------------------------------------------------------------

KqlParser::ColumnContext::ColumnContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
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
  enterRule(_localctx, 10, KqlParser::RuleColumn);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(65);
    literal();
   
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
  enterRule(_localctx, 12, KqlParser::RuleValue_expression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(67);
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
  enterRule(_localctx, 14, KqlParser::RuleList_of_values);
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
    setState(69);
    match(KqlParser::T__3);
    setState(71);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1792) != 0)) {
      setState(70);
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
    setState(76);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == KqlParser::QUOTED_STRING

    || _la == KqlParser::UNQUOTED_LITERAL) {
      setState(73);
      antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literalContext = literal();
      antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literals.push_back(antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literalContext);
      setState(78);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(79);
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
  enterRule(_localctx, 16, KqlParser::RuleTimestamp_expression);
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
    setState(81);
    match(KqlParser::T__5);
    setState(82);
    antlrcpp::downCast<Timestamp_expressionContext *>(_localctx)->timestamp = match(KqlParser::QUOTED_STRING);
    setState(85);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == KqlParser::T__6) {
      setState(83);
      match(KqlParser::T__6);
      setState(84);
      antlrcpp::downCast<Timestamp_expressionContext *>(_localctx)->pattern = match(KqlParser::QUOTED_STRING);
    }
    setState(87);
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
  enterRule(_localctx, 18, KqlParser::RuleLiteral);
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
    setState(89);
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
