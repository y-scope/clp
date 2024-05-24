#include "StreamingReader.hpp"

#include <chrono>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <string>

#include <curl/curl.h>

#include "CurlDownloadHandler.hpp"
#include "CurlOperationFailed.hpp"
#include "CurlStringList.hpp"
#include "ErrorCode.hpp"
#include "Thread.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * To understand the implementation, we first define several terms:
 *
 * - reader_thread: The thread that creates and uses the public API of a StreamingReader instance
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
 *   .....................................           .........................................
 *   |                            +-------------------------+                                |
 *   |   `get_filled_buffer` <=== <   filled_buffer_queue   < <=== `enqueue_filled_buffer`   |
 *   |                            +-------------------------+                                |
 *   |              +                    |           |                    +                  |
 *   |              +                    |           |                   +++                 |
 *   |              +                    |           |                  +++++                |
 *   |              +                    |           |                    +                  |
 *   |            +++++                  |           |                    +                  |
 *   |             +++                   |           |                    +                  |
 *   |              +                    |           |                    +                  |
 *   |                                   |           |                                       |
 *   |      `curr_reader_buf`            |           |          `curr_downloader_buf`        |
 *   |                                   |           |                                       |
 *   |              +                    |           |                    +                  |
 *   |              +                    |           |                   +++                 |
 *   |              +                    |           |                  +++++                |
 *   |              +                    |           |                    +                  |
 *   |            +++++                  |           |                    +                  |
 *   |             +++                   |           |                    +                  |
 *   |              +                    |           |                    +                  |
 *   |                                +------------------+                                   |
 *   |    `release_empty_buffer` ===> >    buffer_pool   > ====> `acquire_empty_buffer`      |
 *   |                                +------------------+                                   |
 *   |       `reader_thread`             |           |            `downloader_thread`        |
 *   .....................................           .........................................
 *
 * `downloader_thread` operates as follows:
 * - It acquires an empty buffer from the buffer pool using `acquire_empty_buffer` and saves it as
 *   `curr_downloader_buf`.
 * - It writes downloaded data into `curr_downloader_buf`.
 * - When `curr_downloader_buf` is filled or the download is complete, it will be enqueued in
 *   `filled_buffer_queue` using `enqueue_filled_buffer`.
 *
 * `reader_thread` operates as follows:
 * - It dequeues a buffer from `filled_buffer_queue` using `dequeue_filled_buffer` and saves it as
 *   `curr_reader_buf`.
 * - It performs any reads using data in `curr_reader_buf`.
 * - When `curr_reader_buf` is exhausted, it is returned to `buffer_pool` using
 *   `release_empty_buffer`.
 */

namespace {
/**
 * libcurl progress callback used to cause libcurl to abort the download if requested by the caller.
 * NOTE: This function must have C linkage to be a libcurl callback.
 * @param reader_ptr A pointer to a `StreamingReader`.
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
    return static_cast<StreamingReader*>(reader_ptr)->is_download_aborted() ? 1 : 0;
}

/**
 * libcurl write callback that writes downloaded data into the buffers.
 * NOTE: This function must have C linkage to be a libcurl callback.
 * @param ptr A pointer to the downloaded data
 * @param size Always 1.
 * @param nmemb The number of bytes downloaded.
 * @param reader_ptr A pointer to a `StreamingReader`.
 * @return On success, the number of bytes processed. If this is less than `nmemb`, the download
 * will be aborted.
 */
extern "C" auto
curl_write_callback(char* ptr, size_t size, size_t nmemb, void* reader_ptr) -> size_t {
    return static_cast<StreamingReader*>(reader_ptr)->download_data({ptr, size * nmemb});
}
}  // namespace

bool StreamingReader::m_initialized{false};

auto StreamingReader::init() -> ErrorCode {
    if (m_initialized) {
        return ErrorCode_Success;
    }
    if (0 != curl_global_init(CURL_GLOBAL_ALL)) {
        return ErrorCode_Failure;
    }
    m_initialized = true;
    return ErrorCode_Success;
}

auto StreamingReader::deinit() -> void {
    curl_global_cleanup();
    m_initialized = false;
}

StreamingReader::StreamingReader(
        std::string_view src_url,
        size_t offset,
        bool disable_caching,
        uint32_t overall_timeout_int_sec,
        uint32_t connection_timeout_in_sec,
        size_t buffer_pool_size,
        size_t buffer_size
)
        : m_src_url{src_url},
          m_overall_timeout{overall_timeout_int_sec},
          m_connection_timeout{connection_timeout_in_sec},
          m_buffer_pool_size{std::max(cMinBufferPoolSize, buffer_pool_size)},
          m_buffer_size{std::max(cMinBufferSize, buffer_size)} {
    for (size_t i = 0; i < m_buffer_pool_size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
        m_buffer_pool.emplace_back(std::make_unique<char[]>(m_buffer_size));
    }
    if (false == m_initialized) {
        throw OperationFailed(ErrorCode_NotReady, __FILE__, __LINE__);
    }
    m_downloader_thread = std::make_unique<DownloaderThread>(*this, offset, disable_caching);
    m_downloader_thread->start();
}

StreamingReader::~StreamingReader() {
    if (is_download_in_progress()) {
        // Abort the download so the downloader thread can destroy the CURL resources and exit.
        abort_data_download();
    }

    while (is_downloader_thread_running()) {
        // We could use sleep instead of busy waiting
    }
}

