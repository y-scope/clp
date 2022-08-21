#ifndef THREAD_HPP
#define THREAD_HPP

// C++ standard libraries
#include <atomic>
#include <memory>
#include <thread>

// Project headers
#include "ErrorCode.hpp"
#include "TraceableException.hpp"

/**
 * Wrapper for C++ threads that has some extra features and provides a more
 * encapsulated way to define a thread. Note that detachment is explicitly not
 * supported since that means this object could go out of scope while the
 * std::thread is still running.
 */
class Thread {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
                TraceableException(error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "Thread operation failed";
        }
    };

    // Constructors
    Thread () : m_thread_running(false) {};

    // Destructor
    virtual ~Thread ();

    // Methods
    /**
     * Starts the thread
     */
    void start ();
    /**
     * Joins with the thread
     */
    void join ();

    bool is_running () const { return m_thread_running; }

protected:
    // Methods
    virtual void thread_method () = 0;

private:
    // Methods
    /**
     * Entry-point method for the thread
     */
    void thread_entry_point ();

    // Variables
    std::unique_ptr<std::thread> m_thread;
    std::atomic_bool m_thread_running;
};

#endif //THREAD_HPP
