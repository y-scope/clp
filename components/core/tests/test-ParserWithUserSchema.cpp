// TODO: move this test to log_surgeon
// TODO: move load_lexer_from_file into SearchParser in log_surgeon

#include <sys/stat.h>

#include <filesystem>
#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <log_surgeon/LogParser.hpp>
#include <log_surgeon/SchemaParser.hpp>

#include "../src/clp/clp/run.hpp"
#include "../src/clp/GlobalMySQLMetadataDB.hpp"
#include "../src/clp/LogSurgeonReader.hpp"
#include "../src/clp/Utils.hpp"

using clp::FileReader;
using clp::load_lexer_from_file;
using clp::LogSurgeonReader;
using log_surgeon::DelimiterStringAST;
using log_surgeon::lexers::ByteLexer;
using log_surgeon::LogParser;
using log_surgeon::ParserAST;
using log_surgeon::SchemaAST;
using log_surgeon::SchemaVarAST;
using log_surgeon::Token;

namespace {
[[nodiscard]] auto get_tests_dir() -> std::filesystem::path;
[[nodiscard]] auto get_test_schema_files_dir() -> std::filesystem::path;
[[nodiscard]] auto get_test_queries_dir() -> std::filesystem::path;

auto get_tests_dir() -> std::filesystem::path {
    std::filesystem::path const current_file_path{__FILE__};
    return std::filesystem::canonical(current_file_path.parent_path());
}

auto get_test_schema_files_dir() -> std::filesystem::path {
    return get_tests_dir() / "test_schema_files";
}

auto get_test_queries_dir() -> std::filesystem::path {
    return get_tests_dir() / "test_search_queries";
}
}  // namespace

std::unique_ptr<SchemaAST> generate_schema_ast(std::string const& schema_file) {
    std::unique_ptr<SchemaAST> schema_ast = log_surgeon::SchemaParser::try_schema_file(schema_file);
    REQUIRE(schema_ast.get() != nullptr);
    return schema_ast;
}

std::unique_ptr<LogParser> generate_log_parser(std::string const& schema_file) {
    std::unique_ptr<SchemaAST> schema_ast = generate_schema_ast(schema_file);
    std::unique_ptr<LogParser> log_parser = std::make_unique<LogParser>(schema_file);
    REQUIRE(log_parser.get() != nullptr);
    return log_parser;
}

void compress(
        std::string const& output_dir,
        std::string const& file_to_compress,
        std::string schema_file,
        bool old = false
) {
    std::vector<std::string> arguments;
    if (old) {
        arguments = {"main.cpp", "c", output_dir, file_to_compress};
    } else {
        arguments
                = {"main.cpp",
                   "c",
                   output_dir,
                   file_to_compress,
                   "--schema-path",
                   std::move(schema_file)};
    }
    std::vector<char const*> argv;
    for (auto const& arg : arguments) {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);
    clp::clp::run(argv.size() - 1, argv.data());
}

void decompress(std::string archive_dir, std::string output_dir) {
    std::vector<std::string> arguments
            = {"main.cpp", "x", std::move(archive_dir), std::move(output_dir)};
    std::vector<char const*> argv;
    for (auto const& arg : arguments) {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);
    clp::clp::run(argv.size() - 1, argv.data());
}

TEST_CASE("Test error for missing schema file", "[LALR1Parser][SchemaParser]") {
    auto const file_path = get_test_schema_files_dir() / "missing_schema.txt";
    auto const file_path_string = file_path.string();
    REQUIRE_THROWS_WITH(
            generate_schema_ast(file_path_string),
            "Failed to read '" + file_path_string + "', error_code="
                    + std::to_string(static_cast<int>(log_surgeon::ErrorCode::FileNotFound))
    );
}

TEST_CASE("Test error for empty schema file", "[LALR1Parser][SchemaParser]") {
    auto const file_path = get_test_schema_files_dir() / "empty_schema.txt";
    REQUIRE_THROWS_WITH(
            generate_schema_ast(file_path.string()),
            "Schema:1:1: error: empty file\n"
            "          \n"
            "^\n"
    );
}

