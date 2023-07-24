#include "ControllerMonitoringThread.hpp"

// C standard libraries
#include <unistd.h>

// Project headers
#include "../networking/socket_utils.hpp"
#include "../spdlog_with_specializations.hpp"

void ControllerMonitoringThread::thread_method () {
    // Wait for the controller socket to close
    constexpr size_t cBufLen = 4096;
    char buf[cBufLen];
    size_t num_bytes_received;
    for (bool exit = false; false == exit;) {
        auto error_code = networking::try_receive(m_controller_socket_fd, buf, cBufLen, num_bytes_received);
        switch (error_code) {
            case ErrorCode_EndOfFile:
                // Controller closed the connection
                m_query_cancelled = true;
                exit = true;
                break;
            case ErrorCode_Success:
                // Unexpectedly received data
                SPDLOG_ERROR("Unexpected received {} bytes of data from controller.", num_bytes_received);
                break;
            case ErrorCode_BadParam:
                SPDLOG_ERROR("Bad parameter sent to try_receive.", num_bytes_received);
                exit = true;
                break;
            case ErrorCode_errno:
                SPDLOG_ERROR("Failed to receive data from controller, errno={}.", errno);
                exit = true;
                break;
            default:
                SPDLOG_ERROR("Unexpected error from try_receive, error_code={}.", error_code);
                exit = true;
                break;
        }
    }

    close(m_controller_socket_fd);
}
