#include "FileDescriptor.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <string_view>

#include "ErrorCode.hpp"
#include "type_utils.hpp"

namespace clp {
FileDescriptor::FileDescriptor(
        std::string_view path,
        OpenMode open_mode,
        CloseFailureCallback close_failure_callback
)
        : m_open_mode{open_mode},
          m_close_failure_callback{close_failure_callback} {
    // For newly created files, we enable writing for the owner and reading for everyone.
    // Callers can change the created file's permissions as necessary.
    constexpr auto cNewFilePermission{S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH};
    auto const flag{enum_to_underlying_type(open_mode)};
    if (0 != (flag & O_CREAT)) {
        m_fd = open(path.data(), flag, cNewFilePermission);
    } else {
        m_fd = open(path.data(), flag);
    }
    if (-1 == m_fd) {
        throw OperationFailed(
                ErrorCode_errno,
                __FILE__,
                __LINE__,
                "Failed to open file descriptor for path: " + std::string{path}
        );
    }
}

FileDescriptor::~FileDescriptor() {
    if (-1 == m_fd) {
        return;
    }
    if (0 != close(m_fd) && nullptr != m_close_failure_callback) {
        m_close_failure_callback(errno);
    }
}

auto FileDescriptor::get_size() const -> size_t {
    struct stat stat_result{};

    if (ErrorCode_Success != stat(stat_result)) {
        throw OperationFailed(
                ErrorCode_errno,
                __FILE__,
                __LINE__,
                "Failed to stat file using file descriptor."
        );
    }
    return static_cast<size_t>(stat_result.st_size);
}

auto FileDescriptor::stat(struct stat& stat_buffer) const -> ErrorCode {
    if (0 != fstat(m_fd, &stat_buffer)) {
        return ErrorCode_errno;
    }
    return ErrorCode_Success;
}
}  // namespace clp
