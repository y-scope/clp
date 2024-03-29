#ifndef REDUCER_NETWORK_UTILS_HPP
#define REDUCER_NETWORK_UTILS_HPP

#include <memory>
#include <string>

#include "RecordGroupIterator.hpp"
#include "types.hpp"

namespace reducer {
/**
 * Tries to connect to the reducer and negotiate a connection for the given job ID.
 * @param host
 * @param port
 * @param job_id
 * @return Socket file descriptor for the reducer on success
 * @return -1 on any error
 */
int connect_to_reducer(std::string const& host, int port, job_id_t job_id);

/**
 * Sends results to the reducer.
 * @param reducer_socket_fd
 * @param results
 * @return Whether the results were sent successfully.
 */
bool send_pipeline_results(int reducer_socket_fd, std::unique_ptr<RecordGroupIterator> results);
}  // namespace reducer

#endif  // REDUCER_NETWORK_UTILS_HPP
