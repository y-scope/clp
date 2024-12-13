#include <array>
#include <cstddef>
#include <string>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/BoundedReader.hpp"
#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/StringReader.hpp"

TEST_CASE("Test Bounded Reader", "[BoundedReader]") {
    constexpr char cTestString[]{"0123456789"};
    constexpr size_t cTestStringLen{std::char_traits<char>::length(cTestString)};

    SECTION("BoundedReader does not support try_read_to_delimiter") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::BoundedReader bounded_reader{&string_reader, cTestStringLen};
        std::string tmp;
        REQUIRE(clp::ErrorCode_Unsupported
                == bounded_reader.try_read_to_delimiter('0', false, false, tmp));
    }

    SECTION("BoundedReader does not allow reads beyond end of underlying stream.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::BoundedReader bounded_reader{&string_reader, cTestStringLen + 1};
        std::array<char, cTestStringLen + 1> buf{};
        size_t num_bytes_read{};
        auto rc = bounded_reader.try_read(buf.data(), cTestStringLen + 1, num_bytes_read);
        REQUIRE(clp::ErrorCode_Success == rc);
        REQUIRE(num_bytes_read == cTestStringLen);
        REQUIRE(cTestStringLen == string_reader.get_pos());
        REQUIRE(cTestStringLen == bounded_reader.get_pos());
    }

    SECTION("BoundedReader does not allow reads beyond checkpoint.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::BoundedReader bounded_reader{&string_reader, 1};
        std::array<char, cTestStringLen> buf{};
        size_t num_bytes_read{};
        auto rc = bounded_reader.try_read(buf.data(), cTestStringLen, num_bytes_read);
        REQUIRE(clp::ErrorCode_Success == rc);
        REQUIRE(1 == num_bytes_read);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == bounded_reader.get_pos());
        rc = bounded_reader.try_read(buf.data(), 1, num_bytes_read);
        REQUIRE(clp::ErrorCode_EndOfFile == rc);
        REQUIRE(0 == num_bytes_read);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == bounded_reader.get_pos());
    }

    SECTION("BoundedReader does allow reads before checkpoint.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::BoundedReader bounded_reader{&string_reader, 1};
        char buf{};
        size_t num_bytes_read{};
        auto rc = bounded_reader.try_read(&buf, 1, num_bytes_read);
        REQUIRE(clp::ErrorCode_Success == rc);
        REQUIRE(1 == num_bytes_read);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == bounded_reader.get_pos());
    }

    SECTION("BoundedReader does not allow seeks beyond end of underlying stream.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::BoundedReader bounded_reader{&string_reader, cTestStringLen + 1};
        auto rc = bounded_reader.try_seek_from_begin(cTestStringLen + 1);
        REQUIRE(clp::ErrorCode_EndOfFile == rc);
        REQUIRE(cTestStringLen == string_reader.get_pos());
        REQUIRE(cTestStringLen == bounded_reader.get_pos());
    }

    SECTION("BoundedReader does not allow seeks beyond checkpoint.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::BoundedReader bounded_reader{&string_reader, 1};
        size_t num_bytes_read{};
        auto rc = bounded_reader.try_seek_from_begin(cTestStringLen);
        REQUIRE(clp::ErrorCode_EndOfFile == rc);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == bounded_reader.get_pos());
    }

    SECTION("BoundedReader does allow seeks before checkpoint.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::BoundedReader bounded_reader{&string_reader, 2};
        size_t num_bytes_read{};
        auto rc = bounded_reader.try_seek_from_begin(1);
        REQUIRE(clp::ErrorCode_Success == rc);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == bounded_reader.get_pos());
    }
}
