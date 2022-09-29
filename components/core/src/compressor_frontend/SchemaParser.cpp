#include "SchemaParser.hpp"

// C++ libraries
#include <cmath>
#include <memory>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "../FileReader.hpp"
#include "Constants.hpp"
#include "finite_automata/RegexAST.hpp"
#include "LALR1Parser.hpp"
#include "Lexer.hpp"

using compressor_frontend::finite_automata::RegexAST;
using compressor_frontend::finite_automata::RegexASTCat;
using compressor_frontend::finite_automata::RegexASTGroup;
using compressor_frontend::finite_automata::RegexASTInteger;
using compressor_frontend::finite_automata::RegexASTLiteral;
using compressor_frontend::finite_automata::RegexASTMultiplication;
using compressor_frontend::finite_automata::RegexASTOr;
using std::make_unique;
using std::string;
using std::unique_ptr;

namespace compressor_frontend {
    SchemaParser::SchemaParser () {
        add_lexical_rules();
        add_productions();
        generate();
    }

    unique_ptr<SchemaFileAST> SchemaParser::generate_schema_ast (ReaderInterface& reader) {
        NonTerminal nonterminal = parse(reader);
        std::unique_ptr<SchemaFileAST> schema_file_ast(dynamic_cast<SchemaFileAST*>(nonterminal.getParserAST().release()));
        return std::move(schema_file_ast);
    }

    unique_ptr<SchemaFileAST> SchemaParser::try_schema_file (const string& schema_file_path) {
        FileReader schema_reader;
        ErrorCode error_code = schema_reader.try_open(schema_file_path);
        if (ErrorCode_Success != error_code) {
            if (ErrorCode_FileNotFound == error_code) {
                SPDLOG_ERROR("'{}' does not exist.", schema_file_path);
            } else if (ErrorCode_errno == error_code) {
                SPDLOG_ERROR("Failed to read '{}', errno={}", schema_file_path, errno);
            } else {
                SPDLOG_ERROR("Failed to read '{}', error_code={}", schema_file_path, error_code);
            }
            return nullptr;
        }

        SchemaParser sp;
        unique_ptr<SchemaFileAST> schema_ast = sp.generate_schema_ast(schema_reader);
        schema_reader.close();

        /// TODO: do the checksum stuff here (and it should not be a filereader function)
        schema_ast->m_file_path = std::filesystem::canonical(schema_reader.get_path()).string();

        return schema_ast;
    }

    static unique_ptr<IdentifierAST> new_identifier_rule (NonTerminal* m) {
        string r1 = m->token_cast(0)->get_string();
        return make_unique<IdentifierAST>(IdentifierAST(r1[0]));
    }

    static unique_ptr<ParserAST> existing_identifier_rule (NonTerminal* m) {
        unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->getParserAST();
        auto* r1_ptr = dynamic_cast<IdentifierAST*>(r1.get());
        string r2 = m->token_cast(1)->get_string();
        r1_ptr->add_character(r2[0]);
        return std::move(r1);
    }

    static unique_ptr<SchemaVarAST> schema_var_rule (NonTerminal* m) {
        auto* r2 = dynamic_cast<IdentifierAST*>(m->nonterminal_cast(1)->getParserAST().get());
        Token* colon_token = m->token_cast(2);
        auto& r4 = m->nonterminal_cast(3)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        return make_unique<SchemaVarAST>(r2->m_name, std::move(r4), colon_token->m_line);
    }

    static unique_ptr<SchemaFileAST> new_schema_file_rule (NonTerminal* m) {
        return make_unique<SchemaFileAST>();
    }

    static unique_ptr<SchemaFileAST> new_schema_file_rule_with_var (NonTerminal* m) {
        unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->getParserAST();
        unique_ptr<SchemaFileAST> schema_file_ast = make_unique<SchemaFileAST>();
        schema_file_ast->add_schema_var(std::move(r1));
        return std::move(schema_file_ast);
    }


    static unique_ptr<SchemaFileAST> new_schema_file_rule_with_delimiters (NonTerminal* m) {
        unique_ptr<ParserAST>& r1 = m->nonterminal_cast(2)->getParserAST();
        unique_ptr<SchemaFileAST> schema_file_ast = make_unique<SchemaFileAST>();
        schema_file_ast->set_delimiters(std::move(r1));
        return std::move(schema_file_ast);
    }

