#include "NetworkReader.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <curl/curl.h>

#include "CurlDownloadHandler.hpp"
#include "CurlOperationFailed.hpp"
#include "ErrorCode.hpp"
#include "Platform.hpp"

namespace clp {
/**
 * To understand the implementation, we first define several terms:
 *
 * - reader_thread: The thread that creates and uses the public API of a NetworkReader instance
 *   (can be the main thread).
 * - downloader_thread: The thread that invokes libcurl to download and buffer data.
 * - buffer_pool: Empty buffers that the downloader thread uses to buffer the data. The pool itself
 *   is a ring buffer.
 * - filled_buffer_queue: A queue of filled (fully or partially) buffers.
 * - curr_downloader_buf: The buffer currently used by `downloader_thread` to buffer downloaded
 *   data.
 * - curr_reader_buf: The buffer currently used by `reader_thread` to read data out of.
 *
 * The diagram below illustrates the relationship between these terms:
 *
 * | +-------------------+ |                                   | +-------------------+ |
 * |     reader_thread     |                                   |   downloader_thread   |
 * | +-------------------+ |                                   | +-------------------+ |
 * |                       |      | +-----------------+ |      |                       |
 * | release_empty_buffer   >>>>>>      buffer_pool      >>>>>>  acquire_empty_buffer  |
 * |                       |      | +-----------------+ |      |                       |
 * |           +           |                                   |           +           |
 * |          +++          |                                   |           +           |
 * |         +++++         |                                   |           +           |
 * |           +           |                                   |           +           |
 * |           +           |                                   |         +++++         |
 * |           +           |                                   |          +++          |
 * |           +           |                                   |           +           |
 * |                       |                                   |                       |
 * |    curr_reader_buf    |                                   |  curr_downloader_buf  |
 * |                       |                                   |                       |
 * |           +           |                                   |           +           |
 * |          +++          |                                   |           +           |
 * |         +++++         |                                   |           +           |
 * |           +           |                                   |           +           |
 * |           +           |                                   |         +++++         |
 * |           +           |                                   |          +++          |
 * |           +           |                                   |           +           |
 * |                       |      | +-----------------+ |      |                       |
 * |   get_filled_buffer    <<<<<<  filled_buffer_queue  <<<<<<  enqueue_filled_buffer |
 * |                       |      | +-----------------+ |      |                       |
 * | +-------------------+ |                                   | +-------------------+ |
 *
 * `downloader_thread` operates as follows:
 * - It acquires an empty buffer from the buffer pool using `acquire_empty_buffer` and saves it as
 *   `curr_downloader_buf`.
 * - It writes downloaded data into `curr_downloader_buf`.
 * - When `curr_downloader_buf` is filled or the download is complete, it will be enqueued in
 *   `filled_buffer_queue` using `enqueue_filled_buffer`.
 *
 * `reader_thread` operates as follows:
 * - It dequeues a buffer from `filled_buffer_queue` using `get_filled_buffer` and saves it as
 *   `curr_reader_buf`.
 * - It performs any reads using data in `curr_reader_buf`.
 * - When `curr_reader_buf` is exhausted, it is returned to `buffer_pool` using
 *   `release_empty_buffer`.
 */

namespace {
/**
 * libcurl progress callback used to cause libcurl to abort the download if requested by the caller.
 * NOTE: This function must have C linkage to be a libcurl callback.
 * @param reader_ptr A pointer to a `NetworkReader`.
 * @param dltotal Unused
 * @param dlnow Unused
 * @param ultotal Unused
 * @param ulnow Unused
 * @return 1 if the download should be aborted, 0 otherwise.
 */
extern "C" auto curl_progress_callback(
        void* reader_ptr,
        [[maybe_unused]] curl_off_t dltotal,
        [[maybe_unused]] curl_off_t dlnow,
        [[maybe_unused]] curl_off_t ultotal,
        [[maybe_unused]] curl_off_t ulnow
) -> int {
    return static_cast<NetworkReader*>(reader_ptr)->is_abort_download_requested() ? 1 : 0;
}

/**
 * libcurl write callback that writes downloaded data into the buffers.
 * NOTE: This function must have C linkage to be a libcurl callback.
 * @param ptr A pointer to the downloaded data
 * @param size Always 1.
 * @param nmemb The number of bytes downloaded.
 * @param reader_ptr A pointer to a `NetworkReader`.
 * @return On success, the number of bytes processed. If this is less than `nmemb`, the download
 * will be aborted.
 */
extern "C" auto curl_write_callback(char* ptr, size_t size, size_t nmemb, void* reader_ptr)
        -> size_t {
    return static_cast<NetworkReader*>(reader_ptr)->buffer_downloaded_data({ptr, size * nmemb});
}
}  // namespace

NetworkReader::NetworkReader(
        std::string_view src_url,
        size_t offset,
        bool disable_caching,
        std::chrono::seconds overall_timeout,
        std::chrono::seconds connection_timeout,
        size_t buffer_pool_size,
        size_t buffer_size,
        std::optional<std::unordered_map<std::string, std::string>> http_header_kv_pairs
)
        : m_src_url{src_url},
          m_offset{offset},
          m_file_pos{offset},
          m_overall_timeout{overall_timeout},
          m_connection_timeout{connection_timeout},
          m_buffer_pool_size{std::max(cMinBufferPoolSize, buffer_pool_size)},
          m_buffer_size{std::max(cMinBufferSize, buffer_size)} {
    for (size_t i = 0; i < m_buffer_pool_size; ++i) {
        m_buffer_pool.emplace_back(m_buffer_size);
    }
    m_downloader_thread = std::make_unique<DownloaderThread>(
            *this,
            offset,
            disable_caching,
            std::move(http_header_kv_pairs)
    );
    m_downloader_thread->start();
}

NetworkReader::~NetworkReader() {
    if (is_download_in_progress()) {
        // Abort the download so the downloader thread can destroy the CURL resources and exit.
        submit_abort_download_request();
    }

    while (is_downloader_thread_running()) {
        // We could use sleep instead of busy waiting
    }
}

auto NetworkReader::try_get_pos(size_t& pos) -> ErrorCode {
    if (0 != m_offset) {
        if (false == is_download_in_progress()) {
            auto const curl_return_code{get_curl_ret_code()};
            if (false == curl_return_code.has_value()) {
                // This shouldn't be possible
                throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
            }
            if (CURLE_HTTP_RETURNED_ERROR == curl_return_code.value()) {
                // Download has failed due to an HTTP error. This can be caused by an out-of-bound
                // offset specified in the HTTP header.
                return ErrorCode_Failure;
            }
            if constexpr (Platform::MacOs == cCurrentPlatform) {
                // On macOS, HTTP response code 416 is not handled as `CURL_HTTP_RETURNED_ERROR` in
                // some `libcurl` versions.
                if (CURLE_RECV_ERROR == curl_return_code.value()) {
                    return ErrorCode_Failure;
                }
            }
        }

        if (false == at_least_one_byte_downloaded()) {
            // We don't know if there are any bytes coming in yet
            return ErrorCode_NotReady;
        }
    }

    pos = m_file_pos;
    return ErrorCode_Success;
}

auto NetworkReader::buffer_downloaded_data(NetworkReader::BufferView data) -> size_t {
    if (data.empty()) {
        return 0;
    }
    if (false == at_least_one_byte_downloaded()) {
        m_at_least_one_byte_downloaded.store(true);
    }
    auto const num_bytes_to_write{data.size()};
    while (false == data.empty()) {
        acquire_empty_buffer();
        if (false == m_curr_downloader_buf.has_value()) {
            return 0;
        }
        auto& curr_downloader_buf{m_curr_downloader_buf.value()};

        // Copy enough to fill the downloader's buf or to exhaust the data
        auto const data_view{data.subspan(0, std::min(curr_downloader_buf.size(), data.size()))};
        std::copy(data_view.begin(), data_view.end(), curr_downloader_buf.begin());
        data = data.subspan(data_view.size());
        curr_downloader_buf = curr_downloader_buf.subspan(data_view.size());
        if (curr_downloader_buf.empty()) {
            enqueue_filled_buffer();
        }
    }
    return num_bytes_to_write;
}

auto NetworkReader::DownloaderThread::thread_method() -> void {
    try {
        CurlDownloadHandler curl_handler{
                m_reader.m_curl_error_msg_buf,
                curl_progress_callback,
                curl_write_callback,
                static_cast<void*>(&m_reader),
                m_reader.m_src_url,
                m_offset,
                m_disable_caching,
                m_reader.m_connection_timeout,
                m_reader.m_overall_timeout,
                m_http_header_kv_pairs
        };
        auto const ret_code{curl_handler.perform()};
        // Enqueue the last filled buffer, if any
        m_reader.enqueue_filled_buffer();
        m_reader.set_download_completion_status(ret_code);
    } catch (CurlOperationFailed const& ex) {
        m_reader.set_download_completion_status(ex.get_curl_err());
    }

    std::unique_lock<std::mutex> const buffer_resource_lock{m_reader.m_buffer_resource_mutex};
    m_reader.m_reader_cv.notify_all();
}

auto NetworkReader::submit_abort_download_request() -> void {
    m_abort_download_requested.store(true);

    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_downloader_cv.notify_all();
}

auto NetworkReader::acquire_empty_buffer() -> void {
    if (m_curr_downloader_buf.has_value()) {
        return;
    }
    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    while (m_filled_buffer_queue.size() == m_buffer_pool_size) {
        if (is_abort_download_requested()) {
            return;
        }
        m_downloader_cv.wait(buffer_resource_lock);
    }
    m_curr_downloader_buf.emplace(
            m_buffer_pool.at(m_curr_downloader_buf_idx).data(),
            m_buffer_size
    );
}

auto NetworkReader::release_empty_buffer() -> void {
    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_curr_reader_buf.reset();
    m_filled_buffer_queue.pop();
    m_downloader_cv.notify_all();
}

auto NetworkReader::enqueue_filled_buffer() -> void {
    if (false == m_curr_downloader_buf.has_value()) {
        return;
    }
    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_filled_buffer_queue.emplace(
            m_buffer_pool.at(m_curr_downloader_buf_idx).data(),
            m_buffer_size - m_curr_downloader_buf.value().size()
    );

    m_curr_downloader_buf.reset();
    ++m_curr_downloader_buf_idx;
    if (m_curr_downloader_buf_idx == m_buffer_pool_size) {
        m_curr_downloader_buf_idx = 0;
    }

    m_reader_cv.notify_all();
}

auto NetworkReader::get_filled_buffer() -> void {
    if (m_curr_reader_buf.has_value()) {
        return;
    }
    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    while (m_filled_buffer_queue.empty()) {
        if (false == is_download_in_progress()) {
            return;
        }
        m_reader_cv.wait(buffer_resource_lock);
    }
    auto const next_reader_buffer{m_filled_buffer_queue.front()};
    m_curr_reader_buf.emplace(next_reader_buffer);
}

auto
NetworkReader::read_from_filled_buffers(size_t num_bytes_to_read, size_t& num_bytes_read, char* dst)
        -> ErrorCode {
    num_bytes_read = 0;
    std::optional<BufferView> dst_view;
    if (nullptr != dst) {
        dst_view = BufferView{dst, num_bytes_to_read};
    }
    while (num_bytes_to_read > 0) {
        get_filled_buffer();
        if (false == m_curr_reader_buf.has_value()) {
            return num_bytes_read > 0 ? ErrorCode_Success : ErrorCode_EndOfFile;
        }
        auto& curr_reader_buf{m_curr_reader_buf.value()};

        auto const reader_buf_size{curr_reader_buf.size()};
        auto const src{curr_reader_buf.subspan(0, std::min(num_bytes_to_read, reader_buf_size))};
        if (dst_view.has_value()) {
            auto& dst_buf{dst_view.value()};
            std::copy(src.begin(), src.end(), dst_buf.begin());
            dst_buf = dst_buf.subspan(src.size());
        }
        num_bytes_to_read -= src.size();
        num_bytes_read += src.size();
        m_file_pos += src.size();
        curr_reader_buf = curr_reader_buf.subspan(src.size());
        if (curr_reader_buf.empty()) {
            release_empty_buffer();
        }
    }
    return ErrorCode_Success;
}
}  // namespace clp
