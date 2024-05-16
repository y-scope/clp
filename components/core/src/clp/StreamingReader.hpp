#ifndef CLP_STREAMINGREADER_HPP
#define CLP_STREAMINGREADER_HPP

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <curl/curl.h>

#include "Defs.h"
#include "ErrorCode.hpp"
#include "ReaderInterface.hpp"
#include "Thread.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * This class implements a ReaderInterface to stream data directly from a given URL (e.g., streaming
 * from a pre-signed S3 URL). When opening a URL, a daemon thread will be created to fetch data in
 * the background using libcurl.
 *
 * TODO Move into implementation details? Should also describe buffer pooling behaviour.
 * This class guarantees synchronization between the read operations
 * and data streaming (fetching). However, it is assumed that both the reader and the data fetcher
 * are single threaded.
 */
class StreamingReader : public ReaderInterface {
public:
    using BufferView = std::span<char>;

    // Types
    /**
     * The exception thrown by this class.
     */
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {
            std::cerr << std::string{filename} << ": " << line_number << "\n";
        }

        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "clp::StreamingReader operation failed.";
        }
    };

    /**
     * Enum defining the states of the reader.
     */
    enum class State : uint8_t {
        // InProgress: The reader is fetching data from the source URL.
        InProgress = 0,
        // Failed: The reader has failed to stream data from the source URL.
        Failed,
        // Finished: The streaming of the given source URL has been accomplished.
        Finished
    };

    static constexpr size_t cDefaultBufferPoolSize{8};
    static constexpr size_t cDefaultBufferSize{4096};

    static constexpr size_t cMinBufferPoolSize{2};
    static constexpr size_t cMinBufferSize{512};

    /**
     * These static members defines the default connection timeout and operation timeout.
     * Please refer to the libcurl documentation for more details:
     * ConnectionTimeout: https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
     * OverallTimeout: https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
     */
    static constexpr uint32_t cDefaultConnectionTimeout{10};  // Seconds
    static constexpr uint32_t cDefaultOverallTimeout{45};  // Seconds

    /**
     * Initializes the underlying libcurl functionalities globally. It should be called before
     * opening a source URL for data fetching.
     * @return ErrorCode_Success on success.
     * @return ErrorCode_Failure if libcurl initialization failed.
     */
    [[nodiscard]] static auto init() -> ErrorCode;

    /**
     * Releases the globally initialized libcurl resources.
     */
    static auto deinit() -> void;

    /**
     * Constructs a new connection to stream data from the given url with a given offset.
     * TODO: the current implementation doesn't handle the case when the given offset is out of
     * range. The file_pos will be set to an invalid state if this happens, which can be
     * problematic if the other part of the program depends on this position. It can be fixed by
     * capturing the error code 416 in the response header.
     * @param src_url
     * @param offset The offset of the starting byte used for data fetching.
     * @param disable_caching Whether to disable the caching.
     * @param overall_timeout_in_sec The overall timeout in seconds used by the underlying CURL
     * handler. Doc: https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
     * @param connection_timeout_in_sec The connection timeout in seconds used by the underlying
     * CURL handler. Doc: https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
     * @param buffer_pool_size Total number of buffers available for fetching.
     * @param buffer_size The size of each data buffer.
     */
    explicit StreamingReader(
            std::string_view src_url,
            size_t offset = 0,
            bool disable_caching = false,
            uint32_t overall_timeout_int_sec = cDefaultOverallTimeout,
            uint32_t connection_timeout_in_sec = cDefaultConnectionTimeout,
            size_t buffer_pool_size = cDefaultBufferPoolSize,
            size_t buffer_size = cDefaultBufferSize
    );

    /**
     * Destructor.
     */
    virtual ~StreamingReader();

    /**
     * Copy/Move Constructors.
     * They are all disabled since the synchronization primitives are non-copyable and non-moveable.
     */
    StreamingReader(StreamingReader const&) = delete;
    StreamingReader(StreamingReader&&) = delete;
    auto operator=(StreamingReader const&) -> StreamingReader& = delete;
    auto operator=(StreamingReader&&) -> StreamingReader& = delete;

    // Methods implementing `clp::ReaderInterface`
    /**
     * Tries to read up to a given number of bytes from the buffer.
     * @param buf
     * @param num_bytes_to_read
     * @param num_bytes_read Returns the number of bytes read.
     * @return ErrorCode_EndOfFile if the buffer doesn't contain any more data.
     * @return ErrorCode_NotInit if the reader is not opened yet.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto
    try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) -> ErrorCode override {
        return read_from_fetched_buffers(num_bytes_to_read, num_bytes_read, buf);
    }

    /**
     * Tries to seek to the given position, relative to the beginning of the buffer.
     * @param pos
     * @return ErrorCode_Unsupported if the given position is lower than the current position.
     * Since this is a streaming reader, it should not seek backward.
     * @return ErrorCode_NotInit if the reader is not opened yet.
     * @return ErrorCode_OutOfBounds if the given pos is out of bound.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto try_seek_from_begin(size_t pos) -> ErrorCode override {
        if (pos < m_file_pos) {
            return ErrorCode_Unsupported;
        }
        if (pos == m_file_pos) {
            return ErrorCode_Success;
        }
        size_t num_bytes_read{};
        auto const err{read_from_fetched_buffers(pos - m_file_pos, num_bytes_read, nullptr)};
        if (ErrorCode_EndOfFile == err) {
            return ErrorCode_OutOfBounds;
        }
        if (m_file_pos != pos) {
            return ErrorCode_Failure;
        }
        return err;
    }

    /**
     * @param pos Returns the position of the read head in the buffer.
     * @return ErrorCode_NotInit if the reader is not opened yet.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) -> ErrorCode override {
        pos = m_file_pos;
        return ErrorCode_Success;
    }

    /**
     * @return true if the transfer has been killed, false otherwise.
     */
    [[nodiscard]] auto is_transfer_aborted() const -> bool { return m_transfer_aborted.load(); }

    /**
     * @return true the transfer thread is running, false otherwise.
     */
    [[nodiscard]] auto is_transfer_thread_running() const -> bool {
        if (nullptr == m_transfer_thread) {
            return false;
        }
        return m_transfer_thread->is_running();
    }

    /**
     * @returns true if the transfer has timed out; false otherwise.
     */
    [[nodiscard]] auto is_transfer_timedout() const -> bool {
        if (false == m_curl_return_code.has_value()) {
            return false;
        }
        auto const val{m_curl_return_code.value()};
        return (CURLE_FTP_ACCEPT_TIMEOUT == val || CURLE_OPERATION_TIMEDOUT == val);
    }

    /**
     * Writes the data into the fetching buffer.
     * This function should be called by libcurl's write callback.
     * @param data_to_write
     * @return NUmber of bytes fetched.
     */
    [[nodiscard]] auto write_to_fetching_buffer(BufferView data_to_write) -> size_t;

    /**
     * @return true if the download is still in progress.
     * @return false if there is no more data to download.
     */
    [[nodiscard]] auto is_download_in_progress() const -> bool {
        return get_state_code() == State::InProgress;
    }

