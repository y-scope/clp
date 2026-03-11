#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "../src/clp/ReadOnlyMemoryMappedFile.hpp"
#include "../src/clp_s/ffi/sfa/ClpArchiveReader.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

namespace {
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
    std::filesystem::remove_all(output_dir);

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

auto assert_archive_event_count_matches_log(
        std::filesystem::path const& archive_path,
        uint64_t expected_event_count
) -> void {
    REQUIRE(std::filesystem::exists(archive_path));

    auto reader_result{clp_s::ffi::sfa::ClpArchiveReader::create_from_path(archive_path.string())};
    REQUIRE(false == reader_result.has_error());
    auto& reader = reader_result.value();

    auto const event_count{reader.get_event_count()};
    REQUIRE(event_count == expected_event_count);
}

auto assert_archive_event_count_matches_log_in_memory(
        std::filesystem::path const& archive_path,
        uint64_t expected_event_count
) -> void {
    REQUIRE(std::filesystem::exists(archive_path));

    auto mmap_result{clp::ReadOnlyMemoryMappedFile::create(archive_path.string())};
    REQUIRE(false == mmap_result.has_error());
    auto& mapped_archive = mmap_result.value();
    auto const view{mapped_archive.get_view()};
    REQUIRE(false == view.empty());

    auto reader_result{clp_s::ffi::sfa::ClpArchiveReader::create_from_bytes(
            std::vector<char>{view.begin(), view.end()}
    )};
    REQUIRE(false == reader_result.has_error());
    auto& reader = reader_result.value();

    auto const event_count{reader.get_event_count()};
    REQUIRE(event_count == expected_event_count);
}
}  // namespace

TEST_CASE("clp_s_ffi_sfa_reader_matches_test_no_floats_sorted", "[clp-s][ffi][sfa]") {
    TestOutputCleaner const test_cleanup{{get_archive_output_root_dir().string()}};
    auto const log_path{get_log_local_path(cInputNoFloats)};
    REQUIRE(std::filesystem::exists(log_path));
    auto const archive_path{generate_single_file_archive(log_path)};
    auto const expected_event_count{get_num_lines(log_path)};

    assert_archive_event_count_matches_log(archive_path, expected_event_count);
    assert_archive_event_count_matches_log_in_memory(archive_path, expected_event_count);
}

TEST_CASE("clp_s_ffi_sfa_reader_matches_test_search_float_timestamp", "[clp-s][ffi][sfa]") {
    TestOutputCleaner const test_cleanup{{get_archive_output_root_dir().string()}};
    auto const log_path{get_log_local_path(cInputFloatTimestamp)};
    REQUIRE(std::filesystem::exists(log_path));
    auto const archive_path{generate_single_file_archive(log_path)};
    auto const expected_event_count{get_num_lines(log_path)};

    assert_archive_event_count_matches_log(archive_path, expected_event_count);
    assert_archive_event_count_matches_log_in_memory(archive_path, expected_event_count);
}
