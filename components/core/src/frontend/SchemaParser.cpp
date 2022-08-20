#include "SchemaParser.hpp"

// C libraries

// C++ libraries
#include <memory>
#include <cmath>

// Project headers
#include "../FileReader.hpp"
#include "Lexer.hpp"
#include "FiniteAutomata.hpp"
#include "LALR1Parser.hpp"

SchemaParser::SchemaParser() {
    add_tokens();
    add_productions();
    generate();
}

std::unique_ptr<ParserASTSchemaFile> SchemaParser::generate_schema_ast(ReaderInterface* reader) {
    NonTerminal nonterminal = parse(reader);
    return std::move(nonterminal.get_ParserASTSchemaFile());
}

//std::unique_ptr<ParserASTSchemaFile> SchemaParser::try_schema_file(std::string const& schema_file_path) {
//    if (true == schema_file_path.empty()) {
//        SPDLOG_ERROR("No schema file provided.");
//        return nullptr;
//    }
//
//    FileReader schema_reader;
//    ErrorCode error_code = schema_reader.try_open(schema_file_path);
//    if (ErrorCode_Success != error_code) {
//        if (ErrorCode_FileNotFound == error_code) {
//            SPDLOG_ERROR("'{}' does not exist.", schema_file_path.c_str());
//        } else if (ErrorCode_errno == error_code) {
//            SPDLOG_ERROR("Failed to read '{}', errno={}", schema_file_path.c_str(), errno);
//        } else {
//            SPDLOG_ERROR("Failed to read '{}', error_code={}", schema_file_path.c_str(), error_code);
//        }
//        return nullptr;
//    }
//
//    SchemaParser sp;
//    std::unique_ptr<ParserASTSchemaFile> schema_ast = sp.generate_schema_ast(&schema_reader);
//    schema_reader.close();
//    return schema_ast;
//}

static std::unique_ptr<ParserASTIdentifier> new_identifier_rule(NonTerminal* m) {
    std::string r1 = m->token_cast(0)->get_string();
    return std::make_unique<ParserASTIdentifier>(ParserASTIdentifier(r1[0]));
}

static std::unique_ptr<ParserAST> existing_identifier_rule(NonTerminal* m) {
    std::unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->get_parserAST();
    auto* r1_ptr = dynamic_cast<ParserASTIdentifier*>(r1.get());
    std::string r2 = m->token_cast(1)->get_string();
    r1_ptr->add_character(r2[0]);
    return std::move(r1);
}

static std::unique_ptr<ParserASTSchemaVar> schema_var_rule(NonTerminal* m) {
    auto* r2 = dynamic_cast<ParserASTIdentifier*>(m->nonterminal_cast(1)->get_parserAST().get());
    Token* colon_token = m->token_cast(2);
    auto& r4 = m->nonterminal_cast(3)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    return std::make_unique<ParserASTSchemaVar>(r2->name, std::move(r4), colon_token->line);
}

static std::unique_ptr<ParserASTSchemaFile> new_schema_file_rule(NonTerminal* m) {
    return std::make_unique<ParserASTSchemaFile>();
}

static std::unique_ptr<ParserASTSchemaFile> new_schema_file_rule_with_var(NonTerminal* m) {
    std::unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->get_parserAST();
    std::unique_ptr<ParserASTSchemaFile> schema_file_ast = std::make_unique<ParserASTSchemaFile>();
    schema_file_ast->add_schema_var(std::move(r1));
    return std::move(schema_file_ast);
}


static std::unique_ptr<ParserASTSchemaFile> new_schema_file_rule_with_delimiters(NonTerminal* m) {
    std::unique_ptr<ParserAST>& r1 = m->nonterminal_cast(2)->get_parserAST();
    std::unique_ptr<ParserASTSchemaFile> schema_file_ast = std::make_unique<ParserASTSchemaFile>();
    schema_file_ast->add_delimiters(std::move(r1));
    return std::move(schema_file_ast);
}

static std::unique_ptr<ParserASTSchemaFile> existing_schema_file_rule_with_delimiter(NonTerminal* m) {
    std::unique_ptr<ParserASTSchemaFile>& r1 = m->nonterminal_cast(0)->get_ParserASTSchemaFile();
    auto* parser_ast_scheme_file = dynamic_cast<ParserASTSchemaFile*>(r1.get());
    std::unique_ptr<ParserAST>& r5 = m->nonterminal_cast(4)->get_parserAST();
    parser_ast_scheme_file->add_delimiters(std::move(r5));
    return std::move(r1);
}

