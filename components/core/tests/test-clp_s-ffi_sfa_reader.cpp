#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
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

auto assert_reader_matches_expected(
        ClpArchiveReader const& reader,
        std::string const& expected_file_name,
        uint64_t expected_event_count
) -> void {
    auto const event_count{reader.get_event_count()};
    REQUIRE(event_count == expected_event_count);

    auto const file_names{reader.get_file_names()};
    auto const& file_infos{reader.get_file_infos()};
    REQUIRE(1 == file_names.size());
    REQUIRE(1 == file_infos.size());

    auto const& file_name{file_names.front()};
    auto const& file_info{file_infos.front()};
    REQUIRE(expected_file_name == file_name);
    REQUIRE(expected_file_name == file_info.get_file_name());
    REQUIRE(0LL == file_info.get_start_index());
    REQUIRE(static_cast<int64_t>(expected_event_count) == file_info.get_end_index());
    REQUIRE(expected_event_count == file_info.get_event_count());
}

auto
assert_decoded_log_event_idx_matches_index(ClpArchiveReader& reader, uint64_t expected_event_count)
        -> Result<void> {
    auto const decoded_events{YSTDLIB_ERROR_HANDLING_TRYX(reader.decode())};
    REQUIRE(expected_event_count == static_cast<uint64_t>(decoded_events.size()));

    int64_t expected_log_event_idx{0};
    for (auto const& decoded_event : decoded_events) {
        REQUIRE(expected_log_event_idx == decoded_event.get_log_event_idx());
        ++expected_log_event_idx;
    }
    return success();
}

auto create_reader_from_path(std::filesystem::path const& archive_path)
        -> Result<ClpArchiveReader> {
    return ClpArchiveReader::create(archive_path.string());
}

auto create_reader_from_bytes(std::filesystem::path const& archive_path)
        -> Result<ClpArchiveReader> {
    auto const mapped_archive{
            YSTDLIB_ERROR_HANDLING_TRYX(ReadOnlyMemoryMappedFile::create(archive_path.string()))
    };
    auto const view{mapped_archive.get_view()};
    REQUIRE(false == view.empty());
    return ClpArchiveReader::create(std::vector<char>{view.begin(), view.end()});
}

auto run_single_log_file_test(
        std::filesystem::path const& archive_path,
        std::string const& expected_file_name,
        uint64_t expected_event_count
) -> Result<void> {
    auto r_path{YSTDLIB_ERROR_HANDLING_TRYX(create_reader_from_path(archive_path))};
    assert_reader_matches_expected(r_path, expected_file_name, expected_event_count);
    YSTDLIB_ERROR_HANDLING_TRYV(
            assert_decoded_log_event_idx_matches_index(r_path, expected_event_count)
    );

    auto r_bytes{YSTDLIB_ERROR_HANDLING_TRYX(create_reader_from_bytes(archive_path))};
    assert_reader_matches_expected(r_bytes, expected_file_name, expected_event_count);
    YSTDLIB_ERROR_HANDLING_TRYV(
            assert_decoded_log_event_idx_matches_index(r_bytes, expected_event_count)
    );

    return success();
}
}  // namespace

TEST_CASE("clp_s_ffi_sfa_reader", "[clp-s][ffi][sfa]") {
    TestOutputCleaner const test_cleanup{{get_archive_output_root_dir().string()}};

    auto const log_file_name{GENERATE(cInputNoFloats, cInputFloatTimestamp)};

    auto const log_path{get_log_local_path(log_file_name)};
    REQUIRE(std::filesystem::exists(log_path));

    auto const archive_path{generate_single_file_archive(log_path)};
    REQUIRE(std::filesystem::exists(archive_path));

    auto const expected_event_count{get_num_lines(log_path)};
    auto const test_result{
            run_single_log_file_test(archive_path, log_path.string(), expected_event_count)
    };
    REQUIRE(false == test_result.has_error());
}
