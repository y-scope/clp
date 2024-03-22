#include "network_utils.hpp"

#include <unistd.h>

#include <cstddef>
#include <memory>
#include <string>

#include "../clp/ErrorCode.hpp"
#include "../clp/networking/socket_utils.hpp"
#include "BufferedSocketWriter.hpp"
#include "DeserializedRecordGroup.hpp"
#include "RecordGroupIterator.hpp"
#include "types.hpp"

namespace reducer {
int connect_to_reducer(std::string const& host, int port, job_id_t job_id) {
    constexpr char cConnectionAcceptedResponse = 'y';
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
    size_t bytes_received{0};
    ecode = clp::networking::try_receive(reducer_socket_fd, &ret, sizeof(ret), bytes_received);
    if (clp::ErrorCode::ErrorCode_Success != ecode || sizeof(ret) != bytes_received
        || cConnectionAcceptedResponse != ret)
    {
        close(reducer_socket_fd);
        return -1;
    }

    return reducer_socket_fd;
}

bool send_pipeline_results(int reducer_socket_fd, std::unique_ptr<RecordGroupIterator> results) {
    constexpr int cBufSize = 1024;
    BufferedSocketWriter buffered_writer{reducer_socket_fd, cBufSize};

    for (; false == results->done(); results->next()) {
        auto& group = results->get();
        auto serialized_result = serialize(group.get_tags(), group.record_iter());
        auto serialized_result_size = serialized_result.size();

        // Send size
        if (false
            == buffered_writer.write(
                    reinterpret_cast<char const*>(&serialized_result_size),
                    sizeof(serialized_result_size)
            ))
        {
            return false;
        }

        // Send data
        if (false == buffered_writer.write(serialized_result)) {
            return false;
        }
    }

    // Send any leftover bytes in the buffer
    return buffered_writer.flush();
}
}  // namespace reducer
