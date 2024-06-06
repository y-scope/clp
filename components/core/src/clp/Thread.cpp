#include "Thread.hpp"

#include "Defs.h"
#include "spdlog_with_specializations.hpp"

using std::system_error;

namespace clp {
Thread::~Thread() {
    if (m_thread_running) {
        SPDLOG_WARN("Thread did not exit before being destroyed.");
    }
    if (nullptr != m_thread && m_thread->joinable()) {
        // NOTE: There are two reasons to join rather than detach.
        // (1) Since the std::thread doesn't take ownership of this object during creation, then
        //     it's possible that this object goes out of scope while the thread is still running.
        // (2) Similarly, derived classes may use references to objects that are not owned by the
        //     std::thread.
        m_thread->join();
    }
}

void Thread::start() {
    try {
        m_thread_running = true;
        m_thread = std::make_unique<std::thread>(&Thread::thread_entry_point, this);
    } catch (system_error& e) {
        m_thread_running = false;
        SPDLOG_ERROR("Failed to start thread - {}", e.what());
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

void Thread::join() {
    if (nullptr == m_thread) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    try {
        m_thread->join();
    } catch (system_error& e) {
        SPDLOG_ERROR("Failed to join thread - {}", e.what());
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
}

void Thread::thread_entry_point() {
    thread_method();
    m_thread_running = false;
}
}  // namespace clp
