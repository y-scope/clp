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

#include "../src/clp/CurlDownloadHandler.hpp"
#include "../src/clp/ErrorCode.hpp"
#include "../src/clp/FileReader.hpp"
#include "../src/clp/NetworkReader.hpp"
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
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    auto const read_buf{std::make_unique<char[]>(read_buf_size)};
    for (bool has_more_content{true}; has_more_content;) {
        size_t num_bytes_read{};
        has_more_content = reader.read(read_buf.get(), read_buf_size, num_bytes_read);
        std::string_view const view{read_buf.get(), num_bytes_read};
        buf.insert(buf.cend(), view.cbegin(), view.cend());
    }
    return buf;
}
}  // namespace

TEST_CASE("network_reader_basic", "[NetworkReader]") {
    clp::FileReader ref_reader{get_test_input_local_path()};
    auto const expected{get_content(ref_reader)};

    REQUIRE((clp::ErrorCode_Success == clp::NetworkReader::init()));
    clp::NetworkReader reader{get_test_input_remote_url()};
    auto const actual{get_content(reader)};
    auto const ret_code{reader.get_curl_ret_code()};
    REQUIRE(ret_code.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    REQUIRE((CURLE_OK == ret_code.value()));
    REQUIRE((actual == expected));
    clp::NetworkReader::deinit();
}

TEST_CASE("network_reader_with_offset_and_seek", "[NetworkReader]") {
    constexpr size_t cOffset{319};
    clp::FileReader ref_reader{get_test_input_local_path()};
    ref_reader.seek_from_begin(cOffset);
    auto const expected{get_content(ref_reader)};
    auto const ref_end_pos{ref_reader.get_pos()};

    REQUIRE((clp::ErrorCode_Success == clp::NetworkReader::init()));

    // Read from an offset onwards by starting the download from that offset.
    {
        clp::NetworkReader reader{get_test_input_remote_url(), cOffset};
        auto const actual{get_content(reader)};
        auto const ret_code{reader.get_curl_ret_code()};
        REQUIRE(ret_code.has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        REQUIRE((CURLE_OK == ret_code.value()));
        REQUIRE((reader.get_pos() == ref_end_pos));
        REQUIRE((actual == expected));
    }

    // Read from an offset onwards by seeking to that offset.
    {
        clp::NetworkReader reader(get_test_input_remote_url());
        reader.seek_from_begin(cOffset);
        auto const actual{get_content(reader)};
        auto const ret_code{reader.get_curl_ret_code()};
        REQUIRE(ret_code.has_value());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        REQUIRE((CURLE_OK == ret_code.value()));
        REQUIRE((reader.get_pos() == ref_end_pos));
        REQUIRE((actual == expected));
    }

    clp::NetworkReader::deinit();
}

TEST_CASE("network_reader_destruct", "[NetworkReader]") {
    REQUIRE((clp::ErrorCode_Success == clp::NetworkReader::init()));

    // We sleep to fill out all the buffers, and then we delete the reader. The destructor will try
    // to abort the underlying download and then destroy the instance. So should ensure destructor
    // is successfully executed without deadlock or exceptions.
    bool no_exception{true};
    try {
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
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

    clp::NetworkReader::deinit();
}

TEST_CASE("network_reader_illegal_offset", "[NetworkReader]") {
    REQUIRE((clp::ErrorCode_Success == clp::NetworkReader::init()));

    // Try to read from an out-of-bound offset.
    constexpr size_t cIllegalOffset{UINT32_MAX};
    clp::NetworkReader reader{get_test_input_remote_url(), cIllegalOffset};
    while (true) {
        auto const ret_code{reader.get_curl_ret_code()};
        if (ret_code.has_value()) {
            REQUIRE((CURLE_HTTP_RETURNED_ERROR == ret_code.value()));
            size_t pos{};
            REQUIRE((clp::ErrorCode_Failure == reader.try_get_pos(pos)));
            break;
        }
    }

    clp::NetworkReader::deinit();
}
