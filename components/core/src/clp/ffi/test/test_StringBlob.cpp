#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "../../BufferReader.hpp"
#include "../../ErrorCode.hpp"
#include "../StringBlob.hpp"

namespace clp::ffi::test {
TEST_CASE("StringBlob basic functionality", "[StringBlob]") {
    StringBlob string_blob;

    std::vector<std::string> const test_strings{
            "Hello, World!",
            "This is a test string.",
            "StringBlob is working correctly.",
    };

    std::string buffer;
    for (auto const& str : test_strings) {
        buffer += str;
    }
    BufferReader reader{buffer.data(), buffer.size()};

    size_t expected_num_strings{0};
    for (auto const& expected_str : test_strings) {
        REQUIRE((expected_num_strings == string_blob.get_num_strings()));

        auto const result{string_blob.read_from(reader, expected_str.size())};
        REQUIRE_FALSE(result.has_value());
        ++expected_num_strings;
        REQUIRE((expected_num_strings == string_blob.get_num_strings()));
        auto const optional_retrieved_str{string_blob.get_string(expected_num_strings - 1)};
        REQUIRE((optional_retrieved_str.has_value()));
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        REQUIRE((optional_retrieved_str.value() == expected_str));
    }

    auto const read_from_eof{string_blob.read_from(reader, 1)};
    REQUIRE(read_from_eof.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    REQUIRE((clp::ErrorCode::ErrorCode_EndOfFile == read_from_eof.value()));
}
}  // namespace clp::ffi::test
