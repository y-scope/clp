#include <Catch2/single_include/catch2/catch.hpp>
#include <iostream>
#include <string_view>

#include "../src/clp/StreamingReader.hpp"

using clp::StreamingReader;

namespace {
constexpr char cTestUrl[]{
        "https://yscope.s3.us-east-2.amazonaws.com/sample-logs/"
        "yarn-ubuntu-resourcemanager-ip-172-31-17-135.log.1.clp.zst"
};
}

TEST_CASE("streaming_reader_basic", "[StreamingReader]") {
    REQUIRE(clp::ErrorCode_Success == clp::StreamingReader::global_init());
    constexpr size_t cReaderBufferSize{1024};
    auto const read_buffer{std::make_unique<char[]>(cReaderBufferSize)};
    
    clp::StreamingReader reader;
    reader.open(cTestUrl);
    size_t num_bytes_read{};
    while (true) {
        auto const ret{reader.read(read_buffer.get(), cReaderBufferSize, num_bytes_read)};
        std::string_view view{read_buffer.get(), num_bytes_read};
        // std::cerr << view;
        if (false == ret) {
            break;
        }
    }
}