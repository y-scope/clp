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
#include "../src/clp_s/ffi/sfa/ClpArchiveDecoder.hpp"
#include "../src/clp_s/ffi/sfa/ClpArchiveReader.hpp"
#include "../src/clp_s/ffi/sfa/KqlQuery.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

namespace {
using clp::ReadOnlyMemoryMappedFile;
<<<<<<< HEAD
using clp_s::ffi::sfa::ClpArchiveDecoder;
using clp_s::ffi::sfa::ClpArchiveReader;
using clp_s::ffi::sfa::KqlQuery;
=======
using clp_s::ffi::sfa::ClpArchiveReader;
>>>>>>> main
using ystdlib::error_handling::Result;
using ystdlib::error_handling::success;

constexpr std::string_view cSfaReaderLogsDirectory{"test_log_files"};
constexpr std::string_view cSfaReaderArchiveOutputDirectory{"test-clp_s-ffi_sfa-reader-archive"};
constexpr std::string_view cInputNoFloats{"test_no_floats_sorted.jsonl"};
constexpr std::string_view cInputFloatTimestamp{"test_search_float_timestamp.jsonl"};
constexpr std::string_view cInputDistinctSchemaEachRow{"test_distinct_schema_each_row.jsonl"};

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

auto get_decoder_output_path() -> std::filesystem::path {
    return get_tests_dir().parent_path() / "build" / "decoder_test.txt";
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
    REQUIRE(0 == file_info.get_start_index());
    REQUIRE(static_cast<int64_t>(expected_event_count) == file_info.get_end_index());
    REQUIRE(expected_event_count == file_info.get_event_count());
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
    auto const r_path{YSTDLIB_ERROR_HANDLING_TRYX(create_reader_from_path(archive_path))};
    assert_reader_matches_expected(r_path, expected_file_name, expected_event_count);

    auto const r_bytes{YSTDLIB_ERROR_HANDLING_TRYX(create_reader_from_bytes(archive_path))};
    assert_reader_matches_expected(r_bytes, expected_file_name, expected_event_count);

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

TEST_CASE("clp_s_ffi_sfa_decoder_writes_decoded_lines", "[clp-s][ffi][sfa][decoder][tmp]") {
    TestOutputCleaner const test_cleanup{{get_archive_output_root_dir().string()}};

    auto const log_path{get_log_local_path(cInputDistinctSchemaEachRow)};
    REQUIRE(std::filesystem::exists(log_path));

    auto const archive_path{generate_single_file_archive(log_path)};
    REQUIRE(std::filesystem::exists(archive_path));

    auto reader_result{create_reader_from_path(archive_path)};
    REQUIRE(false == reader_result.has_error());
    auto reader{std::move(reader_result).value()};

    auto decoder_result{reader.decode_all()};
    REQUIRE(false == decoder_result.has_error());
    auto decoder{std::move(decoder_result).value()};

    auto decode_result{decoder.decode_all()};
    REQUIRE(false == decode_result.has_error());

    auto const& log_events{decoder.get_log_events()};
    REQUIRE(get_num_lines(log_path) == log_events.size());

    auto const output_path{get_decoder_output_path()};
    std::filesystem::create_directories(output_path.parent_path());
    std::ofstream output_file{output_path, std::ios::trunc};
    REQUIRE(output_file.is_open());

    for (auto const& log_event : log_events) {
        output_file << log_event.get_message();
    }

    std::vector<std::string> const queries{
            "service:api",
            "service:api or service:worker",
            "message:alpha or message:beta",
            // TODO: Re-enable once SchemaMatch handles this OR case over fully distinct schemas.
            // "message:alpha or message:beta or message:gamma",
            "service:api or error:timeout",
            "service:api or source:test",
            "sequence:9 or status:200",
            "retry_count:3 or level:INFO"
    };

    for (auto const& query_string : queries) {
        INFO("query_string=" << query_string);
        output_file << "---- query: " << query_string << "\n";

        auto query_result{KqlQuery::create(query_string)};
        REQUIRE(false == query_result.has_error());
        auto query{std::move(query_result).value()};

        auto search_result{reader.search(query)};
        REQUIRE(false == search_result.has_error());
        auto search_decoder{std::move(search_result).value()};

        auto search_decode_result{search_decoder.decode_all()};
        REQUIRE(false == search_decode_result.has_error());

        for (auto const& log_event : search_decoder.get_log_events()) {
            output_file << log_event.get_message();
        }
    }

    output_file.close();
    REQUIRE(std::filesystem::exists(output_path));
}
