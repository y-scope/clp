#ifndef CLP_NETWORKREADER_HPP
#define CLP_NETWORKREADER_HPP

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <curl/curl.h>

#include "CurlDownloadHandler.hpp"
#include "Defs.h"
#include "ErrorCode.hpp"
#include "ReaderInterface.hpp"
#include "Thread.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * This class implements the ReaderInterface to stream data directly from a given URL (e.g.,
 * streaming from a pre-signed S3 URL). This class uses a background thread to download and buffer
 * data in buffers from a buffer pool. When buffering data, if no empty buffer is available, the
 * downloading thread will block until there is, or until the download times out. Any read
 * operations will read from the next filled buffer from a queue. If no filled buffer is available,
 * the thread calling read will block until there is a filled buffer, or the download times out.
 */
class NetworkReader : public ReaderInterface {
public:
    // Types
    using BufferView = std::span<char>;

    /**
     * The exception thrown by this class.
     */
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "clp::NetworkReader operation failed.";
        }
    };

    /**
     * The possible states of the reader.
     */
    enum class State : uint8_t {
        // InProgress: The reader is downloading data from the source URL.
        InProgress = 0,
        // Failed: The reader has failed to download data from the source URL.
        Failed,
        // Finished: The data at the given URL has been downloaded.
        Finished
    };

    // Constants
    static constexpr size_t cDefaultBufferPoolSize{8};
    static constexpr size_t cDefaultBufferSize{4096};

    static constexpr size_t cMinBufferPoolSize{2};
    static constexpr size_t cMinBufferSize{512};

    /**
     * Initializes static resources for this class. This must be called before using the class.
     * @return ErrorCode_Success on success.
     * @return ErrorCode_Failure if libcurl initialization failed.
     */
    [[nodiscard]] static auto init() -> ErrorCode;

    /**
     * De-initializes any static resources.
     */
    static auto deinit() -> void;

    /**
     * Constructs a reader to stream data from the given URL, starting at the given offset.
     * TODO: the current implementation doesn't handle the case when the given offset is out of
     * range. The file_pos will be set to an invalid state if this happens, which can be
     * problematic if the other part of the program depends on this position. It can be fixed by
     * capturing the error code 416 in the response header.
     * @param src_url
     * @param offset Index of the byte at which to start the download
     * @param disable_caching Whether to disable the caching.
     * @param overall_timeout Maximum time (in seconds) that the download may take. Note that this
     * includes `connection_timeout`. Doc: https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
     * @param connection_timeout Maximum time (in seconds) that the connection phase may take.
     * Doc: https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
     * @param buffer_pool_size The required number of buffers in the buffer pool.
     * @param buffer_size The size of each buffer in the buffer pool.
     */
    explicit NetworkReader(
            std::string_view src_url,
            size_t offset = 0,
            bool disable_caching = false,
            std::chrono::seconds overall_timeout_int_sec
            = CurlDownloadHandler::cDefaultOverallTimeout,
            std::chrono::seconds connection_timeout_in_sec
            = CurlDownloadHandler::cDefaultConnectionTimeout,
            size_t buffer_pool_size = cDefaultBufferPoolSize,
            size_t buffer_size = cDefaultBufferSize
    );

    // Destructor
    virtual ~NetworkReader();

    // Copy/Move Constructors
    // These are disabled since this class' synchronization primitives are non-copyable and
    // non-moveable.
    NetworkReader(NetworkReader const&) = delete;
    NetworkReader(NetworkReader&&) = delete;
    auto operator=(NetworkReader const&) -> NetworkReader& = delete;
    auto operator=(NetworkReader&&) -> NetworkReader& = delete;

    // Methods implementing `clp::ReaderInterface`
    /**
     * Tries to read up to a given number of bytes from the buffered data.
     * @param buf
     * @param num_bytes_to_read
     * @param num_bytes_read Returns the number of bytes read.
     * @return ErrorCode_EndOfFile if there is no more buffered data.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto
    try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) -> ErrorCode override {
        return read_from_filled_buffers(num_bytes_to_read, num_bytes_read, buf);
    }

    /**
     * Tries to seek to the given position, relative to the beginning of the data.
     * @param pos
     * @return ErrorCode_Unsupported if the given position is lower than the current position.
     * Since only streaming is supported, it cannot seek backwards.
     * @return ErrorCode_OutOfBounds if the given pos is past the end of the data.
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
        auto const err{read_from_filled_buffers(pos - m_file_pos, num_bytes_read, nullptr)};
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
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) -> ErrorCode override {
        pos = m_file_pos;
        return ErrorCode_Success;
    }

    // Methods
    /**
     * @return true if the download (curl transfer) has been killed, false otherwise.
     */
    [[nodiscard]] auto is_download_aborted() const -> bool { return m_download_aborted.load(); }

    /**
     * @return true the downloader thread is running, false otherwise.
     */
    [[nodiscard]] auto is_downloader_thread_running() const -> bool {
        if (nullptr == m_downloader_thread) {
            return false;
        }
        return m_downloader_thread->is_running();
    }

    /**
     * Buffers the downloaded data to the currently acquired or next available buffer from the
     * buffer pool.
     * NOTE: This function should be called by the libcurl write callback only.
     * @param data
     * @return Number of bytes buffered.
     */
    [[nodiscard]] auto buffer_downloaded_data(BufferView data) -> size_t;

    /**
     * @return true if the CURL data downloading is still in progress.
     * @return false if there will be no more data downloaded.
     */
    [[nodiscard]] auto is_download_in_progress() const -> bool {
        return get_state_code() == State::InProgress;
    }

    /**
     * @returns Whether the download has timed out
     */
    [[nodiscard]] auto is_download_timedout() const -> bool {
        return m_curl_return_code.has_value()
               && CURLE_OPERATION_TIMEDOUT == m_curl_return_code.value();
    }

    [[nodiscard]] auto get_curl_return_code() const -> std::optional<CURLcode> {
        return m_curl_return_code;
    }

