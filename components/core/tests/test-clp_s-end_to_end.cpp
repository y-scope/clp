#include <sys/wait.h>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "../src/clp_s/CommandLineArguments.hpp"
#include "../src/clp_s/InputConfig.hpp"
#include "../src/clp_s/JsonConstructor.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

constexpr std::string_view cTestEndToEndArchiveDirectory{"test-end-to-end-archive"};
constexpr std::string_view cTestEndToEndOutputDirectory{"test-end-to-end-out"};
constexpr std::string_view cTestEndToEndOutputSortedJson{"test-end-to-end_sorted.jsonl"};
constexpr std::string_view cTestEndToEndInputFileDirectory{"test_log_files"};
constexpr std::string_view cTestEndToEndInputFile{"test_no_floats_sorted.jsonl"};

namespace {
auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path;
auto get_test_input_local_path() -> std::string;
auto extract() -> std::filesystem::path;
void compare(std::filesystem::path const& extracted_json_path);

auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path {
    return std::filesystem::path{cTestEndToEndInputFileDirectory} / cTestEndToEndInputFile;
}

auto get_test_input_local_path() -> std::string {
    std::filesystem::path const current_file_path{__FILE__};
    auto const tests_dir{current_file_path.parent_path()};
    return (tests_dir / get_test_input_path_relative_to_tests_dir()).string();
}

auto extract() -> std::filesystem::path {
    constexpr auto cDefaultOrdered = false;
    constexpr auto cDefaultTargetOrderedChunkSize = 0;

    std::filesystem::create_directory(cTestEndToEndOutputDirectory);
    REQUIRE(std::filesystem::is_directory(cTestEndToEndOutputDirectory));

    clp_s::JsonConstructorOption constructor_option{};
    constructor_option.output_dir = cTestEndToEndOutputDirectory;
    constructor_option.ordered = cDefaultOrdered;
    constructor_option.target_ordered_chunk_size = cDefaultTargetOrderedChunkSize;
    for (auto const& entry : std::filesystem::directory_iterator(cTestEndToEndArchiveDirectory)) {
        constructor_option.archive_path = clp_s::Path{
                .source{clp_s::InputSource::Filesystem},
                .path{entry.path().string()}
        };
        clp_s::JsonConstructor constructor{constructor_option};
        constructor.store();
    }
    std::filesystem::path extracted_json_path{cTestEndToEndOutputDirectory};
    extracted_json_path /= "original";
    REQUIRE(std::filesystem::exists(extracted_json_path));

    return extracted_json_path;
}

// Silence the checks below since our use of `std::system` is safe in the context of testing.
// NOLINTBEGIN(cert-env33-c,concurrency-mt-unsafe)
void compare(std::filesystem::path const& extracted_json_path) {
    int result{std::system("command -v jq >/dev/null 2>&1")};
    REQUIRE((0 == result));
    auto command = fmt::format(
            "jq --sort-keys --compact-output '.' {} | sort > {}",
            extracted_json_path.string(),
            cTestEndToEndOutputSortedJson
    );
    result = std::system(command.c_str());
    REQUIRE((0 == result));

    REQUIRE((false == std::filesystem::is_empty(cTestEndToEndOutputSortedJson)));

    result = std::system("command -v diff >/dev/null 2>&1");
    REQUIRE((0 == result));
    command = fmt::format(
            "diff --unified {} {}  > /dev/null",
            cTestEndToEndOutputSortedJson,
            get_test_input_local_path()
    );
    result = std::system(command.c_str());
    REQUIRE((true == WIFEXITED(result)));
    REQUIRE((0 == WEXITSTATUS(result)));
}

// NOLINTEND(cert-env33-c,concurrency-mt-unsafe)
}  // namespace

TEST_CASE("clp-s-compress-extract-no-floats", "[clp-s][end-to-end]") {
    auto structurize_arrays = GENERATE(true, false);
    auto single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{
            {std::string{cTestEndToEndArchiveDirectory},
             std::string{cTestEndToEndOutputDirectory},
             std::string{cTestEndToEndOutputSortedJson}}
    };

    REQUIRE_NOTHROW(compress_archive(
            get_test_input_local_path(),
            std::string{cTestEndToEndArchiveDirectory},
            single_file_archive,
            structurize_arrays,
            clp_s::FileType::Json
    ));

    auto extracted_json_path = extract();

    compare(extracted_json_path);
}
