#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <msgpack.hpp>

#include "../src/clp/BufferReader.hpp"
#include "../src/clp/ffi/ir_stream/decoding_methods.hpp"
#include "../src/clp/ffi/ir_stream/Deserializer.hpp"
#include "../src/clp/ffi/ir_stream/Serializer.hpp"
#include "../src/clp/ffi/KeyValuePairLogEvent.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp/time_types.hpp"
#include "../src/clp_s/JsonConstructor.hpp"
#include "../src/clp_s/JsonParser.hpp"

using clp::BufferReader;
using clp::ffi::ir_stream::Deserializer;
using clp::ffi::ir_stream::IRErrorCode;
using clp::ffi::ir_stream::Serializer;
using clp::ffi::KeyValuePairLogEvent;
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using clp::size_checked_pointer_cast;
using clp::UtcOffset;
using std::string;
using std::string_view;
using std::vector;

auto const cDefaultTargetEncodedSize = 8ULL * 1024 * 1024 * 1024;
auto const cDefaultMaxDocumentSize = 512ULL * 1024 * 1024;
auto const cDefaultMinTableSize = 1ULL * 1024 * 1024;
auto const cDeaultCompressionLevel = 3;
auto const cDefaultPrintArchiveStats = false;
auto const cDefaultStructurizeArrays = false;

namespace {
auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path {
    return std::filesystem::path{"test_log_files"} / "test_no_floats_sorted.json";
}

auto get_test_input_local_path() -> std::string {
    std::filesystem::path const current_file_path{__FILE__};
    auto const tests_dir{current_file_path.parent_path()};
    return (tests_dir / get_test_input_path_relative_to_tests_dir()).string();
}
}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEMPLATE_TEST_CASE(
        "clp-s_compression_and_extraction_no_floats",
        "[clp-s][end-to-end]",
        four_byte_encoded_variable_t,
        eight_byte_encoded_variable_t
) {
    std::filesystem::remove_all("test-end-to-end-archive");
    std::filesystem::remove_all("test-end-to-end-out");
    std::filesystem::remove("test-end-to-end_sorted.json");
    std::filesystem::remove("diff_out.txt");

    std::filesystem::create_directory("test-end-to-end-archive");
    REQUIRE(std::filesystem::is_directory("test-end-to-end-archive"));

    clp_s::JsonParserOption parser_option{};
    parser_option.file_paths.push_back(get_test_input_local_path());
    parser_option.archives_dir = "test-end-to-end-archive";
    parser_option.target_encoded_size = cDefaultTargetEncodedSize;
    parser_option.max_document_size = cDefaultMaxDocumentSize;
    parser_option.min_table_size = cDefaultMinTableSize;
    parser_option.compression_level = cDeaultCompressionLevel;
    parser_option.print_archive_stats = cDefaultPrintArchiveStats;
    parser_option.structurize_arrays = cDefaultStructurizeArrays;

    clp_s::JsonParser parser(parser_option);
    REQUIRE(parser.parse());
    parser.store();

    REQUIRE(false == std::filesystem::is_empty("test-end-to-end-archive"));

    std::filesystem::create_directory("test-end-to-end-out");
    REQUIRE(std::filesystem::is_directory("test-end-to-end-out"));

    clp_s::JsonConstructorOption constructor_option{};
    constructor_option.output_dir = "test-end-to-end-out";
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

    REQUIRE(std::filesystem::exists("test-end-to-end-out/original"));

    int result = std::system("command -v jq >/dev/null 2>&1");
    REQUIRE(0 == result);
    result = std::system(
            "jq -S -c '.' test-end-to-end-out/original | sort > test-end-to-end_sorted.json"
    );
    REQUIRE(0 == result);

    REQUIRE(false == std::filesystem::is_empty("test-end-to-end_sorted.json"));

    result = std::system("command -v diff >/dev/null 2>&1");
    REQUIRE(0 == result);
    std::string const command = "diff -u test-end-to-end_sorted.json " + get_test_input_local_path()
                                + " > diff_out.txt";
    //std::cout << command << "\n";
    result = std::system(command.c_str());
    //std::cout << result << "\n";
    std::cout << WEXITSTATUS(result) << "\n";
    std::system("cat test-end-to-end-out/original");
    std::system("cat diff_out.txt");
    REQUIRE((0 == WEXITSTATUS(result) || 1 == WEXITSTATUS(result)));

    REQUIRE(std::filesystem::is_empty("diff_out.txt"));
}
