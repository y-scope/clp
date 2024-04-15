#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/FileReader.hpp"
#include "../src/clp/ReaderInterface.hpp"
#include "../src/clp/StreamingReader.hpp"

namespace {
/**
 * @return The src url that the StreamingReader will stream from.
 */
[[nodiscard]] auto get_test_src_url() -> std::string_view {
    static constexpr char cTestUrl[]{
            "https://raw.githubusercontent.com/y-scope/clp/main/components/core/tests/"
            "test_network_reader_src/random.log"
    };
    return cTestUrl;
}

[[nodiscard]] auto get_ref_file_abs_path() -> std::filesystem::path {
    std::filesystem::path const file_path{__FILE__};
    auto const test_root_path{file_path.parent_path()};
    return test_root_path / "test_network_reader_src/random.log";
}

/**
 * Reads the content of a given reader into a memory buffer.
 * @param reader
 * @param in_mem_buf
 * @param reader_buffer_size The size of the buffer used to create a buffer for the underlying
 * `read` operation.
 */
auto read_into_memory_buffer(
        clp::ReaderInterface& reader,
        std::vector<char>& in_mem_buf,
        size_t reader_buffer_size = 1024
) -> void {
    in_mem_buf.clear();
    auto const read_buffer{std::make_unique<char[]>(reader_buffer_size)};
    size_t num_bytes_read{};
    bool has_more_content{true};
    while (has_more_content) {
        has_more_content = reader.read(read_buffer.get(), reader_buffer_size, num_bytes_read);
        std::string_view view{read_buffer.get(), num_bytes_read};
        in_mem_buf.insert(in_mem_buf.cend(), view.cbegin(), view.cend());
    }
}
}  // namespace

TEST_CASE("streaming_reader_basic", "[StreamingReader]") {
    clp::FileReader ref_reader;
    ref_reader.open(get_ref_file_abs_path().string());
    std::vector<char> ref_data;
    read_into_memory_buffer(ref_reader, ref_data);
    ref_reader.close();

    REQUIRE(clp::ErrorCode_Success == clp::StreamingReader::global_init());
    clp::StreamingReader reader;
    std::vector<char> streamed_data;
    reader.open(get_test_src_url());
    read_into_memory_buffer(reader, streamed_data);
    reader.close();

    REQUIRE(streamed_data == ref_data);
}
