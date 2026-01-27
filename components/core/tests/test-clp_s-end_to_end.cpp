#include <sys/wait.h>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/format.h>

#include "../src/clp_s/archive_constants.hpp"
#include "../src/clp_s/ArchiveReader.hpp"
#include "../src/clp_s/CommandLineArguments.hpp"
#include "../src/clp_s/InputConfig.hpp"
#include "../src/clp_s/JsonConstructor.hpp"
#include "../src/clp_s/SchemaTree.hpp"
#include "../src/clp_s/SingleFileArchiveDefs.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

constexpr std::string_view cTestEndToEndArchiveDirectory{"test-end-to-end-archive"};
constexpr std::string_view cTestEndToEndOutputDirectory{"test-end-to-end-out"};
constexpr std::string_view cTestEndToEndOutputSortedJson{"test-end-to-end_sorted.jsonl"};
constexpr std::string_view cTestEndToEndInputFileDirectory{"test_log_files"};
constexpr std::string_view cTestEndToEndInputFile{"test_no_floats_sorted.jsonl"};
constexpr std::string_view cTestEndToEndExpectedOutputSortedFile{
        "test-end-to-end_expected_output_sorted.jsonl"
};
constexpr std::string_view cTestEndToEndValidFormattedFloatInputFile{
        "test_valid_formatted_float.jsonl"
};
constexpr std::string_view cTestEndToEndInvalidFormattedFloatInputFile{
        "test_invalid_formatted_float.jsonl"
};

namespace {
auto get_test_input_path_relative_to_tests_dir(std::string_view const test_input_path)
        -> std::filesystem::path;
auto get_test_input_local_path(std::string_view const test_input_path) -> std::string;
auto extract() -> std::filesystem::path;
void compare(std::filesystem::path const& extracted_json_path);
void literallyCompare(
        std::filesystem::path const& expected_output_json_path,
        std::filesystem::path const& extracted_json_path
);
void check_all_leaf_nodes_match_types(std::set<clp_s::NodeType> const& types);
void validate_archive_header();

auto get_test_input_path_relative_to_tests_dir(std::string_view const test_input_path)
        -> std::filesystem::path {
    return std::filesystem::path{cTestEndToEndInputFileDirectory} / test_input_path;
}

auto get_test_input_local_path(std::string_view const test_input_path) -> std::string {
    std::filesystem::path const current_file_path{__FILE__};
    auto const tests_dir{current_file_path.parent_path()};
    return (tests_dir / get_test_input_path_relative_to_tests_dir(test_input_path)).string();
}

void check_all_leaf_nodes_match_types(std::set<clp_s::NodeType> const& types) {
    clp_s::ArchiveReader archive_reader;
    for (auto const& entry : std::filesystem::directory_iterator(cTestEndToEndArchiveDirectory)) {
        REQUIRE_NOTHROW(archive_reader.open(
                clp_s::Path{
                        .source = clp_s::InputSource::Filesystem,
                        .path = entry.path().string()
                },
                clp_s::NetworkAuthOption{}
        ));
        auto const schema_tree{archive_reader.get_schema_tree()};
        REQUIRE(nullptr != schema_tree);
        auto const root{schema_tree->get_object_subtree_node_id_for_namespace(
                clp_s::constants::cDefaultNamespace
        )};
        REQUIRE(-1 != root);
        std::vector<int32_t> node_id_stack = {root};
        while (false == node_id_stack.empty()) {
            auto const cur_node_id{node_id_stack.back()};
            node_id_stack.pop_back();
            auto const& cur_node{schema_tree->get_node(cur_node_id)};

            if (false == cur_node.get_children_ids().empty()) {
                for (auto const child_id : cur_node.get_children_ids()) {
                    node_id_stack.emplace_back(child_id);
                }
                continue;
            }
            REQUIRE(0 != types.count(cur_node.get_type()));
        }

        REQUIRE_NOTHROW(archive_reader.close());
    }
}

void validate_archive_header() {
    clp_s::ArchiveReader archive_reader;
    for (auto const& entry : std::filesystem::directory_iterator(cTestEndToEndArchiveDirectory)) {
        REQUIRE_NOTHROW(archive_reader.open(
                clp_s::Path{
                        .source = clp_s::InputSource::Filesystem,
                        .path = entry.path().string()
                },
                clp_s::NetworkAuthOption{}
        ));
        auto const& archive_header{archive_reader.get_header()};
        REQUIRE(clp_s::cArchiveVersion == archive_header.version);
        auto const [major_version, minor_version, patch_version]
                = clp_s::decompose_archive_version(archive_header.version);
        REQUIRE(clp_s::cArchiveMajorVersion == major_version);
        REQUIRE(clp_s::cArchiveMinorVersion == minor_version);
        REQUIRE(clp_s::cArchivePatchVersion == patch_version);
        REQUIRE(archive_header.compressed_size > 0);
        REQUIRE(archive_header.uncompressed_size > 0);
        REQUIRE(0 == archive_header.padding);
        constexpr auto cReservedPaddingLength{
                sizeof(archive_header.reserved_padding) / sizeof(archive_header.reserved_padding[0])
        };
        for (size_t i{0}; i < cReservedPaddingLength; ++i) {
            REQUIRE(0ULL == archive_header.reserved_padding[i]);
        }

        REQUIRE_NOTHROW(archive_reader.close());
    }
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
            get_test_input_local_path(cTestEndToEndInputFile)
    );
    result = std::system(command.c_str());
    REQUIRE((true == WIFEXITED(result)));
    REQUIRE((0 == WEXITSTATUS(result)));
}

