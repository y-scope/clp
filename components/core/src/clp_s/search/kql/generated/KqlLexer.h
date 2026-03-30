
// Generated from clp_s/search/kql/Kql.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace clp_s::search::kql::generated {


class  KqlLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    AND = 8, OR = 9, NOT = 10, QUOTED_STRING = 11, UNQUOTED_LITERAL = 12, 
    RANGE_OPERATOR = 13, SPACE = 14
  };

  explicit KqlLexer(antlr4::CharStream *input);

  ~KqlLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

}  // namespace clp_s::search::kql::generated
