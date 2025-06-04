
// Generated from clp_s/search/sql/Sql.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "SqlParser.h"


namespace clp_s::search::sql::generated {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by SqlParser.
 */
class  SqlVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SqlParser.
   */
    virtual std::any visitStart(SqlParser::StartContext *context) = 0;


};

}  // namespace clp_s::search::sql::generated
