#ifndef COMPRESSOR_FRONTEND_LOGPARSER_HPP
#define COMPRESSOR_FRONTEND_LOGPARSER_HPP

// C++ standard libraries
#include <cassert>
#include <iostream>

// Boost libraries
#include <boost/filesystem/path.hpp>

// Project headers
#include "../Stopwatch.hpp"
#include "LALR1Parser.hpp"
#include "SchemaParser.hpp"

namespace compressor_frontend {
    /// TODO: try not inheriting from LALR1Parser (and compare c-array vs. vectors (its underlying array) for buffers afterwards)
    class LogParser : public LALR1Parser {
    public:
        LogParser (const std::string& schema_file_path);

        void parse (ReaderInterface& reader);

        void increment_uncompressed_msg_pos (ReaderInterface& reader);

        Token* m_timestamp_token_ptr;
        uint16_t m_schema_checksum;
        uint16_t m_schema_file_size;

    private:
        Token get_next_symbol ();

        void add_delimiters (const std::unique_ptr<ParserAST>& delimiters);

        void add_rules (const std::unique_ptr<SchemaFileAST>& schema_ast);

        Token* m_active_uncompressed_msg;
        uint32_t m_uncompressed_msg_size;
        Token m_static_uncompressed_msg[cStaticByteBuffSize];
        uint32_t m_uncompressed_msg_pos = 0;

    };
}

#endif // COMPRESSOR_FRONTEND_LOGPARSER_HPP
