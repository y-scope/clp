#include "network_utils.hpp"

#include <unistd.h>

#include <cstring>

#include "../clp/networking/socket_utils.hpp"
#include "DeserializedRecordGroup.hpp"

namespace reducer {
namespace {
/**
 * Append data to a buffer, and flush the buffer to reducer_socket_fd if the buffer becomes full.
 * @param reducer_socket_fd
 * @param buf the underlying buffer being filled
 * @param bytes_occupied the number of occupied bytes in the buffer; updated by reference
 * @param buf_size the total number of bytes that the buffer can hold
 * @param data the data being written to the buffer
 * @param size the number of bytes of data to add to the buffer
 * @return true on success
 * @return false on any error
 */
bool append_to_bufffer_and_send(
        int reducer_socket_fd,
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
        auto ecode = clp::networking::try_send(reducer_socket_fd, buf, bytes_occupied);
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
    auto reducer_socket_fd = clp::networking::connect_to_server(host, std::to_string(port));
    if (-1 == reducer_socket_fd) {
        return -1;
    }

    auto ecode = clp::networking::try_send(
            reducer_socket_fd,
            reinterpret_cast<char const*>(&job_id),
            sizeof(job_id)
    );
    if (clp::ErrorCode::ErrorCode_Success != ecode) {
        close(reducer_socket_fd);
        return -1;
    }

    char ret{0};
    size_t bytes_received = 0;
    ecode = clp::networking::try_receive(reducer_socket_fd, &ret, sizeof(ret), bytes_received);
    if (clp::ErrorCode::ErrorCode_Success != ecode || sizeof(ret) != bytes_received
        || constants::cConnectionAcceptedResponse != ret)
    {
        close(reducer_socket_fd);
        return -1;
    }

    return reducer_socket_fd;
}

bool send_pipeline_results(int reducer_socket_fd, std::unique_ptr<RecordGroupIterator> results) {
    constexpr int buf_size = 1024;
    size_t bytes_occupied = 0;
    char buf[buf_size];

    for (; false == results->done(); results->next()) {
        auto& group = results->get();
        auto serialized_result = serialize(group.get_tags(), group.record_iter());
        size_t serialized_result_size = serialized_result.size();

        // Send size
        if (false
            == append_to_bufffer_and_send(
                    reducer_socket_fd,
                    buf,
                    bytes_occupied,
                    buf_size,
                    reinterpret_cast<char const*>(&serialized_result_size),
                    sizeof(serialized_result_size)
            ))
        {
            return false;
        }

        // Send data
        if (false
            == append_to_bufffer_and_send(
                    reducer_socket_fd,
                    buf,
                    bytes_occupied,
                    buf_size,
                    reinterpret_cast<char const*>(serialized_result.data()),
                    serialized_result.size()
            ))
        {
            return false;
        }
    }

    // Send any leftover bytes in the buffer
    if (bytes_occupied > 0) {
        return clp::ErrorCode::ErrorCode_Success
               == clp::networking::try_send(reducer_socket_fd, buf, bytes_occupied);
    }

    return true;
}
}  // namespace reducer
