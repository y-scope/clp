
// Generated from clp_s/search/sql/Sql.g4 by ANTLR 4.13.2


#include "SqlVisitor.h"

#include "SqlParser.h"


using namespace antlrcpp;
using namespace clp_s::search::sql::generated;

using namespace antlr4;

namespace {

struct SqlParserStaticData final {
  SqlParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SqlParserStaticData(const SqlParserStaticData&) = delete;
  SqlParserStaticData(SqlParserStaticData&&) = delete;
  SqlParserStaticData& operator=(const SqlParserStaticData&) = delete;
  SqlParserStaticData& operator=(SqlParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag sqlParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<SqlParserStaticData> sqlParserStaticData = nullptr;

void sqlParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (sqlParserStaticData != nullptr) {
    return;
  }
#else
  assert(sqlParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<SqlParserStaticData>(
    std::vector<std::string>{
      "start"
    },
    std::vector<std::string>{
    },
    std::vector<std::string>{
      "", "SPACE"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,1,5,2,0,7,0,1,0,1,0,1,0,0,0,1,0,0,0,3,0,2,1,0,0,0,2,3,5,0,0,1,3,1,
  	1,0,0,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  sqlParserStaticData = std::move(staticData);
}

}

SqlParser::SqlParser(TokenStream *input) : SqlParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

SqlParser::SqlParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  SqlParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *sqlParserStaticData->atn, sqlParserStaticData->decisionToDFA, sqlParserStaticData->sharedContextCache, options);
}

SqlParser::~SqlParser() {
  delete _interpreter;
}

const atn::ATN& SqlParser::getATN() const {
  return *sqlParserStaticData->atn;
}

std::string SqlParser::getGrammarFileName() const {
  return "Sql.g4";
}

const std::vector<std::string>& SqlParser::getRuleNames() const {
  return sqlParserStaticData->ruleNames;
}

const dfa::Vocabulary& SqlParser::getVocabulary() const {
  return sqlParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SqlParser::getSerializedATN() const {
  return sqlParserStaticData->serializedATN;
}


//----------------- StartContext ------------------------------------------------------------------

SqlParser::StartContext::StartContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SqlParser::StartContext::EOF() {
  return getToken(SqlParser::EOF, 0);
}


size_t SqlParser::StartContext::getRuleIndex() const {
  return SqlParser::RuleStart;
}


std::any SqlParser::StartContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SqlVisitor*>(visitor))
    return parserVisitor->visitStart(this);
  else
    return visitor->visitChildren(this);
}

SqlParser::StartContext* SqlParser::start() {
  StartContext *_localctx = _tracker.createInstance<StartContext>(_ctx, getState());
  enterRule(_localctx, 0, SqlParser::RuleStart);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(2);
    match(SqlParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void SqlParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  sqlParserInitialize();
#else
  ::antlr4::internal::call_once(sqlParserOnceFlag, sqlParserInitialize);
#endif
}