std::unique_ptr<ParserASTSchemaFile> SchemaParser::existing_schema_file_rule(NonTerminal* m) {
    std::unique_ptr<ParserASTSchemaFile>& r1 = m->nonterminal_cast(0)->get_ParserASTSchemaFile();
    auto* parser_ast_scheme_file = dynamic_cast<ParserASTSchemaFile*>(r1.get());
    std::unique_ptr<ParserAST>& r2 = m->nonterminal_cast(2)->get_parserAST();
    parser_ast_scheme_file->add_schema_var(std::move(r2));
    m_lexer.soft_reset();
    return std::move(r1);
}

//static std::unique_ptr<ParserAST> identity_rule(NonTerminal* m) {
//    return std::move(m->nonterminal_cast(0)->get_parserAST());
//}

static std::unique_ptr<ParserASTSchemaFile> identity_rule_ParserASTSchemaFile(NonTerminal* m) {
    return std::move(m->nonterminal_cast(0)->get_ParserASTSchemaFile());
}


typedef ParserValue<std::unique_ptr<RegexAST>> ParserValueRegex;

static std::unique_ptr<ParserAST> regex_identity_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(
            new ParserValueRegex(std::move(m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>())));
}

static std::unique_ptr<ParserAST> regex_cat_rule(NonTerminal* m) {
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto& r2 = m->nonterminal_cast(1)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTCat(std::move(r1), std::move(r2)))));
}

static std::unique_ptr<ParserAST> regex_or_rule(NonTerminal* m) {
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto& r2 = m->nonterminal_cast(2)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTOr(std::move(r1), std::move(r2)))));
}

static std::unique_ptr<ParserAST> regex_match_zero_or_more_rule(NonTerminal* m) {
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTMultiplication(std::move(r1), 0, 0))));
}

static std::unique_ptr<ParserAST> regex_match_one_or_more_rule(NonTerminal* m) {
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTMultiplication(std::move(r1), 1, 0))));
}

static std::unique_ptr<ParserAST> regex_match_exactly_rule(NonTerminal* m) {
    auto& r3 = m->nonterminal_cast(2)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r3_ptr = dynamic_cast<RegexASTInteger*>(r3.get());
    uint32_t reps = 0;
    uint32_t r3_size = r3_ptr->digits.size();
    for (uint32_t i = 0; i < r3_size; i++) {
        reps += r3_ptr->digits[i]*(uint32_t)pow(10,r3_size-i-1);
    }
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTMultiplication(std::move(r1), reps, reps))));
}

static std::unique_ptr<ParserAST> regex_match_range_rule(NonTerminal* m) {
    auto& r3 = m->nonterminal_cast(2)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r3_ptr = dynamic_cast<RegexASTInteger*>(r3.get());
    uint32_t min = 0;
    uint32_t r3_size = r3_ptr->digits.size();
    for (uint32_t i = 0; i < r3_size; i++) {
        min += r3_ptr->digits[i]*(uint32_t)pow(10,r3_size-i-1);
    }
    auto& r5 = m->nonterminal_cast(4)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r5_ptr = dynamic_cast<RegexASTInteger*>(r5.get());
    uint32_t max = 0;
    uint32_t r5_size = r5_ptr->digits.size();
    for (uint32_t i = 0; i < r5_size; i++) {
        max += r5_ptr->digits[i]*(uint32_t)pow(10,r5_size-i-1);
    }
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTMultiplication(std::move(r1), min, max))));
}

static std::unique_ptr<ParserAST> regex_add_literal_existing_group_rule(NonTerminal* m) {
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto& r2 = m->nonterminal_cast(1)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r1_ptr = dynamic_cast<RegexASTGroup*>(r1.get());
    auto* r2_ptr = dynamic_cast<RegexASTLiteral*>(r2.get());
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTGroup(r1_ptr, r2_ptr))));
}

static std::unique_ptr<ParserAST> regex_add_range_existing_group_rule(NonTerminal* m) {
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto& r2 = m->nonterminal_cast(1)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r1_ptr = dynamic_cast<RegexASTGroup*>(r1.get());
    auto* r2_ptr = dynamic_cast<RegexASTGroup*>(r2.get());
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTGroup(r1_ptr, r2_ptr))));
}

static std::unique_ptr<ParserAST> regex_add_literal_new_group_rule(NonTerminal* m) {
    auto& r2 = m->nonterminal_cast(1)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r2_ptr = dynamic_cast<RegexASTLiteral*>(r2.get());
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTGroup(r2_ptr))));
}

