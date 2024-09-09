#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <curl/curl.h>

#include "../src/clp/Array.hpp"
#include "../src/clp/CurlDownloadHandler.hpp"
#include "../src/clp/CurlGlobalInstance.hpp"
#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/FileReader.hpp"
#include "../src/clp/NetworkReader.hpp"
#include "../src/clp/Platform.hpp"
#include "../src/clp/ReaderInterface.hpp"

namespace {
constexpr size_t cDefaultReaderBufferSize{1024};

[[nodiscard]] auto get_test_input_local_path() -> std::string;

[[nodiscard]] auto get_test_input_remote_url() -> std::string;

[[nodiscard]] auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path;

/**
 * @param reader
 * @param read_buf_size The size of the buffer to use for individual reads from the reader.
 * @return All data read from the given reader.
 */
auto get_content(clp::ReaderInterface& reader, size_t read_buf_size = cDefaultReaderBufferSize)
        -> std::vector<char>;

/**
 * Asserts whether the given `CURLcode` and the CURL return code stored in the given `NetworkReader`
 * instance are the same, and prints a log message if not.
 * @param expected
 * @param reader
 * @return Whether the the assertion succeeded.
 */
[[nodiscard]] auto
assert_curl_error_code(CURLcode expected, clp::NetworkReader const& reader) -> bool;

auto get_test_input_local_path() -> std::string {
    std::filesystem::path const current_file_path{__FILE__};
    auto const tests_dir{current_file_path.parent_path()};
    return (tests_dir / get_test_input_path_relative_to_tests_dir()).string();
}

auto get_test_input_remote_url() -> std::string {
    auto const input_path_relative_to_repo = std::filesystem::path{"components"} / "core" / "tests"
                                             / get_test_input_path_relative_to_tests_dir();
    return std::string{"https://raw.githubusercontent.com/y-scope/clp/main/"}
           + input_path_relative_to_repo.string();
}

auto get_test_input_path_relative_to_tests_dir() -> std::filesystem::path {
    return std::filesystem::path{"test_network_reader_src"} / "random.log";
}

auto get_content(clp::ReaderInterface& reader, size_t read_buf_size) -> std::vector<char> {
    std::vector<char> buf;
    clp::Array<char> read_buf{read_buf_size};
    for (bool has_more_content{true}; has_more_content;) {
        size_t num_bytes_read{};
        has_more_content = reader.read(read_buf.data(), read_buf_size, num_bytes_read);
        std::string_view const view{read_buf.data(), num_bytes_read};
        buf.insert(buf.cend(), view.cbegin(), view.cend());
    }
    return buf;
}

auto assert_curl_error_code(CURLcode expected, clp::NetworkReader const& reader) -> bool {
    auto const ret_code{reader.get_curl_ret_code()};
    if (false == ret_code.has_value()) {
        WARN("The CURL error code hasn't been set yet in the given reader.");
        return false;
    }
    auto const actual{ret_code.value()};
    if (expected == actual) {
        return true;
    }
    std::string message_to_log{
            "Unexpected CURL error code: " + std::to_string(actual)
            + "; expected: " + std::to_string(expected)
    };
    auto const curl_error_message{reader.get_curl_error_msg()};
    if (curl_error_message.has_value()) {
        message_to_log += "\nError message:\n" + std::string{curl_error_message.value()};
    }
    WARN(message_to_log);
    return false;
}
}  // namespace

TEST_CASE("network_reader_basic", "[NetworkReader]") {
    clp::FileReader ref_reader{get_test_input_local_path()};
    auto const expected{get_content(ref_reader)};

    clp::CurlGlobalInstance const curl_global_instance;
    clp::NetworkReader reader{get_test_input_remote_url()};
    auto const actual{get_content(reader)};
    REQUIRE(assert_curl_error_code(CURLE_OK, reader));
    REQUIRE((actual == expected));
}

TEST_CASE("network_reader_with_offset_and_seek", "[NetworkReader]") {
    constexpr size_t cOffset{319};
    clp::FileReader ref_reader{get_test_input_local_path()};
    ref_reader.seek_from_begin(cOffset);
    auto const expected{get_content(ref_reader)};
    auto const ref_end_pos{ref_reader.get_pos()};

    // Read from an offset onwards by starting the download from that offset.
    {
        clp::CurlGlobalInstance const curl_global_instance;
        clp::NetworkReader reader{get_test_input_remote_url(), cOffset};
        auto const actual{get_content(reader)};
        REQUIRE(assert_curl_error_code(CURLE_OK, reader));
        REQUIRE((reader.get_pos() == ref_end_pos));
        REQUIRE((actual == expected));
    }

    // Read from an offset onwards by seeking to that offset.
    {
        clp::CurlGlobalInstance const curl_global_instance;
        clp::NetworkReader reader(get_test_input_remote_url());
        reader.seek_from_begin(cOffset);
        auto const actual{get_content(reader)};
        REQUIRE(assert_curl_error_code(CURLE_OK, reader));
        REQUIRE((reader.get_pos() == ref_end_pos));
        REQUIRE((actual == expected));
    }
}

TEST_CASE("network_reader_destruct", "[NetworkReader]") {
    // We sleep to fill out all the buffers, and then we delete the reader. The destructor will try
    // to abort the underlying download and then destroy the instance. So should ensure destructor
    // is successfully executed without deadlock or exceptions.
    bool no_exception{true};
    try {
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        clp::CurlGlobalInstance const curl_global_instance;
        auto reader{std::make_unique<clp::NetworkReader>(
                get_test_input_remote_url(),
                0,
                true,
                clp::CurlDownloadHandler::cDefaultOverallTimeout,
                clp::CurlDownloadHandler::cDefaultConnectionTimeout,
                3,
                512
        )};
        std::this_thread::sleep_for(std::chrono::milliseconds{1500});
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        REQUIRE(reader->is_download_in_progress());
        reader.reset(nullptr);
    } catch (clp::NetworkReader::OperationFailed const& ex) {
        no_exception = false;
    }
    REQUIRE(no_exception);
}

TEST_CASE("network_reader_illegal_offset", "[NetworkReader]") {
    // Try to read from an out-of-bound offset.
    constexpr size_t cIllegalOffset{UINT32_MAX};
    clp::CurlGlobalInstance const curl_global_instance;
    clp::NetworkReader reader{get_test_input_remote_url(), cIllegalOffset};
    while (false == reader.get_curl_ret_code().has_value()) {
        // Wait until the return code is ready
    }

    if constexpr (clp::Platform::MacOs == clp::cCurrentPlatform) {
        // On macOS, HTTP response code 416 is not handled as `CURL_HTTP_RETURNED_ERROR` in some
        // `libcurl` versions.
        REQUIRE(
                (assert_curl_error_code(CURLE_HTTP_RETURNED_ERROR, reader)
                 || assert_curl_error_code(CURLE_RECV_ERROR, reader))
        );
    } else {
        REQUIRE(assert_curl_error_code(CURLE_HTTP_RETURNED_ERROR, reader));
    }
    size_t pos{};
    REQUIRE((clp::ErrorCode_Failure == reader.try_get_pos(pos)));
}
