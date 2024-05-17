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
 * StreamingReader behaviour description:
 * reader_thread: The thread that creates a StreamingReader instance.
 * transfer_thread: The thread that transfers data from a source URL using `libcurl`.
 * buffer_pool: Empty buffers ready for filling transferred data. The pool itself is a ring buffer.
 * filled_buffer_queue: A queue of filled buffers.
 * transfer_buffer: A single buffer locked by `transfer_thread` to write downloaded data.
 * reading_buffer: A single buffer locked by `reader_thread` to read downloaded data.
 *
 *   .....................................           .........................................
 *   |                            +-------------------------+                                |
 *   |   `get_filled_buffer` <=== <   filled_buffer_queue   < <=== `enqueue_filled_buffer`   |
 *   |                            +-------------------------+                                |
 *   |              +                    |           |                    +                  |
 *   |              +                    |           |                    +                  |
 *   |              +                    |           |                    +                  |
 *   |              +                    |           |                    +                  |
 *   |            +++++                  |           |                  +++++                |
 *   |             +++                   |           |                   +++                 |
 *   |              +                    |           |                    +                  |
 *   |                                   |           |                                       |
 *   |      `reading_buffer`             |           |            `transfer_buffer`          |
 *   |                                   |           |                                       |
 *   |              +                    |           |                    +                  |
 *   |              +                    |           |                   +++                 |
 *   |              +                    |           |                  +++++                |
 *   |              +                    |           |                    +                  |
 *   |            +++++                  |           |                    +                  |
 *   |             +++                   |           |                    +                  |
 *   |              +                    |           |                    +                  |
 *   |                                +------------------+                                   |
 *   | `release_empty_buffer` ======> >    buffer_pool   > =======> `acquire_empty_buffer`   |
 *   |                                +------------------+                                   |
 *   |       `reader_thread`             |           |           `transfer_thread`           |
 *   .....................................           .........................................
 *
 * For `transfer_thread`:
 *      - It gets `transfer_buffer` from buffer pool using `acquire_empty_buffer`
 *      - It only writes to `transfer_buffer`
 *      - When `transfer_buffer` is fully filled, or no more data is coming, it will be enqueued to
 *        the filled buffer queue by `enqueue_filled_buffer`
 *
 * For `reader_thread`:
 *      - It gets `reading_buffer` from filled buffer queue using `get_filled_buffer`
 *      - It only reads from `reading_buffer`
 *      - When `reading_buffer` is fully read, it will be returned back to the buffer pool by
 *        `release_empty_buffer`
 */