private:
    /**
     * This class implements clp::Thread to download data using CURL.
     */
    class DownloaderThread : public Thread {
    public:
        // Constructor
        /**
         * Constructs a clp::thread for data downloading.
         * @param reader
         * @param offset The offset of bytes to start downloading data.
         * @param disable_caching Whether to disable caching.
         */
        DownloaderThread(NetworkReader& reader, size_t offset, bool disable_caching)
                : m_reader{reader},
                  m_offset{offset},
                  m_disable_caching{disable_caching} {}

    private:
        // Methods implementing `clp::Thread`
        auto thread_method() -> void final;

        NetworkReader& m_reader;
        size_t m_offset{0};
        bool m_disable_caching{false};
    };

    static bool m_initialized;

    /**
     * Aborts the current on-going curl download session.
     */
    auto abort_data_download() -> void;

    /**
     * Acquires an empty buffer to write downloaded data.
     */
    auto acquire_empty_buffer() -> void;

    /**
     * Enqueues the current downloader buffer into the filled buffer queue.
     */
    auto enqueue_filled_buffer() -> void;

    /**
     * Gets a filled buffer from the filled buffer queue.
     */
    auto get_filled_buffer() -> void;

    /**
     * Releases an empty buffer which has been fully consumed by the reader.
     */
    auto release_empty_buffer() -> void;

    /**
     * Reads data from the filled buffers with a given amount of bytes.
     * @param num_bytes_to_read
     * @param num_bytes_read Returns the number of bytes read.
     * @param dst The pointer to the destination buffer. If the buffer is not nullptr, copy the data
     * to the destination.
     * @return ErrorCode_EndOfFile if the buffer doesn't contain any more data.
     * @return ErrorCode_Success on success.
     */
    [[nodiscard]] auto read_from_filled_buffers(
            size_t num_bytes_to_read,
            size_t& num_bytes_read,
            char* dst
    ) -> ErrorCode;

    auto set_state_code(State code) -> void { m_state_code.store(code); }

    [[nodiscard]] auto get_state_code() const -> State { return m_state_code.load(); }

    std::string m_src_url;
    size_t m_file_pos;

    size_t m_buffer_pool_size;
    size_t m_buffer_size;
    size_t m_curr_downloader_buf_idx{0};

    std::chrono::seconds m_overall_timeout;
    std::chrono::seconds m_connection_timeout;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    std::vector<std::unique_ptr<char[]>> m_buffer_pool;
    std::queue<BufferView> m_filled_buffer_queue;
    std::optional<BufferView> m_curr_downloader_buf{std::nullopt};
    std::optional<BufferView> m_curr_reader_buf{std::nullopt};

    std::mutex m_buffer_resource_mutex;
    std::condition_variable m_cv_downloader;
    std::condition_variable m_cv_reader;

    std::unique_ptr<DownloaderThread> m_downloader_thread{nullptr};
    std::atomic<bool> m_download_aborted{false};
    std::atomic<State> m_state_code{State::InProgress};
    std::optional<CURLcode> m_curl_return_code;
};
}  // namespace clp

#endif  // CLP_NETWORKREADER_HPP
