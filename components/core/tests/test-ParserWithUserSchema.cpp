// C libraries
#include <sys/stat.h>

// Boost libraries
#include <boost/filesystem.hpp>
#include <utility>

// Catch2
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// Project headers
#include "../src/clp/run.hpp"
#include "../src/compressor_frontend/utils.hpp"
#include "../src/compressor_frontend/LogParser.hpp"
#include "../src/GlobalMySQLMetadataDB.hpp"

using compressor_frontend::DelimiterStringAST;
using compressor_frontend::LALR1Parser;
using compressor_frontend::lexers::ByteLexer;
using compressor_frontend::LogParser;
using compressor_frontend::ParserAST;
using compressor_frontend::SchemaFileAST;
using compressor_frontend::SchemaParser;
using compressor_frontend::SchemaVarAST;
using compressor_frontend::Token;

std::unique_ptr<SchemaFileAST> generate_schema_ast(const std::string& schema_file) {
    SchemaParser schema_parser;
    FileReader schema_file_reader;
    schema_file_reader.open(schema_file);
    REQUIRE(schema_file_reader.is_open());
    std::unique_ptr<SchemaFileAST> schema_ast = schema_parser.generate_schema_ast(schema_file_reader);
    REQUIRE(schema_ast.get() != nullptr);
    return schema_ast;
}

std::unique_ptr<LogParser> generate_log_parser(const std::string& schema_file) {
    std::unique_ptr<SchemaFileAST> schema_ast = generate_schema_ast(schema_file);
    std::unique_ptr<LogParser> log_parser = std::make_unique<LogParser>(schema_file);
    REQUIRE(log_parser.get() != nullptr);
    return log_parser;
}

void compress(const std::string& output_dir, const std::string& file_to_compress, std::string schema_file, bool old = false) {
    std::vector<std::string> arguments;
    if(old) {
        arguments = {"main.cpp", "c", output_dir, file_to_compress};        
    } else {
        arguments = {"main.cpp", "c", output_dir, file_to_compress, "--schema-path", std::move(schema_file)};
    }
    std::vector<char*> argv;
    for (const auto& arg : arguments)
        argv.push_back((char*)arg.data());
    argv.push_back(nullptr);
    clp::run(argv.size() - 1, (const char**) argv.data());
}

void decompress(std::string archive_dir, std::string output_dir) {
    std::vector<std::string> arguments = {"main.cpp", "x", std::move(archive_dir), std::move(output_dir)};
    std::vector<char*> argv;
    for (const auto& arg : arguments)
        argv.push_back((char*)arg.data());
    argv.push_back(nullptr);
    std::string archive_path;
    clp::run(argv.size() - 1, (const char**) argv.data());
}

TEST_CASE("Test error for missing schema file", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/missing_schema.txt";
    std::string file_name = boost::filesystem::weakly_canonical(file_path).string();
    REQUIRE_THROWS_WITH(generate_schema_ast(file_path), "File not found: " + file_name + "\n");
    SPDLOG_INFO("File not found: " + file_name + "\n");
}

TEST_CASE("Test error for empty schema file", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/empty_schema.txt";
    std::string file_name = boost::filesystem::canonical(file_path).string();
    REQUIRE_THROWS_WITH(generate_schema_ast(file_path), file_name +":1:1: error: empty file\n"
                                                                  +"          \n"
                                                                  +"^\n");
}

TEST_CASE("Test error for colon missing schema file", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/colon_missing_schema.txt";
    std::string file_name = boost::filesystem::canonical(file_path).string(); 
    REQUIRE_THROWS_WITH(generate_schema_ast(file_path), file_name +":3:4: error: expected ':','AlphaNumeric' before ' ' token\n"
                                                                  +"          int [0-9]+\n"
                                                                  +"             ^\n");
}