static std::unique_ptr<ParserAST> regex_add_range_new_group_rule(NonTerminal* m) {
    auto& r2 = m->nonterminal_cast(1)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r2_ptr = dynamic_cast<RegexASTGroup*>(r2.get());
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTGroup(r2_ptr))));
}

static std::unique_ptr<ParserAST> regex_complement_incomplete_group_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::make_unique<RegexASTGroup>()));
}

static std::unique_ptr<ParserAST> regex_range_rule(NonTerminal* m) {
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto& r2 = m->nonterminal_cast(2)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r1_ptr = dynamic_cast<RegexASTLiteral*>(r1.get());
    auto* r2_ptr = dynamic_cast<RegexASTLiteral*>(r2.get());
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTGroup(r1_ptr, r2_ptr))));
}

static std::unique_ptr<ParserAST> regex_middle_identity_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(
            new ParserValueRegex(std::move(m->nonterminal_cast(1)->get_parserAST()->get<std::unique_ptr<RegexAST>>())));
}

static std::unique_ptr<ParserAST> regex_literal_rule(NonTerminal* m) {
    Token* token = m->token_cast(0);
    assert(token->get_string().size() == 1);
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTLiteral(token->get_string()[0]))));
}

static std::unique_ptr<ParserAST> regex_cancel_literal_rule(NonTerminal* m) {
    Token* token = m->token_cast(1);
    assert(token->get_string().size() == 1);
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTLiteral(token->get_string()[0]))));
}

static std::unique_ptr<ParserAST> regex_existing_integer_rule(NonTerminal* m) {
    auto& r2 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r2_ptr = dynamic_cast<RegexASTInteger*>(r2.get());
    Token* token = m->token_cast(1);
    assert(token->get_string().size() == 1);
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTInteger(r2_ptr, token->get_string()[0]))));
}

static std::unique_ptr<ParserAST> regex_new_integer_rule(NonTerminal* m) {
    Token* token = m->token_cast(0);
    assert(token->get_string().size() == 1);
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTInteger(token->get_string()[0]))));
}

static std::unique_ptr<ParserAST> regex_digit_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTGroup('0', '9'))));
}

static std::unique_ptr<ParserAST> regex_wildcard_rule(NonTerminal* m) {
    std::unique_ptr<RegexASTGroup> regex_wildcard = std::make_unique<RegexASTGroup>(0, RegexASTGroup::UNICODE_MAX);
    regex_wildcard->set_is_wildcard_true();
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::move(regex_wildcard)));
}

static std::unique_ptr<ParserAST> regex_vertical_tab_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTLiteral('\v'))));
}

static std::unique_ptr<ParserAST> regex_form_feed_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTLiteral('\f'))));
}

static std::unique_ptr<ParserAST> regex_tab_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTLiteral('\t'))));
}

static std::unique_ptr<ParserAST> regex_char_return_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTLiteral('\r'))));
}

static std::unique_ptr<ParserAST> regex_newline_rule(NonTerminal* m) {
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(new RegexASTLiteral('\n'))));
}

static std::unique_ptr<ParserAST> regex_white_space_rule(NonTerminal* m) {
    std::unique_ptr<RegexASTGroup> regex_ast_group = std::make_unique<RegexASTGroup>(RegexASTGroup({ ' ', '\t', '\r', '\n', '\v', '\f' }));
    return std::unique_ptr<ParserAST>(new ParserValueRegex(std::unique_ptr<RegexAST>(std::move(regex_ast_group))));
}
static std::unique_ptr<ParserAST> existing_delimiter_string_rule(NonTerminal* m) {
    std::unique_ptr<ParserAST>& r1 = m->nonterminal_cast(0)->get_parserAST();
    auto& r2 = m->nonterminal_cast(1)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    auto* r1_ptr = dynamic_cast<DelimiterStringAST*>(r1.get());
    uint32_t character = dynamic_cast<RegexASTLiteral*>(r2.get())->character;
    r1_ptr->add_delimiter(character);
    return std::move(r1);
}

static std::unique_ptr<ParserAST> new_delimiter_string_rule(NonTerminal* m) {
    auto& r1 = m->nonterminal_cast(0)->get_parserAST()->get<std::unique_ptr<RegexAST>>();
    uint32_t character = dynamic_cast<RegexASTLiteral*>(r1.get())->character;
    return std::make_unique<DelimiterStringAST>(character);
}

