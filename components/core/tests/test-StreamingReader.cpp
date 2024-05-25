#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <curl/curl.h>

#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/FileReader.hpp"
#include "../src/clp/ReaderInterface.hpp"
#include "../src/clp/StreamingReader.hpp"

namespace {
constexpr char const* cTestUrl{
        "https://raw.githubusercontent.com/y-scope/clp/main/components/core/tests/"
        "test_network_reader_src/random.log"
};

constexpr size_t cDefaultReaderBufferSize{1024};

[[nodiscard]] auto get_ref_file_abs_path() -> std::filesystem::path {
    std::filesystem::path const file_path{__FILE__};
    auto const test_root_path{file_path.parent_path()};
    return test_root_path / "test_network_reader_src" / "random.log";
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
        size_t reader_buffer_size = cDefaultReaderBufferSize
) -> void {
    in_mem_buf.clear();
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    auto const read_buffer{std::make_unique<char[]>(reader_buffer_size)};
    size_t num_bytes_read{};
    bool has_more_content{true};
    while (has_more_content) {
        has_more_content = reader.read(read_buffer.get(), reader_buffer_size, num_bytes_read);
        std::string_view const view{read_buffer.get(), num_bytes_read};
        in_mem_buf.insert(in_mem_buf.cend(), view.cbegin(), view.cend());
    }
}

/**
 * Runs a function with a timeout.
 * @tparam Func The function to run. A lambda function is expected, which takes no parameter.
 * @param timeout
 * @param func Function to run.
 * @return Whether the function completes before the timeout is triggered.
 */
template <typename Func>
[[nodiscard]] auto run_with_timeout(std::chrono::milliseconds timeout, Func&& func) -> bool {
    auto future{std::async(std::launch::async, std::forward<Func>(func))};
    return future.wait_for(timeout) == std::future_status::ready;
}
}  // namespace

TEST_CASE("streaming_reader_basic", "[StreamingReader]") {
    clp::FileReader ref_reader;
    ref_reader.open(get_ref_file_abs_path().string());
    std::vector<char> ref_data;
    read_into_memory_buffer(ref_reader, ref_data);
    ref_reader.close();

    REQUIRE((clp::ErrorCode_Success == clp::StreamingReader::init()));
    clp::StreamingReader reader(cTestUrl);
    std::vector<char> streamed_data;
    read_into_memory_buffer(reader, streamed_data);
    auto const ret_code{reader.get_curl_return_code()};
    REQUIRE(ret_code.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    REQUIRE((CURLE_OK == ret_code.value()));
    REQUIRE((streamed_data == ref_data));
    clp::StreamingReader::deinit();
}

TEST_CASE("streaming_reader_with_offset_and_seek", "[StreamingReader]") {
    constexpr size_t cOffset{319};
    clp::FileReader ref_reader;
    ref_reader.open(get_ref_file_abs_path().string());
    ref_reader.seek_from_begin(cOffset);
    std::vector<char> ref_data;
    read_into_memory_buffer(ref_reader, ref_data);
    auto const ref_end_pos{ref_reader.get_pos()};
    ref_reader.close();

    REQUIRE((clp::ErrorCode_Success == clp::StreamingReader::init()));
    std::vector<char> streamed_data;

    // Read from an offset onwards by starting the download from that offset.
    {
        clp::StreamingReader reader_using_offset(cTestUrl, cOffset);
        read_into_memory_buffer(reader_using_offset, streamed_data);
        auto const ret_code_using_offset{reader_using_offset.get_curl_return_code()};
        REQUIRE(ret_code_using_offset.has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        REQUIRE((CURLE_OK == ret_code_using_offset.value()));
        REQUIRE((reader_using_offset.get_pos() == ref_end_pos));
        REQUIRE((streamed_data == ref_data));
        streamed_data.clear();
    }

    // Read from an offset onwards by seeking to that offset.
    {
        clp::StreamingReader reader_using_seek(cTestUrl);
        reader_using_seek.seek_from_begin(cOffset);
        read_into_memory_buffer(reader_using_seek, streamed_data);
        auto const ret_code_using_seek{reader_using_seek.get_curl_return_code()};
        REQUIRE(ret_code_using_seek.has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        REQUIRE((CURLE_OK == ret_code_using_seek.value()));
        REQUIRE((reader_using_seek.get_pos() == ref_end_pos));
        REQUIRE((streamed_data == ref_data));
        streamed_data.clear();
    }

    clp::StreamingReader::deinit();
}

TEST_CASE("streaming_reader_destruct", "[StreamingReader]") {
    REQUIRE((clp::ErrorCode_Success == clp::StreamingReader::init()));

    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    bool peacefully_destructed{false};
    auto test_abort = [&]() -> void {
        // We sleep for a while to fill out all the buffers, and we delete the reader. The
        // destructor should be called to abort the underlying transfer session. We should ensure
        // destructor is successfully executed without deadlock or exceptions in this case.
        try {
            auto reader{std::make_unique<clp::StreamingReader>(
                    cTestUrl,
                    0,
                    true,
                    clp::StreamingReader::cDefaultOverallTimeout,
                    clp::StreamingReader::cDefaultConnectionTimeout,
                    3,
                    512
            )};
            std::this_thread::sleep_for(std::chrono::seconds{1});
            REQUIRE(reader->is_download_in_progress());
            reader.reset(nullptr);
        } catch (clp::StreamingReader::OperationFailed const& ex) {
            return;
        }
        peacefully_destructed = true;
    };

    REQUIRE(run_with_timeout(std::chrono::milliseconds{1500}, test_abort));
    REQUIRE(peacefully_destructed);
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

    clp::StreamingReader::deinit();
}