namespace {
/**
 * libcurl progress callback used only to determine whether to abort the current transfer.
 * NOTE: This function must have C linkage to be a libcurl callback.
 * @param reader_ptr A pointer to a `StreamingReader`.
 * @param dltotal Unused
 * @param dlnow Unused
 * @param ultotal Unused
 * @param ulnow Unused
 * @return 1 if the transfer was aborted, 0 otherwise.
 */
extern "C" auto curl_progress_callback(
        void* reader_ptr,
        [[maybe_unused]] curl_off_t dltotal,
        [[maybe_unused]] curl_off_t dlnow,
        [[maybe_unused]] curl_off_t ultotal,
        [[maybe_unused]] curl_off_t ulnow
) -> int {
    return static_cast<StreamingReader*>(reader_ptr)->is_transfer_aborted() ? 1 : 0;
}

/**
 * libcurl write callback that writes transferred data into the buffers.
 * NOTE: This function must have C linkage to be a libcurl callback.
 * @param ptr A pointer to the transferred data
 * @param size Always 1
 * @param nmemb The number of bytes transferred
 * @param reader_ptr A pointer to a `StreamingReader`.
 * @return On success, the number of bytes processed. If this is less than `nmemb`, the transfer
 * will be aborted.
 */
extern "C" auto
curl_write_callback(char* ptr, size_t size, size_t nmemb, void* reader_ptr) -> size_t {
    return static_cast<StreamingReader*>(reader_ptr)->transfer_data({ptr, size * nmemb});
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

auto StreamingReader::TransferThread::thread_method() -> void {
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
        m_reader.enqueue_filled_buffer();
        m_reader.set_state_code(
                (CURLE_OK == m_reader.m_curl_return_code) ? State::Finished : State::Failed
        );
    } catch (CurlOperationFailed const& ex) {
        m_reader.m_curl_return_code = ex.get_curl_err();
        m_reader.set_state_code(State::Failed);
    }

    m_reader.m_cv_reader.notify_all();
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
        : m_overall_timeout{overall_timeout_int_sec},
          m_connection_timeout{connection_timeout_in_sec},
          m_buffer_pool_size{std::max(cMinBufferPoolSize, buffer_pool_size)},
          m_buffer_size{std::max(cMinBufferPoolSize, buffer_size)} {
    for (size_t i = 0; i < m_buffer_pool_size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
        m_buffer_pool.emplace_back(std::make_unique<char[]>(m_buffer_size));
    }
    if (false == m_initialized) {
        throw OperationFailed(ErrorCode_NotReady, __FILE__, __LINE__);
    }
    m_src_url = src_url;
    m_transfer_thread = std::make_unique<TransferThread>(std::ref(*this), offset, disable_caching);
    m_transfer_thread->start();
}

StreamingReader::~StreamingReader() {
    if (is_curl_transfer_in_progress()) {
        // We need to kill the current connected session to reclaim CURL resources. Since the
        // transfer thread is asynchronized, we need to abort the session to stop data transfer.
        abort_data_transfer();
    }

    while (is_transfer_thread_running()) {
        // TODO: we could use sleep instead of pulling
    }
}

auto StreamingReader::abort_data_transfer() -> void {
    m_transfer_aborted.store(true);
    m_cv_transfer.notify_all();
}

auto StreamingReader::acquire_empty_buffer() -> bool {
    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    if (m_transfer_buffer.has_value()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    while (m_num_filled_buffer >= m_buffer_pool_size) {
        if (m_num_filled_buffer != m_buffer_pool_size) {
            throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
        }
        m_cv_transfer.wait(buffer_resource_lock);
        if (is_transfer_aborted()) {
            return false;
        }
        if (is_transfer_timedout()) {
            return false;
        }
    }
    m_transfer_buffer.emplace(m_buffer_pool.at(m_curr_transfer_buffer_idx).get(), m_buffer_size);
    return true;
}

auto StreamingReader::enqueue_filled_buffer() -> void {
    if (false == m_transfer_buffer.has_value()) {
        return;
    }
    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_filled_buffer_queue.emplace(
            m_buffer_pool.at(m_curr_transfer_buffer_idx).get(),
            m_buffer_size - m_transfer_buffer.value().size()
    );
    ++m_num_filled_buffer;

    m_transfer_buffer.reset();
    ++m_curr_transfer_buffer_idx;
    if (m_curr_transfer_buffer_idx == m_buffer_pool_size) {
        m_curr_transfer_buffer_idx = 0;
    }

    m_cv_reader.notify_all();
}

auto StreamingReader::get_filled_buffer() -> bool {
    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    if (m_reading_buffer.has_value()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    while (m_filled_buffer_queue.empty()) {
        if (false == is_curl_transfer_in_progress()) {
            return false;
        }
        m_cv_reader.wait(buffer_resource_lock);
    }
    auto const next_reading_buffer{m_filled_buffer_queue.front()};
    m_filled_buffer_queue.pop();
    m_reading_buffer.emplace(next_reading_buffer);
    return true;
}

auto StreamingReader::release_empty_buffer() -> void {
    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_reading_buffer.reset();
    --m_num_filled_buffer;
    m_cv_transfer.notify_all();
}

auto StreamingReader::transfer_data(StreamingReader::BufferView data_to_write) -> size_t {
    auto const num_bytes_to_write{data_to_write.size()};
    try {
        while (false == data_to_write.empty()) {
            if (false == m_transfer_buffer.has_value() && false == acquire_empty_buffer()) {
                return 0;
            }
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            auto& buffer_to_fill{m_transfer_buffer.value()};
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
        if (false == m_reading_buffer.has_value() && false == get_filled_buffer()) {
            return ErrorCode_EndOfFile;
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto& reading_buffer{m_reading_buffer.value()};
        auto const reading_buffer_size{reading_buffer.size()};
        if (0 == reading_buffer_size) {
            return ErrorCode_EndOfFile;
        }
        auto const num_bytes_to_consume_from_buffer{
                num_bytes_to_read > reading_buffer_size ? reading_buffer_size : num_bytes_to_read
        };
        if (dst_view.has_value()) {
            memcpy(dst_view.value().last(num_bytes_to_read).data(),
                   reading_buffer.data(),
                   num_bytes_to_consume_from_buffer);
        }
        num_bytes_to_read -= num_bytes_to_consume_from_buffer;
        num_bytes_read += num_bytes_to_consume_from_buffer;
        m_file_pos += num_bytes_to_consume_from_buffer;
        reading_buffer = reading_buffer.subspan(num_bytes_to_consume_from_buffer);
        if (reading_buffer.empty()) {
            release_empty_buffer();
        }
    }
    return ErrorCode_Success;
}
}  // namespace clp
