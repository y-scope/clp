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
#include <log_surgeon/log_surgeon.hpp>
#include <spdlog/spdlog.h>

#include <clp/clp/run.hpp>
#include <clp/FileReader.hpp>
#include <clp/ir/types.hpp>
#include <clp/streaming_archive/reader/Archive.hpp>

#include "../clp/Utils.hpp"
#include "TestOutputCleaner.hpp"

using clp::load_parser_from_file;

namespace {
constexpr std::string_view cTestArchiveDirectory{"test-parser-with-user-schema-archive"};

auto run_clp_compress(
        std::filesystem::path const& schema_path,
        std::filesystem::path const& output_path,
        std::filesystem::path const& input_path
) -> int;
[[nodiscard]] auto get_tests_dir() -> std::filesystem::path;
[[nodiscard]] auto get_test_schema_files_dir() -> std::filesystem::path;
[[nodiscard]] auto get_test_log_dir() -> std::filesystem::path;

auto get_tests_dir() -> std::filesystem::path {
    std::filesystem::path const current_file_path{__FILE__};
    return std::filesystem::canonical(current_file_path.parent_path());
}

auto get_test_schema_files_dir() -> std::filesystem::path {
    return get_tests_dir() / "test_schema_files";
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
            "--unstructured-text-parsing-rule-set",
            schema_path_str.data(),
            output_path_str.data(),
            input_path_str.data(),
            nullptr
    };
    // `clp::clp::run` registers a logger for `spdlog` that persists across runs. `spdlog` will
    // error if a logger with the same name already exists. `spdlog::drop_all` clears all loggers,
    // ensuring `clp::clp::run` can safely create a fresh logger for each new call.
    spdlog::drop_all();
    return clp::clp::run(static_cast<int>(argv.size() - 1), argv.data());
}
}  // namespace

TEST_CASE("Test creating log parser with delimiters", "[LALR1Parser][LogParser]") {
    auto const schema_file_path{get_test_schema_files_dir() / "schema_with_delimiters.txt"};
    REQUIRE_NOTHROW(load_parser_from_file(schema_file_path.string()));
}

TEST_CASE("Test creating log parser from package template schema", "[LALR1Parser][LogParser]") {
    REQUIRE_NOTHROW(load_parser_from_file((get_tests_dir().parent_path().parent_path()
                                           / "package-template" / "src" / "etc"
                                           / "clp-schema.template.txt")
                                                  .string()));
}

TEST_CASE("Test creating log parser without delimiters", "[LALR1Parser][LogParser]") {
    auto const schema_file_path = get_test_schema_files_dir() / "schema_without_delimiters.txt";
    REQUIRE_NOTHROW(load_parser_from_file(schema_file_path.string()));
}

TEST_CASE("Verify CLP compression passes with non-header capture groups", "[Compression]") {
    auto const log_file_path{get_test_log_dir() / "log_with_capture.txt"};
    auto const schema_file_path{get_test_schema_files_dir() / "single_capture_group.txt"};
    TestOutputCleaner const cleaner{{std::string{cTestArchiveDirectory}}};
    std::filesystem::create_directory(cTestArchiveDirectory);

    REQUIRE(0 == run_clp_compress(schema_file_path, cTestArchiveDirectory, log_file_path));
}

TEST_CASE("Succeed on header rule with no capture", "[load_lexer]") {
    auto const schema_file_path{get_test_schema_files_dir() / "header_with_no_capture.txt"};
    REQUIRE_NOTHROW(load_parser_from_file(schema_file_path));
}

TEST_CASE("Succeed on header rule with a single timestamp capture", "[load_lexer]") {
    auto const schema_file_path{get_test_schema_files_dir() / "header_with_timestamp.txt"};
    REQUIRE_NOTHROW(load_parser_from_file(schema_file_path));
}

TEST_CASE("Verify CLP compression succeeds with non-capture header", "[Compression]") {
    auto const log_file_path{get_test_log_dir() / "log_with_capture.txt"};
    auto const schema_file_path{get_test_schema_files_dir() / "header_with_no_capture.txt"};
    TestOutputCleaner const cleaner{{std::string{cTestArchiveDirectory}}};
    std::filesystem::create_directory(cTestArchiveDirectory);

    REQUIRE(0 == run_clp_compress(schema_file_path, cTestArchiveDirectory, log_file_path));
}

TEST_CASE("Verify CLP compression succeeds with timestamp capture header", "[Compression]") {
    auto const log_file_path{get_test_log_dir() / "log_with_capture.txt"};
    auto const schema_file_path{get_test_schema_files_dir() / "header_with_timestamp.txt"};
    TestOutputCleaner const cleaner{{std::string{cTestArchiveDirectory}}};
    std::filesystem::create_directory(cTestArchiveDirectory);

    REQUIRE(0 == run_clp_compress(schema_file_path, cTestArchiveDirectory, log_file_path));
}

TEST_CASE("Verify CLP compression passes with non-timestamp capture header", "[Compression]") {
    auto const log_file_path{get_test_log_dir() / "log_with_capture.txt"};
    auto const schema_file_path{get_test_schema_files_dir() / "header_with_int.txt"};
    TestOutputCleaner const cleaner{{std::string{cTestArchiveDirectory}}};
    std::filesystem::create_directory(cTestArchiveDirectory);

    REQUIRE(0 == run_clp_compress(schema_file_path, cTestArchiveDirectory, log_file_path));
}

TEST_CASE("Verify CLP compression passes with multi-capture header", "[Compression]") {
    auto const log_file_path{get_test_log_dir() / "log_with_capture.txt"};
    auto const schema_file_path{get_test_schema_files_dir() / "header_with_timestamp_and_int.txt"};
    TestOutputCleaner const cleaner{{std::string{cTestArchiveDirectory}}};
    std::filesystem::create_directory(cTestArchiveDirectory);

    REQUIRE(0 == run_clp_compress(schema_file_path, cTestArchiveDirectory, log_file_path));
}
