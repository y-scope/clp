#ifndef PTHREAD_HPP
#define PTHREAD_HPP

// C standard libraries
#include <pthread.h>

// C++ standard libraries
#include <atomic>

// Project headers
#include "ErrorCode.hpp"
#include "TraceableException.hpp"

/**
 * C++ wrapper for POSIX threads
 */
class PThread {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
                TraceableException(error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "PThread operation failed";
        }
    };

    // Constructors
    PThread () : m_thread(0), m_thread_running(false) {};

    // Destructor
    virtual ~PThread ();

    // Methods
    /**
     * Starts the thread
     */
    void start ();
    /**
     * Detaches the thread so its resources are released without being joined
     */
    void detach ();
    /**
     * Tries to join with the thread
     * @param thread_return_value The thread's return value
     * @return ErrorCode_errno on failure
     * @return ErrorCode_Success otherwise
     */
    ErrorCode try_join (void*& thread_return_value);
    /**
     * Joins with the thread
     * @return The thread's return value
     */
    void* join ();

    bool is_running () const { return m_thread_running; }

protected:
    // Methods
    virtual void* thread_method () = 0;
    /**
     * Indicates that the thread has exited
     */
    void mark_as_exited () { m_thread_running = false; }

private:
    // Methods
    /**
     * Entry-point method for the thread
     * @param arg
     * @return Same as PThread::thread_method
     */
    static void* thread_entry_point (void* arg);

    // Variables
    pthread_t m_thread;
    std::atomic_bool m_thread_running;
};

#endif //PTHREAD_HPP
