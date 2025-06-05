
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
      "column", "value_expression", "list_of_values"
    },
    std::vector<std::string>{
      "", "':'", "'{'", "'}'", "'('", "')'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "AND", "OR", "NOT", "DATE_LITERAL", "LITERAL", 
      "QUOTED_STRING", "UNQUOTED_LITERAL", "RANGE_OPERATOR", "SPACE"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,14,78,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,7,
  	7,7,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  	1,3,1,34,8,1,1,1,1,1,1,1,5,1,39,8,1,10,1,12,1,42,9,1,1,2,1,2,1,2,3,2,
  	47,8,2,1,3,1,3,1,3,1,3,3,3,53,8,3,1,4,1,4,1,4,1,4,1,4,3,4,60,8,4,1,5,
  	1,5,1,6,1,6,1,7,1,7,3,7,68,8,7,1,7,5,7,71,8,7,10,7,12,7,74,9,7,1,7,1,
  	7,1,7,0,1,2,8,0,2,4,6,8,10,12,14,0,2,1,0,6,7,1,0,6,8,80,0,16,1,0,0,0,
  	2,33,1,0,0,0,4,46,1,0,0,0,6,48,1,0,0,0,8,54,1,0,0,0,10,61,1,0,0,0,12,
  	63,1,0,0,0,14,65,1,0,0,0,16,17,3,2,1,0,17,18,5,0,0,1,18,1,1,0,0,0,19,
  	20,6,1,-1,0,20,21,3,10,5,0,21,22,5,1,0,0,22,23,5,2,0,0,23,24,3,2,1,0,
  	24,25,5,3,0,0,25,34,1,0,0,0,26,27,5,4,0,0,27,28,3,2,1,0,28,29,5,5,0,0,
  	29,34,1,0,0,0,30,31,5,8,0,0,31,34,3,2,1,3,32,34,3,4,2,0,33,19,1,0,0,0,
  	33,26,1,0,0,0,33,30,1,0,0,0,33,32,1,0,0,0,34,40,1,0,0,0,35,36,10,2,0,
  	0,36,37,7,0,0,0,37,39,3,2,1,3,38,35,1,0,0,0,39,42,1,0,0,0,40,38,1,0,0,
  	0,40,41,1,0,0,0,41,3,1,0,0,0,42,40,1,0,0,0,43,47,3,6,3,0,44,47,3,8,4,
  	0,45,47,3,12,6,0,46,43,1,0,0,0,46,44,1,0,0,0,46,45,1,0,0,0,47,5,1,0,0,
  	0,48,49,3,10,5,0,49,52,5,13,0,0,50,53,5,9,0,0,51,53,5,10,0,0,52,50,1,
  	0,0,0,52,51,1,0,0,0,53,7,1,0,0,0,54,55,3,10,5,0,55,59,5,1,0,0,56,60,3,
  	14,7,0,57,60,5,9,0,0,58,60,5,10,0,0,59,56,1,0,0,0,59,57,1,0,0,0,59,58,
  	1,0,0,0,60,9,1,0,0,0,61,62,5,10,0,0,62,11,1,0,0,0,63,64,5,10,0,0,64,13,
  	1,0,0,0,65,67,5,4,0,0,66,68,7,1,0,0,67,66,1,0,0,0,67,68,1,0,0,0,68,72,
  	1,0,0,0,69,71,5,10,0,0,70,69,1,0,0,0,71,74,1,0,0,0,72,70,1,0,0,0,72,73,
  	1,0,0,0,73,75,1,0,0,0,74,72,1,0,0,0,75,76,5,5,0,0,76,15,1,0,0,0,7,33,
  	40,46,52,59,67,72
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
    setState(16);
    query(0);
    setState(17);
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
    setState(33);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<NestedQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(20);
      antlrcpp::downCast<NestedQueryContext *>(_localctx)->col = column();
      setState(21);
      match(KqlParser::T__0);
      setState(22);
      match(KqlParser::T__1);
      setState(23);
      antlrcpp::downCast<NestedQueryContext *>(_localctx)->q = query(0);
      setState(24);
      match(KqlParser::T__2);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SubQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(26);
      match(KqlParser::T__3);
      setState(27);
      antlrcpp::downCast<SubQueryContext *>(_localctx)->q = query(0);
      setState(28);
      match(KqlParser::T__4);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<NotQueryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(30);
      match(KqlParser::NOT);
      setState(31);
      antlrcpp::downCast<NotQueryContext *>(_localctx)->q = query(3);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(32);
      expression();
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(40);
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
        setState(35);

        if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
        setState(36);
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
        setState(37);
        antlrcpp::downCast<OrAndQueryContext *>(_localctx)->rhs = query(3); 
      }
      setState(42);
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
    setState(46);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(43);
      column_range_expression();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(44);
      column_value_expression();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(45);
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

