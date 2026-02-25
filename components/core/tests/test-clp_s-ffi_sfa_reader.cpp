#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

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

    REQUIRE_NOTHROW(
            std::ignore = compress_archive(
                    get_test_input_local_path(cTestInputFile),
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

    auto reader = clp_s::ffi::sfa::ClpArchiveReader::create(archive_path.string());
    REQUIRE(archive_path.filename().string() == reader->get_archive_id());
}
