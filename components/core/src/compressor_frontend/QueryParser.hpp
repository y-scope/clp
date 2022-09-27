#ifndef COMPRESSOR_FRONTEND_QUERYPARSER_HPP
#define COMPRESSOR_FRONTEND_QUERYPARSER_HPP

// Boost libraries
#include <boost/filesystem/path.hpp>

// Project headers
#include "LALR1Parser.hpp"
#include "SchemaParser.hpp"

namespace compressor_frontend {
    /*
    class QueryParser : public LALR1Parser {
    public:
        QueryParser (std::unique_ptr<SchemaFileAST> schema_ast);

    private:
        void add_tokens ();

        void add_productions (const std::unique_ptr<SchemaFileAST>& schema_ast);
    };
     */
}

#endif // COMPRESSOR_FRONTEND_QUERYPARSER_HPP
