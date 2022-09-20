#ifndef COMPRESSOR_FRONTEND_SCHEMAPARSER_HPP
#define COMPRESSOR_FRONTEND_SCHEMAPARSER_HPP

// Boost libraries
#include <boost/filesystem/path.hpp>
#include <utility>

// Project headers
#include "../ReaderInterface.hpp"
#include "LALR1Parser.hpp"

namespace compressor_frontend {
    class SchemaFileAST : public ParserAST {
    public:
        SchemaFileAST () = default;

        void add_delimiters (std::unique_ptr<ParserAST> delimiters_in) {
            this->m_delimiters = std::move(delimiters_in);
        }

        void add_schema_var (std::unique_ptr<ParserAST> schema_var) {
            m_schema_vars.push_back(std::move(schema_var));
        }

        std::vector<std::unique_ptr<ParserAST>> m_schema_vars;
        std::unique_ptr<ParserAST> m_delimiters;
        std::string m_file_path;
    };
    
    class IdentifierAST : public ParserAST {
    public:
        explicit IdentifierAST (char character) {
            m_name.push_back(character);
        }

        void add_character (char character) {
            m_name.push_back(character);
        }
        
        std::string m_name;
    };
    
    class SchemaVarAST : public ParserAST {
    public:
        SchemaVarAST (std::string name, std::unique_ptr<RegexAST> regex_ptr, uint32_t line_num) : m_name(std::move(name)), m_regex_ptr(std::move(regex_ptr))
                                                                                                  , m_line_num(line_num) {}

        uint32_t m_line_num;
        std::string m_name;
        std::unique_ptr<RegexAST> m_regex_ptr;
    };

    class DelimiterStringAST : public ParserAST {
    public:
        explicit DelimiterStringAST (uint32_t delimiter) {
            m_delimiters.push_back(delimiter);
        }

        void add_delimiter (uint32_t delimiter) {
            m_delimiters.push_back(delimiter);
        }

        std::vector<uint32_t> m_delimiters;
    };

    class SchemaParser : public LALR1Parser {
    public:
        SchemaParser ();

        std::unique_ptr<SchemaFileAST> generate_schema_ast (ReaderInterface& reader);

        static std::unique_ptr<SchemaFileAST> try_schema_file (const std::string& schema_file_path);

        std::unique_ptr<SchemaFileAST> existing_schema_file_rule (NonTerminal* m);

    private:
        void add_tokens ();

        void add_productions ();
    };
}

#endif // COMPRESSOR_FRONTEND_SCHEMAPARSER_HPP
