
// Generated from clp_s/search/kql/Kql.g4 by ANTLR 4.13.2


#include "KqlLexer.h"


using namespace antlr4;

using namespace clp_s::search::kql::generated;


using namespace antlr4;

namespace {

struct KqlLexerStaticData final {
  KqlLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  KqlLexerStaticData(const KqlLexerStaticData&) = delete;
  KqlLexerStaticData(KqlLexerStaticData&&) = delete;
  KqlLexerStaticData& operator=(const KqlLexerStaticData&) = delete;
  KqlLexerStaticData& operator=(KqlLexerStaticData&&) = delete;

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

::antlr4::internal::OnceFlag kqllexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<KqlLexerStaticData> kqllexerLexerStaticData = nullptr;

void kqllexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (kqllexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(kqllexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<KqlLexerStaticData>(
    std::vector<std::string>{
      "T__0", "T__1", "T__2", "T__3", "T__4", "AND", "OR", "NOT", "DATE_LITERAL", 
      "LITERAL", "QUOTED_STRING", "UNQUOTED_LITERAL", "QUOTED_CHARACTER", 
      "UNQUOTED_CHARACTER", "WILDCARD", "KEYWORD", "RANGE_OPERATOR", "ESCAPED_SPECIAL_CHARACTER", 
      "ESCAPED_SPACE", "SPECIAL_CHARACTER", "UNICODE", "HEXDIGIT", "SPACE"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
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
  	4,0,14,163,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
  	7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,
  	7,21,2,22,7,22,1,0,1,0,1,1,1,1,1,2,1,2,1,3,1,3,1,4,1,4,1,5,1,5,1,5,1,
  	5,1,6,1,6,1,6,1,7,1,7,1,7,1,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,4,8,77,
  	8,8,11,8,12,8,78,1,8,1,8,1,8,4,8,84,8,8,11,8,12,8,85,3,8,88,8,8,1,8,1,
  	8,1,9,1,9,3,9,94,8,9,1,10,1,10,5,10,98,8,10,10,10,12,10,101,9,10,1,10,
  	1,10,1,11,4,11,106,8,11,11,11,12,11,107,1,12,1,12,1,12,1,12,3,12,114,
  	8,12,1,13,1,13,1,13,1,13,1,13,3,13,121,8,13,1,14,1,14,1,15,1,15,1,15,
  	3,15,128,8,15,1,16,1,16,1,16,1,16,1,16,3,16,135,8,16,1,17,1,17,1,17,1,
  	18,1,18,1,18,1,18,1,18,1,18,3,18,146,8,18,1,19,1,19,1,20,1,20,1,20,1,
  	20,1,20,1,20,1,20,1,20,1,21,1,21,1,22,1,22,1,22,1,22,0,0,23,1,1,3,2,5,
  	3,7,4,9,5,11,6,13,7,15,8,17,9,19,10,21,11,23,12,25,0,27,0,29,0,31,0,33,
  	13,35,0,37,0,39,0,41,0,43,0,45,14,1,0,13,2,0,65,65,97,97,2,0,78,78,110,
  	110,2,0,68,68,100,100,2,0,79,79,111,111,2,0,82,82,114,114,2,0,84,84,116,
  	116,1,0,34,34,11,0,9,10,13,13,32,32,34,34,40,41,58,58,60,60,62,62,92,
  	92,123,123,125,125,2,0,42,42,63,63,2,0,60,60,62,62,9,0,33,36,40,42,46,
  	46,58,58,60,60,62,64,92,92,123,123,125,125,3,0,48,57,65,70,97,102,3,0,
  	9,10,13,13,32,32,171,0,1,1,0,0,0,0,3,1,0,0,0,0,5,1,0,0,0,0,7,1,0,0,0,
  	0,9,1,0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,1,0,0,0,0,17,1,0,0,0,0,19,
  	1,0,0,0,0,21,1,0,0,0,0,23,1,0,0,0,0,33,1,0,0,0,0,45,1,0,0,0,1,47,1,0,
  	0,0,3,49,1,0,0,0,5,51,1,0,0,0,7,53,1,0,0,0,9,55,1,0,0,0,11,57,1,0,0,0,
  	13,61,1,0,0,0,15,64,1,0,0,0,17,68,1,0,0,0,19,93,1,0,0,0,21,95,1,0,0,0,
  	23,105,1,0,0,0,25,113,1,0,0,0,27,120,1,0,0,0,29,122,1,0,0,0,31,127,1,
  	0,0,0,33,134,1,0,0,0,35,136,1,0,0,0,37,145,1,0,0,0,39,147,1,0,0,0,41,
  	149,1,0,0,0,43,157,1,0,0,0,45,159,1,0,0,0,47,48,5,58,0,0,48,2,1,0,0,0,
  	49,50,5,123,0,0,50,4,1,0,0,0,51,52,5,125,0,0,52,6,1,0,0,0,53,54,5,40,
  	0,0,54,8,1,0,0,0,55,56,5,41,0,0,56,10,1,0,0,0,57,58,7,0,0,0,58,59,7,1,
  	0,0,59,60,7,2,0,0,60,12,1,0,0,0,61,62,7,3,0,0,62,63,7,4,0,0,63,14,1,0,
  	0,0,64,65,7,1,0,0,65,66,7,3,0,0,66,67,7,5,0,0,67,16,1,0,0,0,68,69,5,100,
  	0,0,69,70,5,97,0,0,70,71,5,116,0,0,71,72,5,101,0,0,72,73,5,40,0,0,73,
  	87,1,0,0,0,74,76,5,34,0,0,75,77,3,25,12,0,76,75,1,0,0,0,77,78,1,0,0,0,
  	78,76,1,0,0,0,78,79,1,0,0,0,79,80,1,0,0,0,80,81,5,34,0,0,81,88,1,0,0,
  	0,82,84,3,25,12,0,83,82,1,0,0,0,84,85,1,0,0,0,85,83,1,0,0,0,85,86,1,0,
  	0,0,86,88,1,0,0,0,87,74,1,0,0,0,87,83,1,0,0,0,88,89,1,0,0,0,89,90,5,41,
  	0,0,90,18,1,0,0,0,91,94,3,21,10,0,92,94,3,23,11,0,93,91,1,0,0,0,93,92,
  	1,0,0,0,94,20,1,0,0,0,95,99,5,34,0,0,96,98,3,25,12,0,97,96,1,0,0,0,98,
  	101,1,0,0,0,99,97,1,0,0,0,99,100,1,0,0,0,100,102,1,0,0,0,101,99,1,0,0,
  	0,102,103,5,34,0,0,103,22,1,0,0,0,104,106,3,27,13,0,105,104,1,0,0,0,106,
  	107,1,0,0,0,107,105,1,0,0,0,107,108,1,0,0,0,108,24,1,0,0,0,109,114,3,
  	37,18,0,110,111,5,92,0,0,111,114,5,34,0,0,112,114,8,6,0,0,113,109,1,0,
  	0,0,113,110,1,0,0,0,113,112,1,0,0,0,114,26,1,0,0,0,115,121,3,37,18,0,
  	116,121,3,35,17,0,117,121,3,29,14,0,118,121,3,41,20,0,119,121,8,7,0,0,
  	120,115,1,0,0,0,120,116,1,0,0,0,120,117,1,0,0,0,120,118,1,0,0,0,120,119,
  	1,0,0,0,121,28,1,0,0,0,122,123,7,8,0,0,123,30,1,0,0,0,124,128,3,11,5,
  	0,125,128,3,13,6,0,126,128,3,15,7,0,127,124,1,0,0,0,127,125,1,0,0,0,127,
  	126,1,0,0,0,128,32,1,0,0,0,129,130,5,60,0,0,130,135,5,61,0,0,131,132,
  	5,62,0,0,132,135,5,61,0,0,133,135,7,9,0,0,134,129,1,0,0,0,134,131,1,0,
  	0,0,134,133,1,0,0,0,135,34,1,0,0,0,136,137,5,92,0,0,137,138,3,39,19,0,
  	138,36,1,0,0,0,139,140,5,92,0,0,140,146,5,116,0,0,141,142,5,92,0,0,142,
  	146,5,114,0,0,143,144,5,92,0,0,144,146,5,110,0,0,145,139,1,0,0,0,145,
  	141,1,0,0,0,145,143,1,0,0,0,146,38,1,0,0,0,147,148,7,10,0,0,148,40,1,
  	0,0,0,149,150,5,92,0,0,150,151,5,117,0,0,151,152,1,0,0,0,152,153,3,43,
  	21,0,153,154,3,43,21,0,154,155,3,43,21,0,155,156,3,43,21,0,156,42,1,0,
  	0,0,157,158,7,11,0,0,158,44,1,0,0,0,159,160,7,12,0,0,160,161,1,0,0,0,
  	161,162,6,22,0,0,162,46,1,0,0,0,12,0,78,85,87,93,99,107,113,120,127,134,
  	145,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  kqllexerLexerStaticData = std::move(staticData);
}

}

KqlLexer::KqlLexer(CharStream *input) : Lexer(input) {
  KqlLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *kqllexerLexerStaticData->atn, kqllexerLexerStaticData->decisionToDFA, kqllexerLexerStaticData->sharedContextCache);
}

KqlLexer::~KqlLexer() {
  delete _interpreter;
}

std::string KqlLexer::getGrammarFileName() const {
  return "Kql.g4";
}

const std::vector<std::string>& KqlLexer::getRuleNames() const {
  return kqllexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& KqlLexer::getChannelNames() const {
  return kqllexerLexerStaticData->channelNames;
}

const std::vector<std::string>& KqlLexer::getModeNames() const {
  return kqllexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& KqlLexer::getVocabulary() const {
  return kqllexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView KqlLexer::getSerializedATN() const {
  return kqllexerLexerStaticData->serializedATN;
}

const atn::ATN& KqlLexer::getATN() const {
  return *kqllexerLexerStaticData->atn;
}




void KqlLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  kqllexerLexerInitialize();
#else
  ::antlr4::internal::call_once(kqllexerLexerOnceFlag, kqllexerLexerInitialize);
#endif
}
