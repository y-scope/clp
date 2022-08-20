#ifndef SCHEMAPARSER_HPP
#define SCHEMAPARSER_HPP

// Boost libraries
#include <boost/filesystem/path.hpp>
#include <utility>

#include "../ReaderInterface.hpp"
#include "LALR1Parser.hpp"

class ParserASTSchemaVar : public ParserAST {
public:
    ParserASTSchemaVar(std::string name, std::unique_ptr<RegexAST> regex_ptr, uint32_t line_num) : name(std::move(name)), regex_ptr(std::move(regex_ptr)),
        line_num(line_num) { }
        
    uint32_t line_num;
    std::string name;
    std::unique_ptr<RegexAST> regex_ptr;
};

class DelimiterStringAST : public ParserAST{
public:
    explicit DelimiterStringAST(uint32_t delimiter) {
        delimiters.push_back(delimiter);
    }

    void add_delimiter(uint32_t delimiter) {
        delimiters.push_back(delimiter);
    }
    std::vector<uint32_t> delimiters;
};
/*
class ParserASTWhiteSpaceStar : public ParserAST { // technically not needed atm as whitespace is just ignored
public:
    ParserASTWhiteSpaceStar(int number_of_spaces) : number_of_spaces(number_of_spaces) { }
    int number_of_spaces;
};

std::unique_ptr<ParserAST> new_white_space_star_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(new ParserASTWhiteSpaceStar(0));
}

std::unique_ptr<ParserAST> existing_white_space_star_rule(NonTerminal* m) {
    ParserASTWhiteSpaceStar* r1 = dynamic_cast<ParserASTWhiteSpaceStar *>(m->nonterminal_cast(0)->value.get());
    return std::unique_ptr<ParserAST>(new ParserASTWhiteSpaceStar(r1->number_of_spaces + 1));
}
*/

class SchemaParser : public LALR1Parser {
public:
    SchemaParser();
    std::unique_ptr<ParserASTSchemaFile> generate_schema_ast(ReaderInterface* reader);
    // static std::unique_ptr<ParserASTSchemaFile> try_schema_file(std::string const& schema_file_path);
    std::unique_ptr<ParserASTSchemaFile> existing_schema_file_rule(NonTerminal* m);

private:
    void add_tokens();
    void add_productions();
};

#endif // SCHEMAPARSER_HPP
