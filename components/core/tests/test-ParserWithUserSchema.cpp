// TODO: move this test to log_surgeon
// TODO: move load_lexer_from_file into SearchParser in log_surgeon

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <fmt/format.h>
#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/LogParser.hpp>
#include <log_surgeon/SchemaParser.hpp>
#include <log_surgeon/Token.hpp>
#include <spdlog/spdlog.h>

#include <clp/clp/run.hpp>
#include <clp/FileReader.hpp>
#include <clp/ir/types.hpp>
#include <clp/LogSurgeonReader.hpp>
#include <clp/streaming_archive/Constants.hpp>
#include <clp/streaming_archive/reader/Archive.hpp>
#include <clp/type_utils.hpp>
#include <clp/Utils.hpp>

#include "TestOutputCleaner.hpp"

using clp::load_lexer_from_file;
using log_surgeon::lexers::ByteLexer;
using log_surgeon::LogParser;
using log_surgeon::SchemaAST;

namespace {
constexpr std::string_view cTestArchiveDirectory{"test-parser-with-user-schema-archive"};

auto run_clp_compress(
        std::filesystem::path const& schema_path,
        std::filesystem::path const& output_path,
        std::filesystem::path const& input_path
) -> int;
[[nodiscard]] auto get_config_schema_files_dir() -> std::filesystem::path;
[[nodiscard]] auto get_tests_dir() -> std::filesystem::path;
[[nodiscard]] auto get_test_schema_files_dir() -> std::filesystem::path;
[[nodiscard]] auto get_test_queries_dir() -> std::filesystem::path;
[[nodiscard]] auto get_test_log_dir() -> std::filesystem::path;

auto get_tests_dir() -> std::filesystem::path {
    std::filesystem::path const current_file_path{__FILE__};
    return std::filesystem::canonical(current_file_path.parent_path());
}

auto get_config_schema_files_dir() -> std::filesystem::path {
    std::filesystem::path const current_file_path{__FILE__};
    return std::filesystem::canonical(current_file_path.parent_path().parent_path()) / "config";
}

auto get_test_schema_files_dir() -> std::filesystem::path {
    return get_tests_dir() / "test_schema_files";
}

auto get_test_queries_dir() -> std::filesystem::path {
    return get_tests_dir() / "test_search_queries";
}

auto get_test_log_dir() -> std::filesystem::path {
    return get_tests_dir() / "test_log_files";
}

auto run_clp_compress(
        std::filesystem::path const& schema_path,
        std::filesystem::path const& output_path,
        std::filesystem::path const& input_path
) -> int {
    auto const schema_path_str{schema_path.string()};
    auto const output_path_str{output_path.string()};
    auto const input_path_str{input_path.string()};
    std::vector<char const*> argv{
            "clp",
            "c",
            "--schema-path",
            schema_path_str.data(),
            output_path_str.data(),
            input_path_str.data(),
            nullptr
    };
    return clp::clp::run(static_cast<int>(argv.size() - 1), argv.data());
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

TEST_CASE("Test creating log parser from config schema", "[LALR1Parser][LogParser]") {
    auto const schema_file_path = get_config_schema_files_dir() / "schemas.txt";
    generate_log_parser(schema_file_path.string());
}

TEST_CASE("Test creating log parser without delimiters", "[LALR1Parser][LogParser]") {
    auto const schema_file_path = get_test_schema_files_dir() / "schema_without_delimiters.txt";
    REQUIRE_THROWS_WITH(
            generate_log_parser(schema_file_path.string()),
            "When using --schema-path, \"delimiters:\" line must be used."
    );
}

TEST_CASE("Test lexer", "[Search]") {
    ByteLexer lexer;
    auto const schema_file_path = get_test_schema_files_dir() / "search_schema.txt";
    load_lexer_from_file(schema_file_path.string(), lexer);
    auto const query_file_path = get_test_queries_dir() / "easy.txt";
    clp::FileReader file_reader{query_file_path.string()};
    clp::LogSurgeonReader reader_wrapper(file_reader);
    log_surgeon::ParserInputBuffer parser_input_buffer;
    parser_input_buffer.read_if_safe(reader_wrapper);
    lexer.reset();
    auto [error_code, opt_token] = lexer.scan(parser_input_buffer);
    REQUIRE(error_code == log_surgeon::ErrorCode::Success);
    auto token{opt_token.value()};
    while (token.get_type_ids()->at(0) != static_cast<int>(log_surgeon::SymbolId::TokenEnd)) {
        SPDLOG_INFO("token:" + token.to_string() + "\n");
        SPDLOG_INFO(
                "token.get_type_ids()->back():" + lexer.m_id_symbol[token.get_type_ids()->back()]
                + "\n"
        );
        auto [error_code, opt_token] = lexer.scan(parser_input_buffer);
        REQUIRE(error_code == log_surgeon::ErrorCode::Success);
        token = opt_token.value();
    }
}

TEST_CASE("Test schema with single capture group", "[load_lexer]") {
    auto const schema_file_path{get_test_schema_files_dir() / "single_capture_group.txt"};
    ByteLexer lexer;
    load_lexer_from_file(schema_file_path, lexer);

    auto const rule_id{lexer.m_symbol_id.at("capture")};
    auto captures{lexer.get_captures_from_rule_id(rule_id)};
    REQUIRE(captures.has_value());
    REQUIRE(1 == captures->size());
    REQUIRE("group" == captures->at(0)->get_name());
}

TEST_CASE("Error on schema rule with multiple capture groups", "[load_lexer]") {
    auto const schema_file_path{get_test_schema_files_dir() / "multiple_capture_groups.txt"};
    ByteLexer lexer;
    REQUIRE_THROWS_WITH(
            load_lexer_from_file(schema_file_path, lexer),
            schema_file_path.string()
                    + ":3: error: the schema rule 'multicapture' has a regex pattern containing > "
                      "1 capture groups (found 2).\n"
    );
}

TEST_CASE("Verify dictionary contents", "[Compression]") {
    auto const log_file_path{get_test_log_dir() / "log_with_capture.txt"};
    auto const schema_file_path{get_test_schema_files_dir() / "single_capture_group.txt"};
    TestOutputCleaner const cleaner{{std::string{cTestArchiveDirectory}}};
    std::filesystem::create_directory(cTestArchiveDirectory);

    REQUIRE(0 == run_clp_compress(schema_file_path, cTestArchiveDirectory, log_file_path));

    std::vector<std::filesystem::path> archive_paths;
    for (auto const& entry : std::filesystem::directory_iterator{cTestArchiveDirectory}) {
        auto const& path{entry.path()};
        if (false == path.string().ends_with(clp::streaming_archive::cMetadataDBFileName)) {
            archive_paths.emplace_back(path);
        }
    }
    REQUIRE(1 == archive_paths.size());

    clp::streaming_archive::reader::Archive archive_reader;
    archive_reader.open(archive_paths.at(0));
    archive_reader.refresh_dictionaries();

    auto const& logtype_dict{archive_reader.get_logtype_dictionary()};
    REQUIRE(1 == logtype_dict.get_entries().size());
    REQUIRE(fmt::format(
                    "2016-05-08 07:34:05.251 MyDog{} APet{} test.txt\n",
                    clp::enum_to_underlying_type(clp::ir::VariablePlaceholder::Dictionary),
                    clp::enum_to_underlying_type(clp::ir::VariablePlaceholder::Dictionary)
            )
            == logtype_dict.get_value(0));

    auto const& var_dict{archive_reader.get_var_dictionary()};
    REQUIRE(2 == var_dict.get_entries().size());
    REQUIRE("123" == var_dict.get_value(0));
    REQUIRE("4123" == var_dict.get_value(1));
}
