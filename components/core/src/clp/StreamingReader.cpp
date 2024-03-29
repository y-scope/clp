#include "StreamingReader.hpp"

#include <chrono>
#include <cstring>

#include <curl/curl.h>

namespace clp {
namespace {
/**
 * Casts the given ptr to a StreamingReader object. This wrapper function is used to silence
 * clang-tidy warnings.
 * @param ptr The address of a StreamingReader object.
 * @return a StreamingReader type pointer.
 */
[[nodiscard]] auto get_reader(void* ptr) -> StreamingReader* {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<StreamingReader*>(ptr);
}
}  // namespace

bool StreamingReader::m_initialized{false};

auto StreamingReader::global_init() -> ErrorCode {
    if (m_initialized) {
        return ErrorCode_Success;
    }
    if (0 != curl_global_init(CURL_GLOBAL_ALL)) {
        return ErrorCode_Failure;
    }
    m_initialized = true;
    return ErrorCode_Success;
}

auto StreamingReader::global_cleanup() -> void {
    curl_global_cleanup();
    m_initialized = false;
}

auto StreamingReader::transfer_thread_entry(
        StreamingReader& reader,
        size_t offset,
        bool disable_caching
) -> void {
    CURL* curl_handle{curl_easy_init()};
    if (nullptr == curl_handle) {
        reader.set_status_code(StatusCode::Failed);
        reader.m_transfer_terminated.store(true);
        return;
    }

    // Setup download URL
    curl_easy_setopt(curl_handle, CURLOPT_URL, reader.m_src_url.c_str());

    // Setup progress callback
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, StreamingReader::progress_callback);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, static_cast<void*>(&reader));
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0);

    // Setup write callback
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, StreamingReader::write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, static_cast<void*>(&reader));

    // Setup timeout
    curl_easy_setopt(
            curl_handle,
            CURLOPT_CONNECTTIMEOUT,
            static_cast<long>(reader.m_connection_timeout)
    );
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, static_cast<long>(reader.m_operation_timeout));

    // Setup the header (if necessary)
    reader.m_file_pos = offset;
    struct curl_slist* request_header{nullptr};
    if (0 != offset) {
        std::string const range{"Range: bytes=" + std::to_string(offset) + "-"};
        request_header = curl_slist_append(request_header, range.c_str());
    }
    if (disable_caching) {
        request_header = curl_slist_append(request_header, "Cache-Control: no-cache");
        request_header = curl_slist_append(request_header, "Pragma: no-cache");
    }
    if (nullptr != request_header) {
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, request_header);
    }

    auto const retval{curl_easy_perform(curl_handle)};
    reader.commit_fetching_buffer();
    reader.set_status_code((0 != retval) ? StatusCode::Failed : StatusCode::Finished);
    reader.m_curl_return_code = retval;

    curl_easy_cleanup(curl_handle);
    reader.m_transfer_terminated.store(true);
}

auto StreamingReader::progress_callback(
        void* ptr,
        [[maybe_unused]] curl_off_t dltotal,
        [[maybe_unused]] curl_off_t dlnow,
        [[maybe_unused]] curl_off_t ultotal,
        [[maybe_unused]] curl_off_t ulnow
) -> int {
    auto const* reader{get_reader(ptr)};
    return reader->is_transfer_aborted() ? 1 : 0;
}

auto StreamingReader::write_callback(char* ptr, size_t size, size_t nmemb, void* reader_ptr)
        -> size_t {
    auto const num_total_bytes{size * nmemb};
    try {
        StreamingReader::BufferView input_buffer{ptr, num_total_bytes};
        auto* reader{get_reader(reader_ptr)};
        while (false == input_buffer.empty()) {
            auto const num_bytes_left{input_buffer.size()};
            StreamingReader::BufferView fetching_buffer;
            if (false == reader->get_buffer_to_fetch(fetching_buffer)) {
                return 0;
            }
            auto const num_bytes_to_fetch{
                    num_bytes_left > fetching_buffer.size() ? fetching_buffer.size()
                                                            : num_bytes_left
            };
            memcpy(fetching_buffer.data(), input_buffer.data(), num_bytes_to_fetch);
            input_buffer = input_buffer.last(num_bytes_left - num_bytes_to_fetch);
            reader->commit_fetching(num_bytes_to_fetch);
        }
    } catch (TraceableException const& ex) {
        // TODO: add logging to track the exceptions.
        return 0;
    }
    return num_total_bytes;
}

