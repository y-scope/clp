#ifndef NETWORKING_SOCKET_UTILS_HPP
#define NETWORKING_SOCKET_UTILS_HPP

// C++ standard libraries
#include <cstddef>

// Project headers
#include "../ErrorCode.hpp"

namespace networking {
    // Methods
    /**
     * Tries to send a buffer of data over the socket
     * @param fd
     * @param buf
     * @param buf_len
     * @return ErrorCode_BadParam if the file descriptor or buffer pointer is invalid
     * @return ErrorCode_errno if sending failed
     * @return ErrorCode_Success otherwise
     */
    ErrorCode try_send (int fd, const char* buf, size_t buf_len);
    /**
     * Sends a buffer of data over the socket
     * @param fd
     * @param buf
     * @param buf_len
     */
    void send (int fd, const char* buf, size_t buf_len);

    /**
     * Tries to receive up to a given number of bytes over a socket
     * @param buf Buffer to store received bytes
     * @param buf_len Number of bytes to receive
     * @return ErrorCode_BadParam if file descriptor or buffer pointer are invalid
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_errno if receiving failed
     * @return ErrorCode_Success otherwise
     */
    ErrorCode try_receive (int fd, char* buf, size_t buf_len, size_t& num_bytes_received);
    /**
     * Receives up to the give number of bytes over a socket
     * @param buf Buffer to store received bytes
     * @param buf_len Number of bytes to receive
     */
    void receive (int fd, char* buf, size_t buf_len, size_t& num_bytes_received);
}

#endif //NETWORKING_SOCKET_UTILS_HPP
