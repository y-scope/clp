
// Generated from clp_s/search/sql/Sql.g4 by ANTLR 4.13.2


#include "SqlLexer.h"


using namespace antlr4;

using namespace clp_s::search::sql::generated;


using namespace antlr4;

namespace {

struct SqlLexerStaticData final {
  SqlLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SqlLexerStaticData(const SqlLexerStaticData&) = delete;
  SqlLexerStaticData(SqlLexerStaticData&&) = delete;
  SqlLexerStaticData& operator=(const SqlLexerStaticData&) = delete;
  SqlLexerStaticData& operator=(SqlLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag sqllexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<SqlLexerStaticData> sqllexerLexerStaticData = nullptr;

void sqllexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (sqllexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(sqllexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<SqlLexerStaticData>(
    std::vector<std::string>{
      "SPACE"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
    },
    std::vector<std::string>{
      "", "SPACE"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,1,7,6,-1,2,0,7,0,1,0,1,0,1,0,1,0,0,0,1,1,1,1,0,1,3,0,9,10,13,13,32,
  	32,6,0,1,1,0,0,0,1,3,1,0,0,0,3,4,7,0,0,0,4,5,1,0,0,0,5,6,6,0,0,0,6,2,
  	1,0,0,0,1,0,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  sqllexerLexerStaticData = std::move(staticData);
}

}

SqlLexer::SqlLexer(CharStream *input) : Lexer(input) {
  SqlLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *sqllexerLexerStaticData->atn, sqllexerLexerStaticData->decisionToDFA, sqllexerLexerStaticData->sharedContextCache);
}

SqlLexer::~SqlLexer() {
  delete _interpreter;
}

std::string SqlLexer::getGrammarFileName() const {
  return "Sql.g4";
}

const std::vector<std::string>& SqlLexer::getRuleNames() const {
  return sqllexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& SqlLexer::getChannelNames() const {
  return sqllexerLexerStaticData->channelNames;
}

const std::vector<std::string>& SqlLexer::getModeNames() const {
  return sqllexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& SqlLexer::getVocabulary() const {
  return sqllexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SqlLexer::getSerializedATN() const {
  return sqllexerLexerStaticData->serializedATN;
}

const atn::ATN& SqlLexer::getATN() const {
  return *sqllexerLexerStaticData->atn;
}




void SqlLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  sqllexerLexerInitialize();
#else
  ::antlr4::internal::call_once(sqllexerLexerOnceFlag, sqllexerLexerInitialize);
#endif
}
