#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "../src/clp/ReadOnlyMemoryMappedFile.hpp"
#include "../src/clp_s/ffi/sfa/ClpArchiveReader.hpp"

namespace {
constexpr std::string_view cSfaReaderLogsDirectory{"test_log_files"};
constexpr std::string_view cSfaReaderArchivesDirectory{"test_single_file_archives"};
constexpr std::string_view cArchiveNoFloats{"test_no_floats_sorted.clp"};
constexpr std::string_view cArchiveFloatTimestamp{"test_search_float_timestamp.clp"};

auto get_tests_dir() -> std::filesystem::path {
    std::filesystem::path const current_file_path{__FILE__};
    return current_file_path.parent_path();
}

auto get_archive_local_path(std::string_view const archive_name) -> std::filesystem::path {
    return get_tests_dir() / cSfaReaderArchivesDirectory / archive_name;
}

auto get_log_local_path_from_archive_name(std::string_view const archive_name)
        -> std::filesystem::path {
    std::filesystem::path const archive_path{archive_name};
    auto log_name = archive_path.stem().string() + ".jsonl";
    return get_tests_dir() / cSfaReaderLogsDirectory / log_name;
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

auto assert_archive_event_count_matches_log(std::string_view const archive_name) -> void {
    auto const archive_path = get_archive_local_path(archive_name);
    auto const log_path = get_log_local_path_from_archive_name(archive_name);
    REQUIRE(std::filesystem::exists(archive_path));
    REQUIRE(std::filesystem::exists(log_path));

    auto reader_result = clp_s::ffi::sfa::ClpArchiveReader::create_from_path(archive_path.string());
    REQUIRE(false == reader_result.has_error());
    auto& reader = reader_result.value();

    auto const expected_event_count = get_num_lines(log_path);
    auto const event_count = reader.get_event_count();
    REQUIRE(event_count == expected_event_count);
}

auto assert_archive_event_count_matches_log_in_memory(std::string_view const archive_name) -> void {
    auto const archive_path = get_archive_local_path(archive_name);
    auto const log_path = get_log_local_path_from_archive_name(archive_name);
    REQUIRE(std::filesystem::exists(archive_path));
    REQUIRE(std::filesystem::exists(log_path));

    auto mmap_result = clp::ReadOnlyMemoryMappedFile::create(archive_path.string());
    REQUIRE(false == mmap_result.has_error());
    auto& mapped_archive = mmap_result.value();
    auto const view = mapped_archive.get_view();
    REQUIRE(false == view.empty());

    auto reader_result = clp_s::ffi::sfa::ClpArchiveReader::create_from_bytes(
            std::span<char const>{view.data(), view.size()}
    );
    REQUIRE(false == reader_result.has_error());
    auto& reader = reader_result.value();

    auto const expected_event_count = get_num_lines(log_path);

    auto const event_count = reader.get_event_count();
    REQUIRE(event_count == expected_event_count);
}
}  // namespace

TEST_CASE("clp_s_ffi_sfa_reader_matches_test_no_floats_sorted", "[clp-s][ffi][sfa]") {
    assert_archive_event_count_matches_log(cArchiveNoFloats);
    assert_archive_event_count_matches_log_in_memory(cArchiveNoFloats);
}

TEST_CASE("clp_s_ffi_sfa_reader_matches_test_search_float_timestamp", "[clp-s][ffi][sfa]") {
    assert_archive_event_count_matches_log(cArchiveFloatTimestamp);
    assert_archive_event_count_matches_log_in_memory(cArchiveFloatTimestamp);
}
