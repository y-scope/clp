#include "PThread.hpp"

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Defs.h"

PThread::~PThread () {
    if (m_thread_running) {
        SPDLOG_WARN("PThread did not exit before being destroyed.");
        int returned_value = pthread_cancel(m_thread);
        if (0 != returned_value) {
            SPDLOG_ERROR("Failed to cancel thread, errno={}", returned_value);
        }
    }
}

void PThread::start () {
    int returned_value = pthread_create(&m_thread, nullptr, thread_entry_point, this);
    if (0 != returned_value) {
        SPDLOG_ERROR("Failed to start thread, errno={}", returned_value);
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }
    m_thread_running = true;
}

void PThread::detach () {
    int returned_value = pthread_detach(m_thread);
    if (0 != returned_value) {
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }
}

ErrorCode PThread::try_join (void*& thread_return_value) {
    auto return_value = pthread_join(m_thread, &thread_return_value);
    if (0 != return_value) {
        return ErrorCode_errno;
    } else {
        return ErrorCode_Success;
    }
}

void* PThread::join () {
    void* thread_return_value;
    auto error_code = try_join(thread_return_value);
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
    return thread_return_value;
}

void* PThread::thread_entry_point (void* arg) {
    auto thread = reinterpret_cast<PThread*>(arg);
    auto return_value = thread->thread_method();
    thread->mark_as_exited();
    return return_value;
}
