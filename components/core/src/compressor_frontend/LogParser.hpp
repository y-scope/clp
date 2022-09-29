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

    using finite_automata::RegexDFAByteState;
    using finite_automata::RegexNFAByteState;

    /// TODO: try not inheriting from LALR1Parser (and compare c-array vs. vectors (its underlying array) for buffers afterwards)
    class LogParser : public LALR1Parser<RegexNFAByteState, RegexDFAByteState> {
    public:
        // Constructor
        LogParser (const std::string& schema_file_path);

        /**
         * /// TODO: this description will need to change after adding it directly into the dictionary writer
         * Custom parsing for the log that builds up an uncompressed message and then compresses it all at once
         * @param reader
         */
        void parse (ReaderInterface& reader);

        /**
         * Increment uncompressed message pos, considering swapping to a dynamic buffer (or doubling its size) when the current buffer size is reached
         * @param reader
         */
        void increment_uncompressed_msg_pos (ReaderInterface& reader);

    private:
        /**
         * Request the next symbol from the lexer
         * @return Token
         */
        Token get_next_symbol ();

        /**
         * Add delimiters (originally from the schema AST from the user defined schema) to the log parser
         * @param delimiters
         */
        void add_delimiters (const std::unique_ptr<ParserAST>& delimiters);

        /**
         * Add log lexing rules (directly from the schema AST from the user defined schema) to the log lexer
         * Add delimiters to the start of regex formats if delimiters are specified in user defined schema
         * Timestamps aren't matched mid log message as a variable (as they can contain delimiters, which will break search)
         * Variables other than timestamps cannot have delimiters
         * @param schema_ast
         */
        void add_rules (const std::unique_ptr<SchemaFileAST>& schema_ast);

        Token* m_active_uncompressed_msg;
        uint32_t m_uncompressed_msg_size;
        Token m_static_uncompressed_msg[cStaticByteBuffSize];
        uint32_t m_uncompressed_msg_pos = 0;

    };
}

#endif // COMPRESSOR_FRONTEND_LOGPARSER_HPP
