#ifndef CLP_FILEDESCRIPTOR_HPP
#define CLP_FILEDESCRIPTOR_HPP

#include <fcntl.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * A class to wrap C style file descriptor.
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
     * A C++ wrapper for Unix oflag that describes the operation mode.
     */
    // NOLINTNEXTLINE(performance-enum-size)
    enum class Mode : int {
        ReadOnly = O_RDONLY,
        CreateForWrite = O_WRONLY | O_CREAT | O_TRUNC,
    };

    // Constructors
    /**
     * @param path
     * @param mode
     * @param close_failure_callback
     */
    FileDescriptor(
            std::string_view path,
            Mode mode,
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
     * @return The operation mode.
     */
    [[nodiscard]] auto get_mode() const -> Mode { return m_mode; }

private:
    int m_fd{-1};
    Mode m_mode;
    CloseFailureCallback m_close_failure_callback{nullptr};
};
}  // namespace clp

#endif
