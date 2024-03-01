#include "ReducerNetworkUtils.hpp"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>

#include "../clp/networking/socket_utils.hpp"
#include "DeserializedRecordGroup.hpp"

namespace reducer {
namespace {
/**
 * Connect to a server listening on a given host and port
 * @return an open socket file descriptor on success
 * @return -1 on any error
 */
int connect_to_server(std::string const& host, std::string const& port) {
    // Get address info for reducer
    struct addrinfo hints = {};
    // Address can be IPv4 or IPV6
    hints.ai_family = AF_UNSPEC;
    // TCP socket
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    struct addrinfo* addresses_head = nullptr;
    int error = getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses_head);
    if (0 != error) {
        return -1;
    }

    // Try each address until a socket can be created and connected to
    int socket_fd = -1;
    for (auto curr = addresses_head; nullptr != curr; curr = curr->ai_next) {
        // Create socket
        socket_fd = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (-1 == socket_fd) {
            continue;
        }

        // Connect to address
        if (connect(socket_fd, curr->ai_addr, curr->ai_addrlen) != -1) {
            break;
        }

        // Failed to connect, so close socket
        close(socket_fd);
        socket_fd = -1;
    }
    freeaddrinfo(addresses_head);

    return socket_fd;
}

/**
 * Append data to a buffer, and flush the buffer to socket_fd if the buffer becomes full.
 * @param socket_fd
 * @param buf the underlying buffer being filled
 * @param bytes_occupied the number of occupied bytes in the buffer; updated by reference
 * @param buf_size the total number of bytes that the buffer can hold
 * @param data the data being written to the buffer
 * @param size the number of bytes of data to add to the buffer
 * @return true on success
 * @return false on any error
 */
bool append_to_bufffer_and_send(
        int socket_fd,
        char* buf,
        size_t& bytes_occupied,
        size_t buf_size,
        char const* data,
        size_t size
) {
    if ((bytes_occupied + size) <= buf_size) {
        memcpy(&buf[bytes_occupied], data, size);
        bytes_occupied += size;
        return true;
    }

    do {
        size_t space_left = buf_size - bytes_occupied;
        if (space_left > 0) {
            memcpy(&buf[bytes_occupied], data, space_left);
            data += space_left;
            size -= space_left;
            bytes_occupied += space_left;
        }
        auto ecode = clp::networking::try_send(socket_fd, buf, bytes_occupied);
        if (clp::ErrorCode::ErrorCode_Success != ecode) {
            return false;
        }

        bytes_occupied = 0;
    } while (size > buf_size);

    if (size > 0) {
        memcpy(&buf[bytes_occupied], data, size);
        bytes_occupied += size;
    }
    return true;
}
}  // namespace

int connect_to_reducer(std::string const& host, int port, int64_t job_id) {
    int socket_fd = connect_to_server(host, std::to_string(port));
    if (-1 == socket_fd) {
        return -1;
    }

    auto ecode = clp::networking::try_send(socket_fd, (char const*)&job_id, sizeof(job_id));
    if (clp::ErrorCode::ErrorCode_Success != ecode) {
        close(socket_fd);
        return -1;
    }

    char ret = 'n';
    size_t bytes_received = 0;
    ecode = clp::networking::try_receive(socket_fd, &ret, sizeof(ret), bytes_received);
    if (clp::ErrorCode::ErrorCode_Success != ecode || bytes_received != sizeof(ret) || ret != 'y') {
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

bool send_pipeline_results(int socket_fd, std::unique_ptr<RecordGroupIterator> results) {
    constexpr int buf_size = 1024;
    size_t bytes_occupied = 0;
    char buf[buf_size];

    for (; !results->done(); results->next()) {
        auto& group = results->get();
        auto serialized_result = serialize(group.get_tags(), group.record_iter());
        size_t ser_size = serialized_result.size();

        // send size header
        if (false
            == append_to_bufffer_and_send(
                    socket_fd,
                    buf,
                    bytes_occupied,
                    buf_size,
                    (char const*)&ser_size,
                    sizeof(ser_size)
            ))
        {
            return false;
        }

        // send serialized RecordGroup
        if (false
            == append_to_bufffer_and_send(
                    socket_fd,
                    buf,
                    bytes_occupied,
                    buf_size,
                    (char const*)serialized_result.data(),
                    serialized_result.size()
            ))
        {
            return false;
        }
    }

    // send any leftover bytes in the buffer
    if (bytes_occupied > 0) {
        return clp::ErrorCode::ErrorCode_Success
               == clp::networking::try_send(socket_fd, buf, bytes_occupied);
    }

    return true;
}

}  // namespace reducer
