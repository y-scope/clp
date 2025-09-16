
// Generated from clp_s/search/sql/Sql.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "SqlVisitor.h"


namespace clp_s::search::sql::generated {

/**
 * This class provides an empty implementation of SqlVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  SqlBaseVisitor : public SqlVisitor {
public:

  virtual std::any visitStart(SqlParser::StartContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace clp_s::search::sql::generated