void literallyCompare(
        std::filesystem::path const& expected_output_json_path,
        std::filesystem::path const& extracted_json_path
) {
    auto command = fmt::format(
            "cat {} | awk 'NF' | tr -d ' ' | sort > {}",
            expected_output_json_path.string(),
            cTestEndToEndExpectedOutputSortedFile
    );
    auto result = std::system(command.c_str());
    REQUIRE((0 == result));
    REQUIRE((false == std::filesystem::is_empty(cTestEndToEndExpectedOutputSortedFile)));

    command = fmt::format(
            "cat {} | awk 'NF' | tr -d ' ' | sort > {}",
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
            cTestEndToEndExpectedOutputSortedFile,
            cTestEndToEndOutputSortedJson
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

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestEndToEndInputFile),
                    std::string{cTestEndToEndArchiveDirectory},
                    std::nullopt,
                    false,
                    single_file_archive,
                    structurize_arrays
            )
    );
    validate_archive_header();

    auto extracted_json_path = extract();

    compare(extracted_json_path);
}

/**
 * Tests that floats that can be represented as a `FormattedFloat` are retained accurately.
 */
TEST_CASE("clp-s-compress-extract-valid-formatted-floats", "[clp-s][end-to-end]") {
    auto structurize_arrays = GENERATE(true, false);
    auto single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{
            {std::string{cTestEndToEndArchiveDirectory},
             std::string{cTestEndToEndOutputDirectory},
             std::string{cTestEndToEndOutputSortedJson},
             std::string{cTestEndToEndExpectedOutputSortedFile}}
    };

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestEndToEndValidFormattedFloatInputFile),
                    std::string{cTestEndToEndArchiveDirectory},
                    std::nullopt,
                    true,
                    single_file_archive,
                    structurize_arrays
            )
    );
    validate_archive_header();

    auto const expected_matching_types{
            structurize_arrays ? std::set<clp_s::NodeType>{clp_s::NodeType::FormattedFloat}
                               : std::set<clp_s::NodeType>{
                                         clp_s::NodeType::FormattedFloat,
                                         clp_s::NodeType::UnstructuredArray
                                 }
    };
    check_all_leaf_nodes_match_types(expected_matching_types);

    auto extracted_json_path = extract();
    literallyCompare(
            get_test_input_local_path(cTestEndToEndValidFormattedFloatInputFile),
            extracted_json_path
    );
}

/**
 * Test that floats that are over 17 digits of precision, or are 17 digits of precision or less
 * but do not exactly represent the nearest decimal value for any ieee-754 binary64 at the given
 * precision, can be retained accurately.
 */
TEST_CASE("clp-s-compress-extract-invalid-formatted-floats", "[clp-s][end-to-end]") {
    auto structurize_arrays = GENERATE(true, false);
    auto single_file_archive = GENERATE(true, false);

    TestOutputCleaner const test_cleanup{
            {std::string{cTestEndToEndArchiveDirectory},
             std::string{cTestEndToEndOutputDirectory},
             std::string{cTestEndToEndOutputSortedJson},
             std::string{cTestEndToEndExpectedOutputSortedFile}}
    };

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestEndToEndInvalidFormattedFloatInputFile),
                    std::string{cTestEndToEndArchiveDirectory},
                    std::nullopt,
                    true,
                    single_file_archive,
                    structurize_arrays
            )
    );
    validate_archive_header();

    auto const expected_matching_types{
            structurize_arrays ? std::set<clp_s::NodeType>{clp_s::NodeType::DictionaryFloat}
                               : std::set<clp_s::NodeType>{
                                         clp_s::NodeType::DictionaryFloat,
                                         clp_s::NodeType::UnstructuredArray
                                 }
    };
    check_all_leaf_nodes_match_types(expected_matching_types);

    auto extracted_json_path = extract();
    literallyCompare(
            get_test_input_local_path(cTestEndToEndInvalidFormattedFloatInputFile),
            extracted_json_path
    );
}
