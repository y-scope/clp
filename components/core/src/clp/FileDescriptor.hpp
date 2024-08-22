#ifndef CLP_FILEDESCRIPTOR_HPP
#define CLP_FILEDESCRIPTOR_HPP

#include <fcntl.h>
#include <sys/stat.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * Wrapper for a UNIX file descriptor.
 */
class FileDescriptor {
public:
    // Types
    /**
     * `close` is called in the destructor to close the file descriptor. However, `close` may return
     * an error indicated by `errno`. This type alias defines a callback to handle the `close`
     * failure in the destructor.
     * The signature of the callback: void close_failure_callback(int errno)
     */
    using CloseFailureCallback = void (*)(int);

    class OperationFailed : public TraceableException {
    public:
        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string msg
        )
                : TraceableException{error_code, filename, line_number},
                  m_msg{std::move(msg)} {}

        [[nodiscard]] auto what() const noexcept -> char const* override { return m_msg.c_str(); }

    private:
        std::string m_msg;
    };

    /**
     * A C++ wrapper for Unix oflag that describes the open mode.
     */
    // NOLINTNEXTLINE(performance-enum-size)
    enum class OpenMode : int {
        ReadOnly = O_RDONLY,
        CreateForWrite = O_WRONLY | O_CREAT | O_TRUNC,
    };

    // Constructors
    FileDescriptor(
            std::string_view path,
            OpenMode open_mode,
            CloseFailureCallback close_failure_callback = nullptr
    );

    // Destructor
    ~FileDescriptor();

    // Disable copy/move constructors/assignment operators
    FileDescriptor(FileDescriptor const&) = delete;
    FileDescriptor(FileDescriptor&&) = delete;
    auto operator=(FileDescriptor const&) -> FileDescriptor& = delete;
    auto operator=(FileDescriptor&&) -> FileDescriptor& = delete;

    /**
     * @return The raw fd.
     */
    [[nodiscard]] auto get_raw_fd() const -> int { return m_fd; }

    /**
     * @return The size of the file.
     */
    [[nodiscard]] auto get_size() const -> size_t;

    /**
     * @return The open mode.
     */
    [[nodiscard]] auto get_open_mode() const -> OpenMode { return m_open_mode; }

    /**
     * Obtains information about the open file associated with the underlying file descriptor.
     * @param stat_buffer Returns the stat results.
     * @return ErrorCode_Success on success.
     * @return ErrorCode_errno on error.
     */
    [[nodiscard]] auto stat(struct stat& stat_buffer) const -> ErrorCode;

private:
    int m_fd{-1};
    OpenMode m_open_mode;
    CloseFailureCallback m_close_failure_callback{nullptr};
};
}  // namespace clp

#endif
