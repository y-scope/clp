#ifndef CONTROLLERMONITORINGTHREAD_HPP
#define CONTROLLERMONITORINGTHREAD_HPP

// Project headers
#include "../Thread.hpp"

/**
 * A thread that waits for the controller to close the connection at which time
 * it will indicate the query has been cancelled.
 */
class ControllerMonitoringThread : public Thread {
public:
    // Constructor
    ControllerMonitoringThread (int controller_socket_fd) :
            m_controller_socket_fd(controller_socket_fd), m_query_cancelled(false) {}

    const std::atomic_bool& get_query_cancelled () const { return m_query_cancelled; }

protected:
    // Methods
    void thread_method () override;

private:
    // Variables
    int m_controller_socket_fd;
    std::atomic_bool m_query_cancelled;
};

#endif //CONTROLLERMONITORINGTHREAD_HPP
