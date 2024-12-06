#include <string>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/CheckpointReader.hpp"
#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/StringReader.hpp"

TEST_CASE("Test Checkpoint Reader", "[CheckpointReader]") {
    constexpr char cTestString[]{"0123456789"};
    constexpr size_t cTestStringLen{std::char_traits<char>::length(cTestString)};

    SECTION("CheckpointReader does not support try_read_to_delimiter") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::CheckpointReader checkpoint_reader{&string_reader, cTestStringLen};
        std::string tmp;
        REQUIRE(clp::ErrorCode_Unsupported
                == checkpoint_reader.try_read_to_delimiter('0', false, false, tmp));
    }

    SECTION("CheckpointReader does not allow reads beyond end of underlying stream.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::CheckpointReader checkpoint_reader{&string_reader, cTestStringLen + 1};
        char buf[cTestStringLen + 1];
        size_t num_bytes_read{};
        auto rc = checkpoint_reader.try_read(buf, cTestStringLen + 1, num_bytes_read);
        REQUIRE(clp::ErrorCode_Success == rc);
        REQUIRE(num_bytes_read == cTestStringLen);
        REQUIRE(cTestStringLen == string_reader.get_pos());
        REQUIRE(cTestStringLen == checkpoint_reader.get_pos());
    }

    SECTION("CheckpointReader does not allow reads beyond checkpoint.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::CheckpointReader checkpoint_reader{&string_reader, 1};
        char buf[cTestStringLen];
        size_t num_bytes_read{};
        auto rc = checkpoint_reader.try_read(buf, cTestStringLen, num_bytes_read);
        REQUIRE(clp::ErrorCode_Success == rc);
        REQUIRE(1 == num_bytes_read);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == checkpoint_reader.get_pos());
        rc = checkpoint_reader.try_read(buf, 1, num_bytes_read);
        REQUIRE(clp::ErrorCode_EndOfFile == rc);
        REQUIRE(0 == num_bytes_read);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == checkpoint_reader.get_pos());
    }

    SECTION("CheckpointReader does allow reads before checkpoint.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::CheckpointReader checkpoint_reader{&string_reader, 1};
        char buf{};
        size_t num_bytes_read{};
        auto rc = checkpoint_reader.try_read(&buf, 1, num_bytes_read);
        REQUIRE(clp::ErrorCode_Success == rc);
        REQUIRE(1 == num_bytes_read);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == checkpoint_reader.get_pos());
    }

    SECTION("CheckpointReader does not allow seeks beyond end of underlying stream.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::CheckpointReader checkpoint_reader{&string_reader, cTestStringLen + 1};
        auto rc = checkpoint_reader.try_seek_from_begin(cTestStringLen + 1);
        REQUIRE(clp::ErrorCode_EndOfFile == rc);
        REQUIRE(cTestStringLen == string_reader.get_pos());
        REQUIRE(cTestStringLen == checkpoint_reader.get_pos());
    }

    SECTION("CheckpointReader does not allow seeks beyond checkpoint.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::CheckpointReader checkpoint_reader{&string_reader, 1};
        char buf[cTestStringLen];
        size_t num_bytes_read{};
        auto rc = checkpoint_reader.try_seek_from_begin(cTestStringLen);
        REQUIRE(clp::ErrorCode_EndOfFile == rc);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == checkpoint_reader.get_pos());
    }

    SECTION("CheckpointReader does allow seeks before checkpoint.") {
        clp::StringReader string_reader;
        string_reader.open(cTestString);
        clp::CheckpointReader checkpoint_reader{&string_reader, 2};
        char buf[cTestStringLen];
        size_t num_bytes_read{};
        auto rc = checkpoint_reader.try_seek_from_begin(1);
        REQUIRE(clp::ErrorCode_Success == rc);
        REQUIRE(1 == string_reader.get_pos());
        REQUIRE(1 == checkpoint_reader.get_pos());
    }
}