TEST_CASE("Test error for colon missing schema file", "[LALR1Parser][SchemaParser]") {
    auto const file_path = get_test_schema_files_dir() / "colon_missing_schema.txt";
    REQUIRE_THROWS_WITH(
            generate_schema_ast(file_path.string()),
            "Schema:3:4: error: expected '>',':','IdentifierCharacters' before ' ' token\n"
            "          int [0-9]+\n"
            "             ^\n"
    );
}

TEST_CASE("Test error for multi-character tokens in schema file", "[LALR1Parser][SchemaParser]") {
    auto const file_path
            = get_test_schema_files_dir() / "schema_with_multicharacter_token_error.txt";
    REQUIRE_THROWS_WITH(
            generate_schema_ast(file_path.string()),
            "Schema:2:11: error: expected ':' before ' ' token\n"
            "          delimiters : \\r\\n\n"
            "                    ^\n"
    );
}

TEST_CASE("Test creating schema parser", "[LALR1Parser][SchemaParser]") {
    auto const schema_file_path = get_test_schema_files_dir() / "easy_schema.txt";
    generate_schema_ast(schema_file_path.string());
}

TEST_CASE("Test creating log parser with delimiters", "[LALR1Parser][LogParser]") {
    auto const schema_file_path = get_test_schema_files_dir() / "schema_with_delimiters.txt";
    generate_log_parser(schema_file_path.string());
}

TEST_CASE("Test creating log parser without delimiters", "[LALR1Parser][LogParser]") {
    auto const schema_file_path = get_test_schema_files_dir() / "schema_without_delimiters.txt";
    REQUIRE_THROWS_WITH(
            generate_log_parser(schema_file_path.string()),
            "When using --schema-path, \"delimiters:\" line must be used."
    );
}

// TODO: This test doesn't currently work because delimiters are allowed in
// schema files, and there is no option to disable this yet
// TEST_CASE("Test error for creating log file with delimiter in regex pattern",
//          "[LALR1Parser]SchemaParser]") {
//    std::string file_path = "../tests/test_schema_files/schema_with_delimiter_in_regex_error.txt";
//    std::string file_name = boost::filesystem::canonical(file_path).string();
//    REQUIRE_THROWS_WITH(generate_log_parser(file_path),
//                        file_name +
//                        ":2: error: 'equals' has regex pattern which contains delimiter '='.\n"
//                        + "          equals:.*=.*\n"
//                        + "                 ^^^^^\n");
//}

// TODO: This error check is performed correctly by CLP, but it is handled by
// something different now so this test will fail as is
// TEST_CASE("Test error for missing log file", "[LALR1Parser][LogParser]") {
//    std::string file_name = "../tests/test_log_files/missing_log.txt";
//    std::string file_path = boost::filesystem::weakly_canonical(file_name).string();
//    REQUIRE_THROWS(compress("../tests/test_archives", file_name,
//                            "../tests/test_schema_files/schema_that_does_not_exist.txt"),
//                   "Specified schema file does not exist.");
//}

TEST_CASE("Test lexer", "[Search]") {
    ByteLexer lexer;
    auto const schema_file_path = get_test_schema_files_dir() / "search_schema.txt";
    load_lexer_from_file(schema_file_path.string(), lexer);
    auto const query_file_path = get_test_queries_dir() / "easy.txt";
    FileReader file_reader{query_file_path.string()};
    LogSurgeonReader reader_wrapper(file_reader);
    log_surgeon::ParserInputBuffer parser_input_buffer;
    parser_input_buffer.read_if_safe(reader_wrapper);
    lexer.reset();
    auto [error_code, opt_token] = lexer.scan(parser_input_buffer);
    REQUIRE(error_code == log_surgeon::ErrorCode::Success);
    Token token{opt_token.value()};
    while (token.m_type_ids_ptr->at(0) != static_cast<int>(log_surgeon::SymbolId::TokenEnd)) {
        SPDLOG_INFO("token:" + token.to_string() + "\n");
        SPDLOG_INFO(
                "token.m_type_ids->back():" + lexer.m_id_symbol[token.m_type_ids_ptr->back()] + "\n"
        );
        auto [error_code, opt_token] = lexer.scan(parser_input_buffer);
        REQUIRE(error_code == log_surgeon::ErrorCode::Success);
        token = opt_token.value();
    }
}
