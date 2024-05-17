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
extern "C" auto curl_download_progress_callback(
        void* reader_ptr,
        [[maybe_unused]] curl_off_t dltotal,
        [[maybe_unused]] curl_off_t dlnow,
        [[maybe_unused]] curl_off_t ultotal,
        [[maybe_unused]] curl_off_t ulnow
) -> int {
    return static_cast<StreamingReader*>(reader_ptr)->is_transfer_aborted() ? 1 : 0;
}

/**
 * libcurl write callback that writes downloaded data into the fetching buffer.
 * NOTE: This function must have C linkage to be a libcurl callback.
 * @param ptr The downloaded data
 * @param size Always 1
 * @param nmemb The number of bytes downloaded
 * @param reader_ptr A pointer to a `StreamingReader`.
 * @return On success, the number of bytes processed. If this is less than `nmemb`, the download
 * will be aborted.
 */
extern "C" auto
curl_download_write_callback(char* ptr, size_t size, size_t nmemb, void* reader_ptr) -> size_t {
    return static_cast<StreamingReader*>(reader_ptr)->write_to_fetching_buffer({ptr, size * nmemb});
}
}  // namespace

bool StreamingReader::m_initialized{false};

auto StreamingReader::TransferThread::thread_method() -> void {
    try {
        CurlDownloadHandler curl_handler{
                m_reader.m_src_url,
                static_cast<void*>(&m_reader),
                curl_download_progress_callback,
                curl_download_write_callback,
                static_cast<long>(m_reader.m_connection_timeout),
                static_cast<long>(m_reader.m_overall_timeout),
                m_offset,
                m_disable_caching
        };
        m_reader.m_curl_return_code = curl_handler.perform();
        m_reader.commit_fetching_buffer();
        m_reader.set_state_code(
                (CURLE_OK == m_reader.m_curl_return_code) ? State::Finished : State::Failed
        );
    } catch (CurlOperationFailed const& ex) {
        m_reader.m_curl_return_code = ex.get_curl_err();
        m_reader.set_state_code(State::Failed);
    }

    m_reader.m_cv_reader.notify_all();
}

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
    if (is_download_in_progress()) {
        // We need to kill the current connected session to reclaim CURL resources. Since we use
        // async thread for downloading, we need to abort the session to stop data transfer.
        abort_data_transfer();
    }

    while (is_transfer_thread_running()) {
        // TODO: we could use sleep instead of pulling
    }
}

auto StreamingReader::abort_data_transfer() -> void {
    m_transfer_aborted.store(true);
    m_cv_fetcher.notify_all();
}

auto StreamingReader::set_fetching_buffer() -> bool {
    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    if (m_fetching_buffer.has_value()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    while (m_num_fetched_buffer >= m_buffer_pool_size) {
        if (m_num_fetched_buffer != m_buffer_pool_size) {
            throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
        }
        m_cv_fetcher.wait(buffer_resource_lock);
        if (is_transfer_aborted()) {
            return false;
        }
        if (is_transfer_timedout()) {
            return false;
        }
    }
    m_fetching_buffer.emplace(m_buffer_pool.at(m_curr_fetching_buffer_idx).get(), m_buffer_size);
    return true;
}

auto StreamingReader::commit_fetching_buffer() -> void {
    if (false == m_fetching_buffer.has_value()) {
        return;
    }
    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_fetched_buffer_queue.emplace(
            m_buffer_pool.at(m_curr_fetching_buffer_idx).get(),
            m_buffer_size - m_fetching_buffer.value().size()
    );
    ++m_num_fetched_buffer;

    m_fetching_buffer.reset();
    ++m_curr_fetching_buffer_idx;
    if (m_curr_fetching_buffer_idx == m_buffer_pool_size) {
        m_curr_fetching_buffer_idx = 0;
    }

    m_cv_reader.notify_all();
}

auto StreamingReader::set_reading_buffer() -> bool {
    std::unique_lock<std::mutex> buffer_resource_lock{m_buffer_resource_mutex};
    if (m_reading_buffer.has_value()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    while (m_fetched_buffer_queue.empty()) {
        if (false == is_download_in_progress()) {
            return false;
        }
        m_cv_reader.wait(buffer_resource_lock);
    }
    auto const next_reading_buffer{m_fetched_buffer_queue.front()};
    m_fetched_buffer_queue.pop();
    m_reading_buffer.emplace(next_reading_buffer);
    return true;
}

auto StreamingReader::free_reading_buffer() -> void {
    std::unique_lock<std::mutex> const buffer_resource_lock{m_buffer_resource_mutex};
    m_reading_buffer.reset();
    --m_num_fetched_buffer;
    m_cv_fetcher.notify_all();
}

auto StreamingReader::write_to_fetching_buffer(StreamingReader::BufferView data_to_write
) -> size_t {
    auto const num_bytes_to_write{data_to_write.size()};
    try {
        while (false == data_to_write.empty()) {
            if (false == m_fetching_buffer.has_value() && false == set_fetching_buffer()) {
                return 0;
            }
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            auto& fetching_buffer{m_fetching_buffer.value()};
            auto const num_bytes_to_fetch{std::min(fetching_buffer.size(), data_to_write.size())};
            auto const copy_it_begin{data_to_write.begin()};
            auto copy_it_end{data_to_write.begin()};
            std::advance(copy_it_end, num_bytes_to_fetch);
            std::copy(copy_it_begin, copy_it_end, fetching_buffer.begin());
            data_to_write = data_to_write.subspan(num_bytes_to_fetch);
            fetching_buffer = fetching_buffer.subspan(num_bytes_to_fetch);
            if (fetching_buffer.empty()) {
                commit_fetching_buffer();
            }
        }
    } catch (TraceableException const& ex) {
        // TODO: add logging to track the exceptions.
        return 0;
    }
    return num_bytes_to_write;
}

auto StreamingReader::read_from_fetched_buffers(
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
        if (false == m_reading_buffer.has_value() && false == set_reading_buffer()) {
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
            free_reading_buffer();
        }
    }
    return ErrorCode_Success;
}
}  // namespace clp