    static unique_ptr<SchemaFileAST> existing_schema_file_rule_with_delimiter (NonTerminal* m) {
        unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->getParserAST();
        std::unique_ptr<SchemaFileAST> schema_file_ast(dynamic_cast<SchemaFileAST*>(r1.release()));
        unique_ptr<ParserAST>& r5 = m->nonterminal_cast(4)->getParserAST();
        schema_file_ast->set_delimiters(std::move(r5));
        return std::move(schema_file_ast);
    }

    unique_ptr<SchemaFileAST> SchemaParser::existing_schema_file_rule (NonTerminal* m) {
        unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->getParserAST();
        std::unique_ptr<SchemaFileAST> schema_file_ast(dynamic_cast<SchemaFileAST*>(r1.release()));
        unique_ptr<ParserAST>& r2 = m->nonterminal_cast(2)->getParserAST();
        schema_file_ast->add_schema_var(std::move(r2));
        m_lexer.soft_reset(NonTerminal::m_next_children_start);
        return std::move(schema_file_ast);
    }

    static unique_ptr<SchemaFileAST> identity_rule_ParserASTSchemaFile (NonTerminal* m) {
        unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->getParserAST();
        std::unique_ptr<SchemaFileAST> schema_file_ast(dynamic_cast<SchemaFileAST*>(r1.release()));
        return std::move(schema_file_ast);
    }
    
    typedef ParserValue<unique_ptr<RegexAST<RegexNFAByteState>>> ParserValueRegex;

