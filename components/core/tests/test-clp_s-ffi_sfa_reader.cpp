#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "../src/clp_s/ffi/sfa/ClpArchiveReader.hpp"
#include "clp_s_test_utils.hpp"
#include "TestOutputCleaner.hpp"

namespace {
constexpr std::string_view cTestArchiveDirectory{"test-clp_s-ffi-sfa-reader-archive"};
constexpr std::string_view cTestInputFile{"test_no_floats_sorted.jsonl"};
}  // namespace

TEST_CASE("clp_s_ffi_sfa_reader_get_archive_id", "[clp-s][ffi][sfa]") {
    TestOutputCleaner const test_cleanup{{std::string{cTestArchiveDirectory}}};
    auto const input_path = get_test_input_local_path(cTestInputFile);

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    input_path,
                    std::string{cTestArchiveDirectory},
                    std::nullopt,
                    false,
                    true,
                    false
            )
    );

    auto const archive_path = std::filesystem::directory_iterator{
            std::filesystem::path{cTestArchiveDirectory}
    }->path();

    auto reader_result = clp_s::ffi::sfa::ClpArchiveReader::create(archive_path.string());
    REQUIRE_FALSE(reader_result.has_error());
    auto& reader = reader_result.value();
    REQUIRE(archive_path.filename().string() == reader.get_archive_id());

    uint64_t expected_event_count{0};
    {
        std::ifstream input_file{input_path};
        std::string line;
        while (std::getline(input_file, line)) {
            ++expected_event_count;
        }
    }
    REQUIRE(expected_event_count == reader.get_event_count());
}
