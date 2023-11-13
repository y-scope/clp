#include "socket_utils.hpp"

#include <sys/socket.h>

#include <cstdio>

#include "../Defs.h"
#include "SocketOperationFailed.hpp"

namespace networking {
ErrorCode try_send(int fd, char const* buf, size_t buf_len) {
    if (fd < 0 || nullptr == buf) {
        return ErrorCode_BadParam;
    }

    ssize_t num_bytes_sent = ::send(fd, buf, buf_len, 0);
    if (-1 == num_bytes_sent) {
        return ErrorCode_errno;
    }

    return ErrorCode_Success;
}

void send(int fd, char const* buf, size_t buf_len) {
    auto error_code = try_send(fd, buf, buf_len);
    if (ErrorCode_Success != error_code) {
        throw SocketOperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

ErrorCode try_receive(int fd, char* buf, size_t buf_len, size_t& num_bytes_received) {
    if (fd < 0 || nullptr == buf) {
        return ErrorCode_BadParam;
    }

    ssize_t result = recv(fd, buf, buf_len, 0);
    if (result < 0) {
        return ErrorCode_errno;
    }
    if (0 == result) {
        return ErrorCode_EndOfFile;
    }
    num_bytes_received = result;

    return ErrorCode_Success;
}

void receive(int fd, char* buf, size_t buf_len, size_t& num_bytes_received) {
    auto error_code = try_receive(fd, buf, buf_len, num_bytes_received);
    if (ErrorCode_Success != error_code) {
        throw SocketOperationFailed(error_code, __FILENAME__, __LINE__);
    }
}
}  // namespace networking