private:
    /**
     * This class implements clp::Thread to fetch data using CURL.
     */
    class TransferThread : public Thread {
    public:
        // Constructor
        /**
         * Constructs a clp::thread for data downloading.
         * @param reader
         * @param offset The offset of bytes to start downloading.
         * @param disable_caching Whether to disable caching.
         */
        TransferThread(StreamingReader& reader, size_t offset, bool disable_caching)
                : m_reader{reader},
                  m_offset{offset},
                  m_disable_caching{disable_caching} {}

    private:
        // Methods implementing `clp::Thread`
        auto thread_method() -> void final;

        StreamingReader& m_reader;
        size_t m_offset;
        bool m_disable_caching;
    };

    static bool m_initialized;

    /**
     * Terminates the current transfer. When this function returns, it will ensure that the current
     * data transfer session has been terminated, and all the daemon threads exit.
     */
    auto terminate_current_transfer() -> void;

    /**
     * Aborts the current on-going data transfer session.
     */
    auto abort_data_transfer() -> void;

    /**
     * Sets the next underlying buffer for fetching transferred data.
     * @return true on success, false if the transfer has been aborted.
     */
    [[nodiscard]] auto set_fetching_buffer() -> bool;

    /**
     * Commits the current fetching buffer to the fetched buffer queue.
     */
    auto commit_fetching_buffer() -> void;

    /**
     * Sets the next underlying fetched buffer for reading.
     * @return true if there is a buffer available for reading.
     * @return false otherwise.
     */
    [[nodiscard]] auto set_reading_buffer() -> bool;

    /**
     * Frees the current reading buffer.
     */
    auto free_reading_buffer() -> void;

    /**
     * Commits the number of bytes fetched. If the buffer is full, commits it to the fetched buffer
     * queue.
     * @param num_bytes_fetched
     */
    auto commit_fetching(size_t num_bytes_fetched) -> void;

    /**
     * Commits the number of bytes read. If the entire buffer is consumed, free the buffer resource.
     * @param num_bytes_read
     */
    auto commit_reading(size_t num_bytes_read) -> void;

    /**
     * Gets the buffer to fetch (which is the unfilled part of the current fetching buffer). If the
     * current fetching buffer is full, it will commit the current buffer and get the next buffer
     * from the buffer pool.
     * @param fetching_buffer Output the buffer to fetch.
     * @return true on success, false on failure (transfer aborted).
     */
    [[nodiscard]] auto get_buffer_to_fetch(BufferView& fetching_buffer) -> bool;

    /**
     * Reads data from the fetched buffers with a given amount of bytes.
     * @param num_bytes_to_read
     * @param num_bytes_read Returns the number of bytes read.
     * @param dst The pointer to the destination buffer. If the buffer is not nullptr, copy the data
     * to the destination.
     * @return ErrorCode_EndOfFile if the buffer doesn't contain any more data.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto read_from_fetched_buffers(
            size_t num_bytes_to_read,
            size_t& num_bytes_read,
            char* dst
    ) -> ErrorCode;

    auto set_state_code(State code) -> void { m_state_code.store(code); }

    [[nodiscard]] auto get_state_code() const -> State { return m_state_code.load(); }

    std::string m_src_url;
    size_t m_file_pos{0};

    size_t m_buffer_pool_size;
    size_t m_buffer_size;
    size_t m_num_fetched_buffer{0};
    size_t m_curr_fetching_buffer_idx{0};
    size_t m_fetching_buffer_pos{0};

    uint32_t m_overall_timeout;
    uint32_t m_connection_timeout;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    std::vector<std::unique_ptr<char[]>> m_buffer_pool;
    std::queue<BufferView> m_fetched_buffer_queue;
    std::optional<BufferView> m_fetching_buffer{std::nullopt};
    std::optional<BufferView> m_reading_buffer{std::nullopt};

    std::mutex m_buffer_resource_mutex;
    std::condition_variable m_cv_fetcher;
    std::condition_variable m_cv_reader;

    std::unique_ptr<TransferThread> m_transfer_thread{nullptr};
    std::atomic<bool> m_transfer_aborted{false};
    std::atomic<State> m_state_code{State::InProgress};
    std::optional<CURLcode> m_curl_return_code{std::nullopt};
};
}  // namespace clp

#endif  // CLP_STREAMINGREADER_HPP
