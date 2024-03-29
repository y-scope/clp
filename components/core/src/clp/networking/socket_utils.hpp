#ifndef CLP_NETWORKING_SOCKET_UTILS_HPP
#define CLP_NETWORKING_SOCKET_UTILS_HPP

#include <cstddef>
#include <string>

#include "../ErrorCode.hpp"

namespace clp::networking {
// Methods
/**
 * Opens a TCP connection to a server listening on the given host and port.
 * @return An open socket file descriptor on success
 * @return -1 on any error
 */
int connect_to_server(std::string const& host, std::string const& port);

/**
 * Tries to send a buffer of data over the socket
 * @param fd
 * @param buf
 * @param buf_len
 * @return ErrorCode_BadParam if the file descriptor or buffer pointer is invalid
 * @return ErrorCode_errno if sending failed
 * @return ErrorCode_Success otherwise
 */
ErrorCode try_send(int fd, char const* buf, size_t buf_len);
/**
 * Sends a buffer of data over the socket
 * @param fd
 * @param buf
 * @param buf_len
 */
void send(int fd, char const* buf, size_t buf_len);

/**
 * Tries to receive up to a given number of bytes over a socket
 * @param buf Buffer to store received bytes
 * @param buf_len Number of bytes to receive
 * @return ErrorCode_BadParam if file descriptor or buffer pointer are invalid
 * @return ErrorCode_EndOfFile on EOF
 * @return ErrorCode_errno if receiving failed
 * @return ErrorCode_Success otherwise
 */
ErrorCode try_receive(int fd, char* buf, size_t buf_len, size_t& num_bytes_received);
/**
 * Receives up to the give number of bytes over a socket
 * @param buf Buffer to store received bytes
 * @param buf_len Number of bytes to receive
 */
void receive(int fd, char* buf, size_t buf_len, size_t& num_bytes_received);
}  // namespace clp::networking

#endif  // CLP_NETWORKING_SOCKET_UTILS_HPP
