#ifndef REDUCER_REDUCER_NETWORK_UTILS_HPP
#define REDUCER_REDUCER_NETWORK_UTILS_HPP

#include <memory>
#include <string>

#include "RecordGroupIterator.hpp"

namespace reducer {
/**
 * Try to connect to a reducer listening at a given host and port, and negotiate a connection using
 * the job ID for the aggregation being performed.
 * @param host
 * @param port
 * @param job_id
 * @return socket fd for the reducer on success
 * @return -1 on any error
 */
int connect_to_reducer(std::string const& host, int port, int64_t job_id);

/**
 * Send a single record group of results to the reducer connected to with socket_fd
 * @param socket_fd
 * @param results
 * @return true on success
 * @return false on any error
 */
bool send_pipeline_results(int socket_fd, std::unique_ptr<RecordGroupIterator> results);
}  // namespace reducer

#endif  // REDUCER_REDUCER_NETWORK_UTILS_HPP
