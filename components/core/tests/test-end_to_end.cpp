#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp_s/JsonConstructor.hpp"
#include "../src/clp_s/JsonParser.hpp"

auto const cDefaultTargetEncodedSize = 8ULL * 1024 * 1024 * 1024; // 8 GB
auto const cDefaultMaxDocumentSize = 512ULL * 1024 * 1024; // 512 MB
auto const cDefaultMinTableSize = 1ULL * 1024 * 1024; // 1 MB
auto const cDefaultCompressionLevel = 3;
auto const cDefaultPrintArchiveStats = false;

constexpr char cTestEndToEndArchiveDirectory[] = "test-end-to-end-archive";
constexpr char cTestEndToEndOutputDirectory[] = "test-end-to-end-out";
constexpr char cTestEndToEndOutputSortedJson[] = "test-end-to-end_sorted.json";
constexpr char cTestEndToEndInputFileDirectory[] = "test_log_files";
constexpr char cTestEndToEndInputFile[] = "test_no_floats_sorted.json";


namespace {

class Cleanup {
  public:          
    Cleanup() {
        std::filesystem::remove_all(cTestEndToEndArchiveDirectory);
        std::filesystem::remove_all(cTestEndToEndOutputDirectory);
        std::filesystem::remove(cTestEndToEndOutputSortedJson);
    }
    ~Cleanup() {
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
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("clp-s_compression_and_extraction_no_floats", 
    "[clp-s][end-to-end]") {

    auto structurize_arrays = GENERATE(true, false);

    Cleanup test_cleanup;

    std::filesystem::create_directory(cTestEndToEndArchiveDirectory);
    REQUIRE(std::filesystem::is_directory(cTestEndToEndArchiveDirectory));

    clp_s::JsonParserOption parser_option{};
    parser_option.file_paths.push_back(get_test_input_local_path());
    parser_option.archives_dir = cTestEndToEndArchiveDirectory;
    parser_option.target_encoded_size = cDefaultTargetEncodedSize;
    parser_option.max_document_size = cDefaultMaxDocumentSize;
    parser_option.min_table_size = cDefaultMinTableSize;
    parser_option.compression_level = cDefaultCompressionLevel;
    parser_option.print_archive_stats = cDefaultPrintArchiveStats;
    parser_option.structurize_arrays = structurize_arrays;

    clp_s::JsonParser parser(parser_option);
    REQUIRE(parser.parse());
    parser.store();

    REQUIRE(false == std::filesystem::is_empty(cTestEndToEndArchiveDirectory));

    std::filesystem::create_directory(cTestEndToEndOutputDirectory);
    REQUIRE(std::filesystem::is_directory(cTestEndToEndOutputDirectory));

    clp_s::JsonConstructorOption constructor_option{};
    constructor_option.output_dir = cTestEndToEndOutputDirectory;
    constructor_option.ordered = false;
    constructor_option.archives_dir = parser_option.archives_dir;
    constructor_option.ordered_chunk_size = 0;
    for (auto const& entry : std::filesystem::directory_iterator(constructor_option.archives_dir)) {
        if (false == entry.is_directory()) {
            // Skip non-directories
            continue;
        }

        constructor_option.archive_id = entry.path().filename();
        clp_s::JsonConstructor constructor(constructor_option);
        constructor.store();
    }

    std::string command = cTestEndToEndOutputDirectory;
    command += "/original";
    REQUIRE(std::filesystem::exists(command.c_str()));

    int result = std::system("command -v jq >/dev/null 2>&1");
    REQUIRE(0 == result);
    command = "";
    command = (((command += "jq -S -c '.' ") += cTestEndToEndOutputDirectory) += "/original | sort > ") += cTestEndToEndOutputSortedJson;
    result = std::system(command.c_str());
    REQUIRE(0 == result);

    REQUIRE(false == std::filesystem::is_empty(cTestEndToEndOutputSortedJson));

    result = std::system("command -v diff >/dev/null 2>&1");
    REQUIRE(0 == result);
    command = "";
    command = ((((command +="diff -u ") += cTestEndToEndOutputSortedJson) += " ") += get_test_input_local_path()) += " > /dev/null";
    result = std::system(command.c_str());
    REQUIRE(0 == WEXITSTATUS(result));
}
