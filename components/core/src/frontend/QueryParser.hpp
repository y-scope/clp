#ifndef QUERYPARSER_HPP
#define QUERYPARSER_HPP

// Boost libraries
#include <boost/filesystem/path.hpp>

#include "LALR1Parser.hpp"
#include "SchemaParser.hpp"

class QueryParser : public LALR1Parser {
public:
    QueryParser(std::unique_ptr<ParserASTSchemaFile> schema_ast);

private:
    void add_tokens();
    void add_productions(std::unique_ptr<ParserASTSchemaFile> const& schema_ast);
};

#endif // QUERYPARSER_HPP