    static unique_ptr<ParserAST> regex_identity_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(
                new ParserValueRegex(std::move(m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>())));
    }

    static unique_ptr<ParserAST> regex_cat_rule (NonTerminal* m) {
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto& r2 = m->nonterminal_cast(1)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTCat(std::move(r1), std::move(r2)))));
    }

    static unique_ptr<ParserAST> regex_or_rule (NonTerminal* m) {
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto& r2 = m->nonterminal_cast(2)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTOr(std::move(r1), std::move(r2)))));
    }

    static unique_ptr<ParserAST> regex_match_zero_or_more_rule (NonTerminal* m) {
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTMultiplication(std::move(r1), 0, 0))));
    }

    static unique_ptr<ParserAST> regex_match_one_or_more_rule (NonTerminal* m) {
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTMultiplication(std::move(r1), 1, 0))));
    }

    static unique_ptr<ParserAST> regex_match_exactly_rule (NonTerminal* m) {
        auto& r3 = m->nonterminal_cast(2)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r3_ptr = dynamic_cast<RegexASTInteger<RegexNFAByteState>*>(r3.get());
        uint32_t reps = 0;
        uint32_t r3_size = r3_ptr->get_digits().size();
        for (uint32_t i = 0; i < r3_size; i++) {
            reps += r3_ptr->get_digit(i) * (uint32_t) pow(10, r3_size - i - 1);
        }
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTMultiplication(std::move(r1), reps, reps))));
    }

    static unique_ptr<ParserAST> regex_match_range_rule (NonTerminal* m) {
        auto& r3 = m->nonterminal_cast(2)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r3_ptr = dynamic_cast<RegexASTInteger<RegexNFAByteState>*>(r3.get());
        uint32_t min = 0;
        uint32_t r3_size = r3_ptr->get_digits().size();
        for (uint32_t i = 0; i < r3_size; i++) {
            min += r3_ptr->get_digit(i) * (uint32_t) pow(10, r3_size - i - 1);
        }
        auto& r5 = m->nonterminal_cast(4)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r5_ptr = dynamic_cast<RegexASTInteger<RegexNFAByteState>*>(r5.get());
        uint32_t max = 0;
        uint32_t r5_size = r5_ptr->get_digits().size();
        for (uint32_t i = 0; i < r5_size; i++) {
            max += r5_ptr->get_digit(i) * (uint32_t) pow(10, r5_size - i - 1);
        }
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTMultiplication(std::move(r1), min, max))));
    }

    static unique_ptr<ParserAST> regex_add_literal_existing_group_rule (NonTerminal* m) {
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto& r2 = m->nonterminal_cast(1)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r1_ptr = dynamic_cast<RegexASTGroup<RegexNFAByteState>*>(r1.get());
        auto* r2_ptr = dynamic_cast<RegexASTLiteral<RegexNFAByteState>*>(r2.get());
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTGroup(r1_ptr, r2_ptr))));
    }

    static unique_ptr<ParserAST> regex_add_range_existing_group_rule (NonTerminal* m) {
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto& r2 = m->nonterminal_cast(1)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r1_ptr = dynamic_cast<RegexASTGroup<RegexNFAByteState>*>(r1.get());
        auto* r2_ptr = dynamic_cast<RegexASTGroup<RegexNFAByteState>*>(r2.get());
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTGroup(r1_ptr, r2_ptr))));
    }

    static unique_ptr<ParserAST> regex_add_literal_new_group_rule (NonTerminal* m) {
        auto& r2 = m->nonterminal_cast(1)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r2_ptr = dynamic_cast<RegexASTLiteral<RegexNFAByteState>*>(r2.get());
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTGroup(r2_ptr))));
    }

    static unique_ptr<ParserAST> regex_add_range_new_group_rule (NonTerminal* m) {
        auto& r2 = m->nonterminal_cast(1)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r2_ptr = dynamic_cast<RegexASTGroup<RegexNFAByteState>*>(r2.get());
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTGroup(r2_ptr))));
    }

    static unique_ptr<ParserAST> regex_complement_incomplete_group_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(new ParserValueRegex(make_unique<RegexASTGroup<RegexNFAByteState>>()));
    }

    static unique_ptr<ParserAST> regex_range_rule (NonTerminal* m) {
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto& r2 = m->nonterminal_cast(2)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r1_ptr = dynamic_cast<RegexASTLiteral<RegexNFAByteState>*>(r1.get());
        auto* r2_ptr = dynamic_cast<RegexASTLiteral<RegexNFAByteState>*>(r2.get());
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTGroup(r1_ptr, r2_ptr))));
    }

    static unique_ptr<ParserAST> regex_middle_identity_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(
                new ParserValueRegex(std::move(m->nonterminal_cast(1)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>())));
    }

    static unique_ptr<ParserAST> regex_literal_rule (NonTerminal* m) {
        Token* token = m->token_cast(0);
        assert(token->get_string().size() == 1);
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(
                new RegexASTLiteral<RegexNFAByteState>(token->get_string()[0]))));
    }

    static unique_ptr<ParserAST> regex_cancel_literal_rule (NonTerminal* m) {
        Token* token = m->token_cast(1);
        assert(token->get_string().size() == 1);
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(
                new RegexASTLiteral<RegexNFAByteState>(token->get_string()[0]))));
    }

    static unique_ptr<ParserAST> regex_existing_integer_rule (NonTerminal* m) {
        auto& r2 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r2_ptr = dynamic_cast<RegexASTInteger<RegexNFAByteState>*>(r2.get());
        Token* token = m->token_cast(1);
        assert(token->get_string().size() == 1);
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTInteger(r2_ptr, token->get_string()[0]))));
    }

    static unique_ptr<ParserAST> regex_new_integer_rule (NonTerminal* m) {
        Token* token = m->token_cast(0);
        assert(token->get_string().size() == 1);
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(
                new RegexASTInteger<RegexNFAByteState>(token->get_string()[0]))));
    }

    static unique_ptr<ParserAST> regex_digit_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTGroup<RegexNFAByteState>('0', '9'))));
    }

    static unique_ptr<ParserAST> regex_wildcard_rule (NonTerminal* m) {
        unique_ptr<RegexASTGroup<RegexNFAByteState>> regex_wildcard = make_unique<RegexASTGroup<RegexNFAByteState>>(0, cUnicodeMax);
        regex_wildcard->set_is_wildcard_true();
        return unique_ptr<ParserAST>(new ParserValueRegex(std::move(regex_wildcard)));
    }

    static unique_ptr<ParserAST> regex_vertical_tab_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTLiteral<RegexNFAByteState>('\v'))));
    }

    static unique_ptr<ParserAST> regex_form_feed_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTLiteral<RegexNFAByteState>('\f'))));
    }

    static unique_ptr<ParserAST> regex_tab_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTLiteral<RegexNFAByteState>('\t'))));
    }

    static unique_ptr<ParserAST> regex_char_return_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTLiteral<RegexNFAByteState>('\r'))));
    }

    static unique_ptr<ParserAST> regex_newline_rule (NonTerminal* m) {
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(new RegexASTLiteral<RegexNFAByteState>('\n'))));
    }

    static unique_ptr<ParserAST> regex_white_space_rule (NonTerminal* m) {
        unique_ptr<RegexASTGroup<RegexNFAByteState>> regex_ast_group = make_unique<RegexASTGroup<RegexNFAByteState>>(RegexASTGroup<RegexNFAByteState>({' ', '\t', '\r', '\n', '\v', '\f'}));
        return unique_ptr<ParserAST>(new ParserValueRegex(unique_ptr<RegexAST<RegexNFAByteState>>(std::move(regex_ast_group))));
    }

    static unique_ptr<ParserAST> existing_delimiter_string_rule (NonTerminal* m) {
        unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->getParserAST();
        auto& r2 = m->nonterminal_cast(1)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        auto* r1_ptr = dynamic_cast<DelimiterStringAST*>(r1.get());
        uint32_t character = dynamic_cast<RegexASTLiteral<RegexNFAByteState>*>(r2.get())->get_character();
        r1_ptr->add_delimiter(character);
        return std::move(r1);
    }

    static unique_ptr<ParserAST> new_delimiter_string_rule (NonTerminal* m) {
        auto& r1 = m->nonterminal_cast(0)->getParserAST()->get<unique_ptr<RegexAST<RegexNFAByteState>>>();
        uint32_t character = dynamic_cast<RegexASTLiteral<RegexNFAByteState>*>(r1.get())->get_character();
        return make_unique<DelimiterStringAST>(character);
    }

    void SchemaParser::add_lexical_rules () {
        add_token("Tab", '\t'); //9
        add_token("NewLine", '\n'); //10
        add_token("VerticalTab", '\v'); //11
        add_token("FormFeed", '\f'); //12
        add_token("CarriageReturn", '\r'); //13
        add_token("Space", ' ');
        add_token("Bang", '!');
        add_token("Quotation", '"');
        add_token("Hash", '#');
        add_token("DollarSign", '$');
        add_token("Percent", '%');
        add_token("Ampersand", '&');
        add_token("Apostrophe", '\'');
        add_token("Lparen", '(');
        add_token("Rparen", ')');
        add_token("Star", '*');
        add_token("Plus", '+');
        add_token("Comma", ',');
        add_token("Dash", '-');
        add_token("Dot", '.');
        add_token("ForwardSlash", '/');
        add_token_group("Numeric", make_unique<RegexASTGroup<RegexNFAByteState>>('0', '9'));
        add_token("Colon", ':');
        add_token("SemiColon", ';');
        add_token("LAngle", '<');
        add_token("Equal", '=');
        add_token("RAngle", '>');
        add_token("QuestionMark", '?');
        add_token("At", '@');
        add_token_group("AlphaNumeric", make_unique<RegexASTGroup<RegexNFAByteState>>('a', 'z'));
        add_token_group("AlphaNumeric", make_unique<RegexASTGroup<RegexNFAByteState>>('A', 'Z'));
        add_token_group("AlphaNumeric", make_unique<RegexASTGroup<RegexNFAByteState>>('0', '9'));
        add_token("Lbracket", '[');
        add_token("Backslash", '\\');
        add_token("Rbracket", ']');
        add_token("Hat", '^');
        add_token("Underscore", '_');
        add_token("Backtick", '`');
        add_token("Lbrace", '{');
        add_token("Vbar", '|');
        add_token("Rbrace", '}');
        add_token("Tilde", '~');
        add_token("d", 'd');
        add_token("s", 's');
        add_token("n", 'n');
        add_token("r", 'r');
        add_token("t", 't');
        add_token("f", 'f');
        add_token("v", 'v');
        add_token_chain("Delimiters", "delimiters");
        // default constructs to a m_negate group
        unique_ptr<RegexASTGroup<RegexNFAByteState>> comment_characters = make_unique<RegexASTGroup<RegexNFAByteState>>();
        comment_characters->add_literal('\r');
        comment_characters->add_literal('\n');
        add_token_group("CommentCharacters", std::move(comment_characters));
    }

    void SchemaParser::add_productions () {
        // add_production("SchemaFile", {}, new_schema_file_rule);
        add_production("SchemaFile", {"Comment"}, new_schema_file_rule);
        add_production("SchemaFile", {"SchemaVar"}, new_schema_file_rule_with_var);
        add_production("SchemaFile", {"Delimiters", "Colon", "DelimiterString"}, new_schema_file_rule_with_delimiters);
        add_production("SchemaFile", {"SchemaFile", "PortableNewLine"}, identity_rule_ParserASTSchemaFile);
        add_production("SchemaFile", {"SchemaFile", "PortableNewLine", "Comment"}, identity_rule_ParserASTSchemaFile);
        add_production("SchemaFile", {"SchemaFile", "PortableNewLine", "SchemaVar"},
                       std::bind(&SchemaParser::existing_schema_file_rule, this, std::placeholders::_1));
        add_production("SchemaFile", {"SchemaFile", "PortableNewLine", "Delimiters", "Colon", "DelimiterString"}, existing_schema_file_rule_with_delimiter);
        add_production("DelimiterString", {"DelimiterString", "Literal"}, existing_delimiter_string_rule);
        add_production("DelimiterString", {"Literal"}, new_delimiter_string_rule);
        add_production("PortableNewLine", {"CarriageReturn", "NewLine"}, nullptr);
        add_production("PortableNewLine", {"NewLine"}, nullptr);
        add_production("Comment", {"ForwardSlash", "ForwardSlash", "Text"}, nullptr);
        add_production("Text", {"Text", "CommentCharacters"}, nullptr);
        add_production("Text", {"CommentCharacters"}, nullptr);
        add_production("Text", {"Text", "Delimiters"}, nullptr);
        add_production("Text", {"Delimiters"}, nullptr);
        add_production("SchemaVar", {"WhitespaceStar", "Identifier", "Colon", "Regex"}, schema_var_rule);
        add_production("Identifier", {"Identifier", "AlphaNumeric"}, existing_identifier_rule);
        add_production("Identifier", {"AlphaNumeric"}, new_identifier_rule);
        add_production("WhitespaceStar", {"WhitespaceStar", "Space"}, nullptr);
        add_production("WhitespaceStar", {}, nullptr);
        add_production("Regex", {"Concat"}, regex_identity_rule);
        add_production("Concat", {"Concat", "Or"}, regex_cat_rule);
        add_production("Concat", {"Or"}, regex_identity_rule);
        add_production("Or", {"Or", "Vbar", "Literal"}, regex_or_rule);
        add_production("Or", {"MatchStar"}, regex_identity_rule);
        add_production("Or", {"MatchPlus"}, regex_identity_rule);
        add_production("Or", {"MatchExact"}, regex_identity_rule);
        add_production("Or", {"MatchRange"}, regex_identity_rule);
        add_production("Or", {"CompleteGroup"}, regex_identity_rule);
        add_production("MatchStar", {"CompleteGroup", "Star"}, regex_match_zero_or_more_rule);
        add_production("MatchPlus", {"CompleteGroup", "Plus"}, regex_match_one_or_more_rule);
        add_production("MatchExact", {"CompleteGroup", "Lbrace", "Integer", "Rbrace"}, regex_match_exactly_rule);
        add_production("MatchRange", {"CompleteGroup", "Lbrace", "Integer", "Comma", "Integer", "Rbrace"}, regex_match_range_rule);
        add_production("CompleteGroup", {"IncompleteGroup", "Rbracket"}, regex_identity_rule);
        add_production("CompleteGroup", {"Literal"}, regex_identity_rule);
        add_production("CompleteGroup", {"Digit"}, regex_identity_rule);
        add_production("CompleteGroup", {"Wildcard"}, regex_identity_rule);
        add_production("CompleteGroup", {"WhiteSpace"}, regex_identity_rule);
        add_production("IncompleteGroup", {"IncompleteGroup", "LiteralRange"}, regex_add_range_existing_group_rule);
        add_production("IncompleteGroup", {"IncompleteGroup", "Digit"}, regex_add_range_existing_group_rule);
        add_production("IncompleteGroup", {"IncompleteGroup", "Literal"}, regex_add_literal_existing_group_rule);
        add_production("IncompleteGroup", {"IncompleteGroup", "WhiteSpace"}, regex_add_literal_existing_group_rule);
        add_production("IncompleteGroup", {"Lbracket", "LiteralRange"}, regex_add_range_new_group_rule);
        add_production("IncompleteGroup", {"Lbracket", "Digit"}, regex_add_range_new_group_rule);
        add_production("IncompleteGroup", {"Lbracket", "Literal"}, regex_add_literal_new_group_rule);
        add_production("IncompleteGroup", {"Lbracket", "WhiteSpace"}, regex_add_literal_new_group_rule);
        add_production("IncompleteGroup", {"Lbracket", "Hat"}, regex_complement_incomplete_group_rule);
        add_production("LiteralRange", {"Literal", "Dash", "Literal"}, regex_range_rule);
        add_production("Literal", {"Backslash", "t"}, regex_tab_rule);
        add_production("Literal", {"Backslash", "n"}, regex_newline_rule);
        add_production("Literal", {"Backslash", "v"}, regex_vertical_tab_rule);
        add_production("Literal", {"Backslash", "f"}, regex_form_feed_rule);
        add_production("Literal", {"Backslash", "r"}, regex_char_return_rule);
        add_production("Literal", {"Space"}, regex_literal_rule);
        add_production("Literal", {"Bang"}, regex_literal_rule);
        add_production("Literal", {"Quotation"}, regex_literal_rule);
        add_production("Literal", {"Hash"}, regex_literal_rule);
        add_production("Literal", {"DollarSign"}, regex_literal_rule);
        add_production("Literal", {"Percent"}, regex_literal_rule);
        add_production("Literal", {"Ampersand"}, regex_literal_rule);
        add_production("Literal", {"Apostrophe"}, regex_literal_rule);
        add_production("Literal", {"Backslash", "Lparen"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Rparen"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Star"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Plus"}, regex_cancel_literal_rule);
        add_production("Literal", {"Comma"}, regex_literal_rule);
        add_production("Literal", {"Backslash", "Dash"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Dot"}, regex_cancel_literal_rule);
        add_production("Literal", {"ForwardSlash"}, regex_literal_rule);
        add_production("Literal", {"AlphaNumeric"}, regex_literal_rule);
        add_production("Literal", {"Colon"}, regex_literal_rule);
        add_production("Literal", {"SemiColon"}, regex_literal_rule);
        add_production("Literal", {"LAngle"}, regex_literal_rule);
        add_production("Literal", {"Equal"}, regex_literal_rule);
        add_production("Literal", {"RAngle"}, regex_literal_rule);
        add_production("Literal", {"QuestionMark"}, regex_literal_rule);
        add_production("Literal", {"At"}, regex_literal_rule);
        add_production("Literal", {"Backslash", "Lbracket"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Backslash"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Rbracket"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Hat"}, regex_cancel_literal_rule);
        add_production("Literal", {"Underscore"}, regex_literal_rule);
        add_production("Literal", {"Backtick"}, regex_literal_rule);
        add_production("Literal", {"Backslash", "Lbrace"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Vbar"}, regex_cancel_literal_rule);
        add_production("Literal", {"Backslash", "Rbrace"}, regex_cancel_literal_rule);
        add_production("Literal", {"Tilde"}, regex_literal_rule);
        add_production("Literal", {"Lparen", "Regex", "Rparen"}, regex_middle_identity_rule);
        add_production("Integer", {"Integer", "Numeric"}, regex_existing_integer_rule);
        add_production("Integer", {"Numeric"}, regex_new_integer_rule);
        add_production("Digit", {"Backslash", "d"}, regex_digit_rule);
        add_production("Wildcard", {"Dot"}, regex_wildcard_rule);
        add_production("WhiteSpace", {"Backslash", "s"}, regex_white_space_rule);
    }
}