#include <sys/wait.h>

#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp_s/JsonConstructor.hpp"
#include "../src/clp_s/JsonParser.hpp"

constexpr std::string_view cTestEndToEndArchiveDirectory{"test-end-to-end-archive"};
constexpr std::string_view cTestEndToEndOutputDirectory{"test-end-to-end-out"};
constexpr std::string_view cTestEndToEndOutputSortedJson{"test-end-to-end_sorted.json"};
constexpr std::string_view cTestEndToEndInputFileDirectory{"test_log_files"};
constexpr std::string_view cTestEndToEndInputFile{"test_no_floats_sorted.json"};

namespace {
/**
 * Class with no data members who's contruction and destruction is soley
 *      for cleanup up files and directorys created by the test case.
 */
class Cleanup {
public:
    Cleanup() { delete_files(); }

    ~Cleanup() { delete_files(); }

private:
    static void delete_files() {
        std::filesystem::remove_all(cTestEndToEndArchiveDirectory);
        std::filesystem::remove_all(cTestEndToEndOutputDirectory);
        std::filesystem::remove(cTestEndToEndOutputSortedJson);
    }
};

auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path {
    return std::filesystem::path{cTestEndToEndInputFileDirectory} / cTestEndToEndInputFile;
}

auto get_test_input_local_path() -> std::string {
    std::filesystem::path const current_file_path{__FILE__};
    auto const tests_dir{current_file_path.parent_path()};
    return (tests_dir / get_test_input_path_relative_to_tests_dir()).string();
}
}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("clp-s_compression_and_extraction_no_floats", "[clp-s][end-to-end]") {
    auto const default_target_encoded_size = 8ULL * 1024 * 1024 * 1024;  // 8 GiB
    auto const default_max_document_size = 512ULL * 1024 * 1024;  // 512 MiB
    auto const default_min_table_size = 1ULL * 1024 * 1024;  // 1 MiB
    auto const default_compression_level = 3;
    auto const default_print_archive_stats = false;
    auto const default_ordered = false;
    auto const default_target_ordered_chunk_size = 0;
    auto structurize_arrays = GENERATE(true, false);

    Cleanup const test_cleanup;

    std::filesystem::create_directory(cTestEndToEndArchiveDirectory);
    REQUIRE(std::filesystem::is_directory(cTestEndToEndArchiveDirectory));

    clp_s::JsonParserOption parser_option{};
    parser_option.file_paths.push_back(get_test_input_local_path());
    parser_option.archives_dir = cTestEndToEndArchiveDirectory;
    parser_option.target_encoded_size = default_target_encoded_size;
    parser_option.max_document_size = default_max_document_size;
    parser_option.min_table_size = default_min_table_size;
    parser_option.compression_level = default_compression_level;
    parser_option.print_archive_stats = default_print_archive_stats;
    parser_option.structurize_arrays = structurize_arrays;

    clp_s::JsonParser parser{parser_option};
    REQUIRE(parser.parse());
    parser.store();

    REQUIRE(false == std::filesystem::is_empty(cTestEndToEndArchiveDirectory));

    std::filesystem::create_directory(cTestEndToEndOutputDirectory);
    REQUIRE(std::filesystem::is_directory(cTestEndToEndOutputDirectory));

    clp_s::JsonConstructorOption constructor_option{};
    constructor_option.archives_dir = parser_option.archives_dir;
    constructor_option.output_dir = cTestEndToEndOutputDirectory;
    constructor_option.ordered = false;
    constructor_option.target_ordered_chunk_size = 0;
    for (auto const& entry : std::filesystem::directory_iterator(constructor_option.archives_dir)) {
        if (false == entry.is_directory()) {
            // Skip non-directories
            continue;
        }

        constructor_option.archive_id = entry.path().filename();
        clp_s::JsonConstructor constructor{constructor_option};
        constructor.store();
    }

    std::filesystem::path extracted_json_path{cTestEndToEndOutputDirectory};
    extracted_json_path /= "original";
    REQUIRE(std::filesystem::exists(extracted_json_path));

    int result = std::system("command -v jq >/dev/null 2>&1");
    REQUIRE(0 == result);
    std::string command = std::format(
            "jq --sort-keys --compact-output '.' {}/original | sort > {}",
            cTestEndToEndOutputDirectory,
            cTestEndToEndOutputSortedJson
    );
    result = std::system(command.c_str());
    REQUIRE(0 == result);

    REQUIRE(false == std::filesystem::is_empty(cTestEndToEndOutputSortedJson));

    result = std::system("command -v diff >/dev/null 2>&1");
    REQUIRE(0 == result);
    command = std::format(
            "diff -u {} {} > /dev/null",
            cTestEndToEndOutputSortedJson,
            get_test_input_local_path()
    );
    result = std::system(command.c_str());
    REQUIRE(0 == WEXITSTATUS(result));
}
