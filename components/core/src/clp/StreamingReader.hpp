#ifndef CLP_STREAMINGREADER_HPP
#define CLP_STREAMINGREADER_HPP

#include <condition_variable>
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
#include "TraceableException.hpp"

namespace clp {
/**
 * This class implements a ReaderInterface to stream data directly from a given URL (i.e., streaming
 * from a pre-signed S3 URL). When open an URL< a daemon thread will be created to fetch data in the
 * background using libcurl. This class guarantees the synchronization between the read operations
 * and data streaming (fetching). However, it is assumed that both the reader and the data fetcher
 * are single threaded.
 */
class StreamingReader : public ReaderInterface {
public:
    using BufferView = std::span<char>;

    // Types
    /**
     * This class defines the exception thrown by the streaming reader.
     */
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "StreamingReader operation failed.";
        }
    };

    /**
     * This enum defines the states of the reader.
     * NotInit: The reader is not initialized with a source URL.
     * Failed: The reader has failed to stream data from the source URL.
     * InProgress: The reader is fetching data from the source URL.
     * Finished: The streaming of the given source URL has been accomplished.
     */
    enum class StatusCode : uint8_t {
        NotInit = 0,
        Failed,
        InProgress,
        Finished
    };

    /**
     * These static members defines the default buffer size and the number of pools.
     */
    static constexpr size_t cDefaultBufferPoolSize{8};
    static constexpr size_t cDefaultBufferSize{4096};

    /**
     * These static members defines the minimum allowed buffer size and the number of pools.
     */
    static constexpr size_t cMinimalBufferSize{512};
    static constexpr size_t cMinimalBufferPoolSize{2};

    /**
     * These static members defines the default connection timeout and operation timeout.
     * Please refer to the libcurl documentation for more details:
     * ConnectionTimeout: https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
     * OperationTimeout: https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
     */
    static constexpr uint32_t cDefaultConnectionTimeout{10};
    static constexpr uint32_t cDefaultOperationTimeout{45};

    /**
     * Initializes the underlying libcurl functionalities globally. It should be called before
     * opening a source URL for data fetching.
     * @return ErrorCode_Success on success.
     * @return ErrorCode_Failure if the libcurl initialization failed.
     */
    [[nodiscard]] static auto global_init() -> ErrorCode;

    /**
     * Releases the globally initialized libcurl resources.
     */
    static auto global_cleanup() -> void;

    /**
     * Constructor.
     * @param buffer_pool_size Total number of buffers available for fetching.
     * @param buffer_size The size of each data buffer.
     */
    explicit StreamingReader(
            size_t buffer_pool_size = cDefaultBufferPoolSize,
            size_t buffer_size = cDefaultBufferSize
    )
            : m_buffer_pool_size{buffer_pool_size},
              m_buffer_size{buffer_size} {
        if (cMinimalBufferSize > m_buffer_size) {
            m_buffer_size = cMinimalBufferSize;
        }
        if (cMinimalBufferPoolSize > m_buffer_pool_size) {
            m_buffer_pool_size = cMinimalBufferPoolSize;
        }
        for (size_t i = 0; i < m_buffer_pool_size; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
            m_buffer_pool.emplace_back(std::make_unique<char[]>(m_buffer_size));
        }
    }

    /**
     * Destructor.
     */
    virtual ~StreamingReader() {
        if (StatusCode::NotInit == get_status_code()) {
            return;
        }
        // We must ensure the transfer has been terminated before we cleanup the allocated
        // resources. Otherwise, the transfer thread may trigger UB.
        terminate_current_transfer();
    }

    /**
     * Copy/Move Constructors.
     * They are all disabled since the synchronization primitives are non-copyable and non-moveable.
     */
    StreamingReader(StreamingReader const&) = delete;
    StreamingReader(StreamingReader&&) = delete;
    auto operator=(StreamingReader const&) -> StreamingReader& = delete;
    auto operator=(StreamingReader&&) -> StreamingReader& = delete;

    // Methods implementing `ReaderInterface`
    /**
     * Tries to read up to a given number of bytes from the buffer.
     * @param buf
     * @param num_bytes_to_read
     * @param num_bytes_read Returns the number of bytes read.
     * @return ErrorCode_EndOfFile if the buffer doesn't contain any more data.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
            -> ErrorCode override {
        return read_from_fetched_buffers(num_bytes_to_read, num_bytes_read, buf);
    }

    /**
     * Tries to seek to the given position, relative to the beginning of the buffer.
     * @param pos
     * @return ErrorCode_Unsupported if the given position is lower than the current position.
     * Since this is a streaming reader, it should not seek backward.
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
     * @return ErrorCode_Success.
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) -> ErrorCode override {
        pos = m_file_pos;
        return ErrorCode_Success;
    }

    // Methods
    /**
     * Opens a new connection to stream data from the given url with a given offset.
     * TODO: the current implementation doesn't handle the case when the given offset is out of
     * range. The file_pos will be set to an invalid state if this happens, which can be
     * problematic if the other part of the program depends on this position. It can be fixed by
     * capturing the error code 416 in the response header.
     * @param src_url
     * @param offset The offset of the starting byte used for data fetching.
     * @param disable_caching Whether to disable the caching.
     */
    auto open(std::string_view src_url, size_t offset = 0, bool disable_caching = false) -> void;

    /**
     * Closes the current data transfer and resets the buffer state.
     */
    auto close() -> void {
        if (StatusCode::NotInit == get_status_code()) {
            return;
        }
        terminate_current_transfer();
        reset();
    }

    /**
     * Sets the connection timeout in seconds. This value will be used when trying to build the
     * connections. The default value is set to `cDefaultConnectionTimeout`.
     * Doc: https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
     * @param connection_timeout_in_sec
     */
    auto set_connection_timeout(uint32_t connection_timeout_in_sec) -> void {
        m_connection_timeout = connection_timeout_in_sec;
    }

    /**
     * Sets the operation timeout in seconds. This values determines the maximum time a transfer
     * can take. The default value is set to `cDefaultOperationTimeout`.
     * Doc: https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
     * @param operation_timeout_in_sec
     */
    auto set_operation_timeout(uint32_t operation_timeout_in_sec) -> void {
        m_operation_timeout = operation_timeout_in_sec;
    }

    /**
     * @return true if the transfer has been killed, false otherwise.
     */
    [[nodiscard]] auto is_transfer_aborted() const -> bool { return m_transfer_aborted.load(); }

    /**
     * @return true the transfer has terminated, false otherwise.
     */
    [[nodiscard]] auto is_transfer_terminated() const -> bool {
        return m_transfer_terminated.load();
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

private:
    static constexpr uint32_t cConditionVariableTimeoutMilliSecond{50};

    static bool m_initialized;

    /**
     * The entry of the transfer thread.
     * @param reader
     * @param offset
     * @param disable_caching
     */
    static auto transfer_thread_entry(StreamingReader& reader, size_t offset, bool disable_caching)
            -> void;

    /**
     * Progress callback. It should be used to determine whether to abort the current transfer.
     * Doc: https://curl.se/libcurl/c/CURLOPT_XFERINFOFUNCTION.html
     * @param ptr A pointer to an instance of StreamingReader.
     * @param dltotal Unused
     * @param dlnow Unused
     * @param ultotal Unused
     * @param ulnow Unused
     * @return 0 if the transfer is not aborted; 1 otherwise.
     */
    static auto progress_callback(
            void* ptr,
            [[maybe_unused]] curl_off_t dltotal,
            [[maybe_unused]] curl_off_t dlnow,
            [[maybe_unused]] curl_off_t ultotal,
            [[maybe_unused]] curl_off_t ulnow
    ) -> int;

    /**
     * Write-back callback. This is how the data is written into the fetching buffer.
     * Doc: https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
     * @param ptr A pointer to an instance of StreamingReader.
     * @param size Number of bytes of the input data.
     * @param nmemb
     * @param reader_ptr A pointer pointing to an instance of StreamingReader.
     * @return Number of bytes transferred (fetched).
     */
    static auto write_callback(char* ptr, size_t size, size_t nmemb, void* reader_ptr) -> size_t;

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
     * Resets the buffer states and transfer manager.
     * Note:
     * 1. It is assumed that the thread that calls `reset` would be the reader thread that executed
     *  `open`.
     * 2. The transfer should already be terminated (either naturally or aborted) before calling
     *  this thread.
     */
    auto reset() -> void;

    /**
     * Reads data from the fetched buffers with a given amount of bytes.
     * @param num_bytes_to_read
     * @param num_bytes_read Returns the number of bytes read.
     * @param dst The pointer to the destination buffer. If the buffer is not nullptr, copy the data
     * to the destination.
     * @return ErrorCode_EndOfFile if the buffer doesn't contain any more data.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto
    read_from_fetched_buffers(size_t num_bytes_to_read, size_t& num_bytes_read, char* dst)
            -> ErrorCode;

    auto set_status_code(StatusCode code) -> void { m_status_code.store(code); }

    [[nodiscard]] auto get_status_code() -> StatusCode { return m_status_code.load(); }

    std::string m_src_url;
    size_t m_file_pos{0};

    size_t m_buffer_pool_size;
    size_t m_buffer_size;
    size_t m_num_fetched_buffer{0};
    size_t m_curr_fetching_buffer_idx{0};
    size_t m_fetching_buffer_pos{0};

    uint32_t m_connection_timeout{cDefaultConnectionTimeout};
    uint32_t m_operation_timeout{cDefaultOperationTimeout};

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    std::vector<std::unique_ptr<char[]>> m_buffer_pool;
    std::queue<BufferView> m_fetched_buffer_queue;
    std::optional<BufferView> m_fetching_buffer{std::nullopt};
    std::optional<BufferView> m_reading_buffer{std::nullopt};

    std::mutex m_buffer_resource_mutex;
    std::condition_variable m_cv_fetcher;
    std::condition_variable m_cv_reader;

    std::unique_ptr<std::thread> m_transfer_thread{nullptr};
    std::atomic<bool> m_transfer_aborted{false};
    std::atomic<bool> m_transfer_terminated{false};
    std::atomic<StatusCode> m_status_code{StatusCode::NotInit};
    std::optional<CURLcode> m_curl_return_code{std::nullopt};
};
}  // namespace clp

#endif  // CLP_STREAMINGREADER_HPP