tree::TerminalNode* KqlParser::Column_range_expressionContext::DATE_LITERAL() {
  return getToken(KqlParser::DATE_LITERAL, 0);
}

tree::TerminalNode* KqlParser::Column_range_expressionContext::LITERAL() {
  return getToken(KqlParser::LITERAL, 0);
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
    setState(48);
    antlrcpp::downCast<Column_range_expressionContext *>(_localctx)->col = column();
    setState(49);
    match(KqlParser::RANGE_OPERATOR);
    setState(52);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case KqlParser::DATE_LITERAL: {
        setState(50);
        antlrcpp::downCast<Column_range_expressionContext *>(_localctx)->date_lit = match(KqlParser::DATE_LITERAL);
        break;
      }

      case KqlParser::LITERAL: {
        setState(51);
        antlrcpp::downCast<Column_range_expressionContext *>(_localctx)->lit = match(KqlParser::LITERAL);
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

tree::TerminalNode* KqlParser::Column_value_expressionContext::DATE_LITERAL() {
  return getToken(KqlParser::DATE_LITERAL, 0);
}

tree::TerminalNode* KqlParser::Column_value_expressionContext::LITERAL() {
  return getToken(KqlParser::LITERAL, 0);
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
    setState(54);
    antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->col = column();
    setState(55);
    match(KqlParser::T__0);
    setState(59);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case KqlParser::T__3: {
        setState(56);
        antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->list = list_of_values();
        break;
      }

      case KqlParser::DATE_LITERAL: {
        setState(57);
        antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->date_lit = match(KqlParser::DATE_LITERAL);
        break;
      }

      case KqlParser::LITERAL: {
        setState(58);
        antlrcpp::downCast<Column_value_expressionContext *>(_localctx)->lit = match(KqlParser::LITERAL);
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

tree::TerminalNode* KqlParser::ColumnContext::LITERAL() {
  return getToken(KqlParser::LITERAL, 0);
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
    setState(61);
    match(KqlParser::LITERAL);
   
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

tree::TerminalNode* KqlParser::Value_expressionContext::LITERAL() {
  return getToken(KqlParser::LITERAL, 0);
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
    setState(63);
    match(KqlParser::LITERAL);
   
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

std::vector<tree::TerminalNode *> KqlParser::List_of_valuesContext::LITERAL() {
  return getTokens(KqlParser::LITERAL);
}

tree::TerminalNode* KqlParser::List_of_valuesContext::LITERAL(size_t i) {
  return getToken(KqlParser::LITERAL, i);
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
    setState(65);
    match(KqlParser::T__3);
    setState(67);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 448) != 0)) {
      setState(66);
      antlrcpp::downCast<List_of_valuesContext *>(_localctx)->condition = _input->LT(1);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 448) != 0))) {
        antlrcpp::downCast<List_of_valuesContext *>(_localctx)->condition = _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
    }
    setState(72);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == KqlParser::LITERAL) {
      setState(69);
      antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literalToken = match(KqlParser::LITERAL);
      antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literals.push_back(antlrcpp::downCast<List_of_valuesContext *>(_localctx)->literalToken);
      setState(74);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(75);
    match(KqlParser::T__4);
   
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