TEST_CASE("Test error for multi-character tokens in schema file", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/schema_with_multicharacter_token_error.txt";
    std::string file_name = boost::filesystem::canonical(file_path).string();
    REQUIRE_THROWS_WITH(generate_schema_ast(file_path), file_name +":2:11: error: expected ':' before ' ' token\n"
                                                                  +"          delimiters : \\r\\n\n"
                                                                  +"                    ^\n");
}

TEST_CASE("Test creating schema parser", "[LALR1Parser][SchemaParser]") {
    generate_schema_ast("../tests/test_schema_files/easy_schema.txt");
}

TEST_CASE("Test creating log parser with delimiters", "[LALR1Parser][LogParser]") {
    generate_log_parser("../tests/test_schema_files/schema_with_delimiters.txt");
}

TEST_CASE("Test creating log parser without delimiters", "[LALR1Parser][LogParser]") {
    REQUIRE_THROWS_WITH(generate_log_parser("../tests/test_schema_files/schema_without_delimiters.txt"),
                        "When using --schema-path, \"delimiters:\" line must be used.");
}

TEST_CASE("Test error for creating log file with delimiter in regex pattern", "[LALR1Parser][SchemaParser]") {
    std::string file_path = "../tests/test_schema_files/schema_with_delimiter_in_regex_error.txt";
    std::string file_name = boost::filesystem::canonical(file_path).string();
    REQUIRE_THROWS_WITH(generate_log_parser(file_path), file_name + ":2: error: 'equals' has regex pattern which contains delimiter '='.\n"
                                                        + "          equals:.*=.*\n"
                                                        + "                 ^^^^^\n");
}

/// TODO: This error check is performed correctly by CLP, but it is handled by something different now so this test will fail as is
//TEST_CASE("Test error for missing log file", "[LALR1Parser][LogParser]") {
//    std::string file_name = "../tests/test_log_files/missing_log.txt";
//    std::string file_path = boost::filesystem::weakly_canonical(file_name).string();
//    REQUIRE_THROWS(compress("../tests/test_archives", file_name, "../tests/test_schema_files/schema_that_does_not_exist.txt"),
//                   "Specified schema file does not exist.");
//}

TEST_CASE("Test forward lexer", "[Search]") {
    ByteLexer forward_lexer;
    std::string schema_file_name = "../tests/test_schema_files/search_schema.txt";
    std::string schema_file_path = boost::filesystem::weakly_canonical(schema_file_name).string();
    compressor_frontend::load_lexer_from_file(schema_file_path, false, forward_lexer);
    FileReader reader;
    reader.open("../tests/test_search_queries/easy.txt");
    forward_lexer.reset(reader);
    Token token = forward_lexer.scan();
    while (token.m_type_ids->at(0) != (int)compressor_frontend::SymbolID::TokenEndID) {
        SPDLOG_INFO("token:" + token.get_string() + "\n");
        SPDLOG_INFO("token.m_type_ids->back():" + forward_lexer.m_id_symbol[token.m_type_ids->back()] + "\n");
        token = forward_lexer.scan();
    }
}

TEST_CASE("Test reverse lexer", "[Search]") {
    ByteLexer reverse_lexer;
    std::string schema_file_name = "../tests/test_schema_files/search_schema.txt";
    std::string schema_file_path = boost::filesystem::weakly_canonical(schema_file_name).string();
    compressor_frontend::load_lexer_from_file(schema_file_path, true, reverse_lexer);
    FileReader reader;
    reader.open("../tests/test_search_queries/easy.txt");
    reverse_lexer.reset(reader);
    Token token = reverse_lexer.scan();
    while (token.m_type_ids->at(0) != (int)compressor_frontend::SymbolID::TokenEndID) {
        SPDLOG_INFO("token:" + token.get_string() + "\n");
        SPDLOG_INFO("token.m_type_ids->back():" + reverse_lexer.m_id_symbol[token.m_type_ids->back()] + "\n");
        token = reverse_lexer.scan();
    }
}
