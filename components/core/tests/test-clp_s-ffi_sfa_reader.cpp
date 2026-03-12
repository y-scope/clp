#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../src/clp/ReadOnlyMemoryMappedFile.hpp"
#include "../src/clp_s/ffi/sfa/ClpArchiveReader.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

namespace {
using clp::ReadOnlyMemoryMappedFile;
using clp_s::ffi::sfa::ClpArchiveReader;
using ystdlib::error_handling::Result;
using ystdlib::error_handling::success;

constexpr std::string_view cSfaReaderLogsDirectory{"test_log_files"};
constexpr std::string_view cSfaReaderArchiveOutputDirectory{"test-clp_s-ffi_sfa-reader-archive"};
constexpr std::string_view cInputNoFloats{"test_no_floats_sorted.jsonl"};
constexpr std::string_view cInputFloatTimestamp{"test_search_float_timestamp.jsonl"};

auto get_tests_dir() -> std::filesystem::path {
    std::filesystem::path const current_file_path{__FILE__};
    return current_file_path.parent_path();
}

auto get_log_local_path(std::string_view const log_name) -> std::filesystem::path {
    return get_tests_dir() / cSfaReaderLogsDirectory / log_name;
}

auto get_archive_output_root_dir() -> std::filesystem::path {
    return get_tests_dir() / cSfaReaderArchiveOutputDirectory;
}

auto generate_single_file_archive(std::filesystem::path const& log_path) -> std::filesystem::path {
    auto const root_output_dir{get_archive_output_root_dir()};
    std::filesystem::create_directories(root_output_dir);

    auto const output_dir{root_output_dir / log_path.stem().string()};

    auto const archive_stats = compress_archive(
            log_path.string(),
            output_dir.string(),
            std::nullopt,
            false,
            true,
            false
    );
    REQUIRE(false == archive_stats.empty());
    return output_dir / archive_stats.front().get_id();
}

auto get_num_lines(std::filesystem::path const& path) -> uint64_t {
    std::ifstream input_file{path};
    REQUIRE(input_file.is_open());

    uint64_t num_lines{0};
    std::string line;
    while (std::getline(input_file, line)) {
        ++num_lines;
    }
    return num_lines;
}

auto assert_reader_matches_expected(ClpArchiveReader const& reader, uint64_t expected_event_count)
        -> void {
    auto const event_count{reader.get_event_count()};
    REQUIRE(event_count == expected_event_count);

    auto const file_names{reader.get_file_names()};
    auto const& file_infos{reader.get_file_infos()};
    REQUIRE(1 == file_names.size());
    REQUIRE(1 == file_infos.size());

    auto const& file_name{file_names.front()};
    auto const& file_info{file_infos.front()};
    REQUIRE(false == file_name.empty());
    REQUIRE(file_info.get_file_name() == file_name);
    REQUIRE(0 == file_info.get_start_index());
    REQUIRE(expected_event_count == file_info.get_end_index());
    REQUIRE(expected_event_count == file_info.get_event_count());
}

auto test_archive_created_from_path(
        std::filesystem::path const& archive_path,
        uint64_t expected_event_count
) -> Result<void> {
    auto reader{YSTDLIB_ERROR_HANDLING_TRYX(ClpArchiveReader::create(archive_path.string()))};
    assert_reader_matches_expected(reader, expected_event_count);
    return success();
}

auto test_archive_created_from_bytes(
        std::filesystem::path const& archive_path,
        uint64_t expected_event_count
) -> Result<void> {
    auto mapped_archive{
            YSTDLIB_ERROR_HANDLING_TRYX(ReadOnlyMemoryMappedFile::create(archive_path.string()))
    };
    auto const view{mapped_archive.get_view()};
    REQUIRE(false == view.empty());

    auto reader{YSTDLIB_ERROR_HANDLING_TRYX(
            ClpArchiveReader::create(std::vector<char>{view.begin(), view.end()})
    )};
    assert_reader_matches_expected(reader, expected_event_count);
    return success();
}

auto run_single_log_file_case(std::string_view log_name) -> Result<void> {
    auto const log_path{get_log_local_path(log_name)};
    REQUIRE(std::filesystem::exists(log_path));

    auto const archive_path{generate_single_file_archive(log_path)};
    REQUIRE(std::filesystem::exists(archive_path));

    auto const expected_event_count{get_num_lines(log_path)};

    YSTDLIB_ERROR_HANDLING_TRYV(test_archive_created_from_path(archive_path, expected_event_count));
    YSTDLIB_ERROR_HANDLING_TRYV(
            test_archive_created_from_bytes(archive_path, expected_event_count)
    );
    return success();
}
}  // namespace

TEST_CASE("clp_s_ffi_sfa_reader", "[clp-s][ffi][sfa]") {
    TestOutputCleaner const test_cleanup{{get_archive_output_root_dir().string()}};

    SECTION("test_no_floats_sorted") {
        auto const test_result{run_single_log_file_case(cInputNoFloats)};
        REQUIRE(false == test_result.has_error());
    }

    SECTION("test_search_float_timestamp") {
        auto const test_result{run_single_log_file_case(cInputFloatTimestamp)};
        REQUIRE(false == test_result.has_error());
    }
}