void SchemaParser::add_tokens() {
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
    add_token_group("Numeric", std::make_unique<RegexASTGroup>('0', '9'));
    add_token("Colon", ':');
    add_token("SemiColon", ';');
    add_token("LAngle", '<');
    add_token("Equal", '=');
    add_token("RAngle", '>');
    add_token("QuestionMark", '?');
    add_token("At", '@');
    add_token_group("AlphaNumeric", std::make_unique<RegexASTGroup>('a', 'z'));
    add_token_group("AlphaNumeric", std::make_unique<RegexASTGroup>('A', 'Z'));
    add_token_group("AlphaNumeric", std::make_unique<RegexASTGroup>('0', '9'));
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
    // default constructs to a negate group
    std::unique_ptr<RegexASTGroup> comment_characters = std::make_unique<RegexASTGroup>();
    comment_characters->add_literal('\r');
    comment_characters->add_literal('\n');
    add_token_group("CommentCharacters", std::move(comment_characters));
}

void SchemaParser::add_productions() {
    add_production("SchemaFile", { }, new_schema_file_rule);
    add_production("SchemaFile", { "Comment" }, new_schema_file_rule);
    add_production("SchemaFile", { "SchemaVar" }, new_schema_file_rule_with_var);
    add_production("SchemaFile", { "Delimiters", "Colon", "DelimiterString"}, new_schema_file_rule_with_delimiters);
    add_production("SchemaFile", { "SchemaFile", "PortableNewLine"}, identity_rule_ParserASTSchemaFile);
    add_production("SchemaFile", { "SchemaFile", "PortableNewLine", "Comment"}, identity_rule_ParserASTSchemaFile);
    add_production("SchemaFile", { "SchemaFile", "PortableNewLine", "SchemaVar" }, 
                   std::bind(&SchemaParser::existing_schema_file_rule, this, std::placeholders::_1));
    add_production("SchemaFile", { "SchemaFile", "PortableNewLine", "Delimiters", "Colon", "DelimiterString" }, existing_schema_file_rule_with_delimiter);
    add_production("DelimiterString", { "DelimiterString", "Literal" }, existing_delimiter_string_rule);
    add_production("DelimiterString", { "Literal" }, new_delimiter_string_rule);
    add_production("PortableNewLine", { "CarriageReturn", "NewLine" }, nullptr);
    add_production("PortableNewLine", { "NewLine" }, nullptr);
    add_production("Comment", {"ForwardSlash", "ForwardSlash", "Text"}, nullptr);
    add_production("Text", {"Text", "CommentCharacters"}, nullptr);
    add_production("Text", {"CommentCharacters"}, nullptr);
    add_production("Text", {"Text", "Delimiters"}, nullptr);
    add_production("Text", {"Delimiters"}, nullptr);
    add_production("SchemaVar", { "WhitespaceStar", "Identifier", "Colon", "Regex"}, schema_var_rule);
    add_production("Identifier", { "Identifier", "AlphaNumeric" }, existing_identifier_rule);
    add_production("Identifier", { "AlphaNumeric" }, new_identifier_rule);
    add_production("WhitespaceStar", { "WhitespaceStar", "Space" }, nullptr);
    add_production("WhitespaceStar", { }, nullptr);
    add_production("Regex", { "Concat" }, regex_identity_rule);
    add_production("Concat", { "Concat","Or" }, regex_cat_rule);
    add_production("Concat", { "Or" }, regex_identity_rule);
    add_production("Or", { "Or","Vbar","Literal" }, regex_or_rule);
    add_production("Or", { "MatchStar" },regex_identity_rule);
    add_production("Or", { "MatchPlus" },regex_identity_rule);
    add_production("Or", { "MatchExact" },regex_identity_rule);
    add_production("Or", { "MatchRange" },regex_identity_rule);
    add_production("Or", { "CompleteGroup" },regex_identity_rule);
    add_production("MatchStar", { "CompleteGroup","Star" }, regex_match_zero_or_more_rule);
    add_production("MatchPlus", { "CompleteGroup","Plus" }, regex_match_one_or_more_rule);
    add_production("MatchExact", { "CompleteGroup","Lbrace","Integer","Rbrace" }, regex_match_exactly_rule);
    add_production("MatchRange", { "CompleteGroup","Lbrace","Integer","Comma","Integer","Rbrace" }, regex_match_range_rule);
    add_production("CompleteGroup", { "IncompleteGroup","Rbracket" }, regex_identity_rule);
    add_production("CompleteGroup", { "Literal" }, regex_identity_rule);
    add_production("CompleteGroup", { "Digit" }, regex_identity_rule);
    add_production("CompleteGroup", { "Wildcard" }, regex_identity_rule);
    add_production("CompleteGroup", { "WhiteSpace" }, regex_identity_rule);
    add_production("IncompleteGroup", { "IncompleteGroup","LiteralRange" }, regex_add_range_existing_group_rule);
    add_production("IncompleteGroup", { "IncompleteGroup","Digit" }, regex_add_range_existing_group_rule);
    add_production("IncompleteGroup", { "IncompleteGroup","Literal" }, regex_add_literal_existing_group_rule);
    add_production("IncompleteGroup", { "IncompleteGroup","WhiteSpace" }, regex_add_literal_existing_group_rule);
    add_production("IncompleteGroup", { "Lbracket","LiteralRange" }, regex_add_range_new_group_rule);
    add_production("IncompleteGroup", { "Lbracket","Digit" }, regex_add_range_new_group_rule);
    add_production("IncompleteGroup", { "Lbracket","Literal" }, regex_add_literal_new_group_rule);
    add_production("IncompleteGroup", { "Lbracket","WhiteSpace" }, regex_add_literal_new_group_rule);
    add_production("IncompleteGroup", { "Lbracket","Hat" }, regex_complement_incomplete_group_rule);
    add_production("LiteralRange", { "Literal","Dash","Literal" }, regex_range_rule);
    add_production("Literal", { "Backslash", "t" }, regex_tab_rule);
    add_production("Literal", { "Backslash", "n" }, regex_newline_rule);
    add_production("Literal", { "Backslash", "v" }, regex_vertical_tab_rule);
    add_production("Literal", { "Backslash", "f" }, regex_form_feed_rule);
    add_production("Literal", { "Backslash", "r" }, regex_char_return_rule);
    add_production("Literal", { "Space" }, regex_literal_rule);
    add_production("Literal", { "Bang" }, regex_literal_rule);
    add_production("Literal", { "Quotation" }, regex_literal_rule);
    add_production("Literal", { "Hash" }, regex_literal_rule);
    add_production("Literal", { "DollarSign" }, regex_literal_rule);
    add_production("Literal", { "Percent" }, regex_literal_rule);
    add_production("Literal", { "Ampersand" }, regex_literal_rule);
    add_production("Literal", { "Apostrophe" }, regex_literal_rule);
    add_production("Literal", { "Backslash", "Lparen" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Rparen" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Star" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Plus" }, regex_cancel_literal_rule);
    add_production("Literal", { "Comma" }, regex_literal_rule);
    add_production("Literal", { "Backslash", "Dash" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Dot" }, regex_cancel_literal_rule);
    add_production("Literal", { "ForwardSlash" }, regex_literal_rule);
    add_production("Literal", { "AlphaNumeric" }, regex_literal_rule);
    add_production("Literal", { "Colon" }, regex_literal_rule);
    add_production("Literal", { "SemiColon" }, regex_literal_rule);
    add_production("Literal", { "LAngle" }, regex_literal_rule);
    add_production("Literal", { "Equal" }, regex_literal_rule);
    add_production("Literal", { "RAngle" }, regex_literal_rule);
    add_production("Literal", { "QuestionMark" }, regex_literal_rule);
    add_production("Literal", { "At" }, regex_literal_rule);
    add_production("Literal", { "Backslash", "Lbracket" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Backslash" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Rbracket" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Hat" }, regex_cancel_literal_rule);
    add_production("Literal", { "Underscore" }, regex_literal_rule);
    add_production("Literal", { "Backtick" }, regex_literal_rule);
    add_production("Literal", { "Backslash", "Lbrace" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Vbar" }, regex_cancel_literal_rule);
    add_production("Literal", { "Backslash", "Rbrace" }, regex_cancel_literal_rule);
    add_production("Literal", { "Tilde" }, regex_literal_rule);
    add_production("Literal", { "Lparen","Regex","Rparen" }, regex_middle_identity_rule);
    add_production("Integer", { "Integer","Numeric" }, regex_existing_integer_rule);
    add_production("Integer", { "Numeric" }, regex_new_integer_rule);
    add_production("Digit", { "Backslash", "d" }, regex_digit_rule);
    add_production("Wildcard", { "Dot" }, regex_wildcard_rule);
    add_production("WhiteSpace", { "Backslash", "s" }, regex_white_space_rule);
}