auto StreamingReader::download_data(StreamingReader::BufferView data_to_write) -> size_t {
    auto const num_bytes_to_write{data_to_write.size()};
    try {
        while (false == data_to_write.empty()) {
            if (false == m_curr_downloader_buf.has_value() && false == acquire_empty_buffer()) {
                return 0;
            }
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            auto& buffer_to_fill{m_curr_downloader_buf.value()};
            auto const num_bytes_to_fill{std::min(buffer_to_fill.size(), data_to_write.size())};
            auto const copy_it_begin{data_to_write.begin()};
            auto copy_it_end{data_to_write.begin()};
            std::advance(copy_it_end, num_bytes_to_fill);
            std::copy(copy_it_begin, copy_it_end, buffer_to_fill.begin());
            data_to_write = data_to_write.subspan(num_bytes_to_fill);
            buffer_to_fill = buffer_to_fill.subspan(num_bytes_to_fill);
            if (buffer_to_fill.empty()) {
                enqueue_filled_buffer();
            }
        }
    } catch (TraceableException const& ex) {
        // TODO: add logging to track the exceptions.
        return 0;
    }
    return num_bytes_to_write;
}

auto StreamingReader::DownloaderThread::thread_method() -> void {
    try {
        CurlDownloadHandler curl_handler{
                m_reader.m_src_url,
                static_cast<void*>(&m_reader),
                curl_progress_callback,
                curl_write_callback,
                static_cast<long>(m_reader.m_connection_timeout),
                static_cast<long>(m_reader.m_overall_timeout),
                m_offset,
                m_disable_caching
        };
        m_reader.m_curl_return_code = curl_handler.perform();
        // Enqueue the last filled buffer, if any
        m_reader.enqueue_filled_buffer();
        m_reader.set_state_code(
                (CURLE_OK == m_reader.m_curl_return_code) ? State::Finished : State::Failed
        );
    } catch (CurlOperationFailed const& ex) {
        m_reader.m_curl_return_code = ex.get_curl_err();
        m_reader.set_state_code(State::Failed);
    }

    std::unique_lock<std::mutex> buffer_resource_lock{m_reader.m_buffer_resource_mutex};
    m_reader.m_cv_reader.notify_all();
}

auto StreamingReader::abort_data_download() -> void {
    m_download_aborted.store(true);

    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    m_cv_downloader.notify_all();
}

auto StreamingReader::acquire_empty_buffer() -> bool {
    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    if (m_curr_downloader_buf.has_value()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    while (m_filled_buffer_queue.size() == m_buffer_pool_size) {
        m_cv_downloader.wait(buffer_resource_lock);
        if (is_download_aborted()) {
            return false;
        }
        if (is_download_timedout()) {
            return false;
        }
    }
    m_curr_downloader_buf.emplace(m_buffer_pool.at(m_curr_downloader_buf_idx).get(), m_buffer_size);
    return true;
}

auto StreamingReader::enqueue_filled_buffer() -> void {
    if (false == m_curr_downloader_buf.has_value()) {
        return;
    }
    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_filled_buffer_queue.emplace(
            m_buffer_pool.at(m_curr_downloader_buf_idx).get(),
            m_buffer_size - m_curr_downloader_buf.value().size()
    );

    m_curr_downloader_buf.reset();
    ++m_curr_downloader_buf_idx;
    if (m_curr_downloader_buf_idx == m_buffer_pool_size) {
        m_curr_downloader_buf_idx = 0;
    }

    m_cv_reader.notify_all();
}

auto StreamingReader::get_filled_buffer() -> bool {
    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    if (m_curr_reader_buf.has_value()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    while (m_filled_buffer_queue.empty()) {
        if (false == is_download_in_progress()) {
            return false;
        }
        m_cv_reader.wait(buffer_resource_lock);
    }
    auto const next_reader_buffer{m_filled_buffer_queue.front()};
    m_curr_reader_buf.emplace(next_reader_buffer);
    return true;
}

auto StreamingReader::release_empty_buffer() -> void {
    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_curr_reader_buf.reset();
    m_filled_buffer_queue.pop();
    m_cv_downloader.notify_all();
}

auto StreamingReader::read_from_filled_buffers(
        size_t num_bytes_to_read,
        size_t& num_bytes_read,
        char* dst
) -> ErrorCode {
    num_bytes_read = 0;
    std::optional<BufferView> dst_view{std::nullopt};
    if (nullptr != dst) {
        dst_view = BufferView{dst, num_bytes_to_read};
    }
    while (0 != num_bytes_to_read) {
        if (false == m_curr_reader_buf.has_value() && false == get_filled_buffer()) {
            return ErrorCode_EndOfFile;
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto& curr_reader_buf{m_curr_reader_buf.value()};
        auto const reader_buf_size{curr_reader_buf.size()};
        if (0 == reader_buf_size) {
            return ErrorCode_EndOfFile;
        }
        auto const num_bytes_to_consume_from_buffer{
                num_bytes_to_read > reader_buf_size ? reader_buf_size : num_bytes_to_read
        };
        if (dst_view.has_value()) {
            memcpy(dst_view.value().last(num_bytes_to_read).data(),
                   curr_reader_buf.data(),
                   num_bytes_to_consume_from_buffer);
        }
        num_bytes_to_read -= num_bytes_to_consume_from_buffer;
        num_bytes_read += num_bytes_to_consume_from_buffer;
        m_file_pos += num_bytes_to_consume_from_buffer;
        curr_reader_buf = curr_reader_buf.subspan(num_bytes_to_consume_from_buffer);
        if (curr_reader_buf.empty()) {
            release_empty_buffer();
        }
    }
    return ErrorCode_Success;
}
}  // namespace clp