auto StreamingReader::open(std::string_view src_url, size_t offset, bool disable_caching) -> void {
    if (false == m_initialized) {
        throw OperationFailed(ErrorCode_NotReady, __FILE__, __LINE__);
    }
    if (StatusCode::NotInit != get_status_code()) {
        throw OperationFailed(ErrorCode_NotReady, __FILE__, __LINE__);
    }
    m_src_url = src_url;
    set_status_code(StatusCode::InProgress);
    m_transfer_thread = std::make_unique<std::thread>(
            transfer_thread_entry,
            std::ref(*this),
            offset,
            disable_caching
    );
    m_transfer_thread->detach();
}

auto StreamingReader::terminate_current_transfer() -> void {
    if (StatusCode::InProgress != get_status_code()) {
        while (false == is_transfer_terminated()) {
        }
        return;
    }
    // If control flow reaches here, it means the we need to kill the current connected session.
    // Since the fetcher thread is an async daemon thread, we need to set the flag for aborting
    // and wait for it to terminate.
    // TODO: we could use sleep here instead of actively pulling.
    auto transfer_aborted{is_transfer_aborted()};
    while (false == is_transfer_terminated()) {
        if (transfer_aborted) {
            continue;
        }
        abort_data_transfer();
        transfer_aborted = true;
    }
}

auto StreamingReader::abort_data_transfer() -> void {
    m_transfer_aborted.store(true);
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
        if (std::cv_status::timeout
            != m_cv_fetcher.wait_for(
                    buffer_resource_lock,
                    std::chrono::milliseconds(cConditionVariableTimeoutMilliSecond)
            ))
        {
            continue;
        }
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
    auto const fetching_buffer{m_fetching_buffer.value()};
    m_fetched_buffer_queue.emplace(fetching_buffer.data(), m_fetching_buffer_pos);
    ++m_num_fetched_buffer;

    m_fetching_buffer.reset();
    m_fetching_buffer_pos = 0;
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
        if (is_transfer_terminated()) {
            return false;
        }
        m_cv_reader.wait_for(
                buffer_resource_lock,
                std::chrono::milliseconds(cConditionVariableTimeoutMilliSecond)
        );
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

auto StreamingReader::commit_fetching(size_t num_bytes_fetched) -> void {
    m_fetching_buffer_pos += num_bytes_fetched;
    if (m_fetching_buffer_pos > m_buffer_size) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    if (m_fetching_buffer_pos == m_buffer_size) {
        commit_fetching_buffer();
    }
}

auto StreamingReader::commit_reading(size_t num_bytes_read) -> void {
    if (false == m_reading_buffer.has_value()) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    auto const reading_buffer_size{m_reading_buffer.value().size()};
    if (reading_buffer_size < num_bytes_read) {
        throw OperationFailed(ErrorCode_Corrupt, __FILE__, __LINE__);
    }
    auto const num_bytes_left{reading_buffer_size - num_bytes_read};
    if (0 == num_bytes_left) {
        free_reading_buffer();
    } else {
        m_reading_buffer = m_reading_buffer.value().last(num_bytes_left);
    }
}

auto StreamingReader::get_buffer_to_fetch(BufferView& fetching_buffer) -> bool {
    if (false == m_fetching_buffer.has_value()) {
        if (false == set_fetching_buffer() || false == m_fetching_buffer.has_value()) {
            return false;
        }
        fetching_buffer = m_fetching_buffer.value();
        return true;
    }
    fetching_buffer = m_fetching_buffer.value().subspan(
            m_fetching_buffer_pos,
            m_buffer_size - m_fetching_buffer_pos
    );
    return true;
}

auto StreamingReader::reset() -> void {
    if (StatusCode::NotInit == get_status_code()) {
        return;
    }

    if (false == m_transfer_terminated.load()) {
        throw OperationFailed(ErrorCode_Failure, __FILE__, __LINE__);
    }

    m_src_url.clear();
    m_file_pos = 0;

    std::queue<BufferView> empty_buffer_queue;
    m_fetched_buffer_queue.swap(empty_buffer_queue);
    m_num_fetched_buffer = 0;
    m_fetching_buffer_pos = 0;
    m_fetching_buffer.reset();
    m_reading_buffer.reset();

    m_transfer_thread.reset();
    m_transfer_aborted.store(false);
    m_transfer_terminated.store(false);
    set_status_code(StatusCode::NotInit);
    m_curl_return_code.reset();
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
        if (false == m_reading_buffer.has_value()) {
            if (false == set_reading_buffer() || false == m_reading_buffer.has_value()) {
                return ErrorCode_EndOfFile;
            }
        }
        auto const reading_buffer{m_reading_buffer.value()};
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
        commit_reading(num_bytes_to_consume_from_buffer);
    }
    return ErrorCode_Success;
}
}  // namespace clp
