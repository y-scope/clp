#ifndef CLP_CLO_RESULTSCACHECLIENT_HPP
#define CLP_CLO_RESULTSCACHECLIENT_HPP

#include <unistd.h>

#include <queue>
#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/uri.hpp>

#include "../Defs.h"
#include "../TraceableException.hpp"

namespace clp::clo {
class Client {
public:
    virtual ~Client() = default;
    /**
     * Adds a query result to a batch or sends it to the destination.
     * @param original_path The original path of the log event.
     * @param message The content of the log event.
     * @param timestamp The timestamp of the log event.
     * @return ErrorCode_Success if the result was added successfully, an error code otherwise.
     */
    virtual ErrorCode
    add_result(std::string const& original_path, std::string const& message, epochtime_t timestamp)
            = 0;

    /**
     * Flushes the results.
     */
    virtual void flush() = 0;
};

class NetworkClient : public Client {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "NetworkClient operation failed"; }
    };

    // Constructors
    NetworkClient(std::string const& host, std::string const& port);

    // Methods inherited from Client
    ErrorCode add_result(
            std::string const& original_path,
            std::string const& message,
            epochtime_t timestamp
    ) override;

    void flush() override { close(m_socket_fd); }

private:
    int m_socket_fd;
};

/**
 * Class encapsulating a MongoDB client used to send query results to the results cache.
 */
class ResultsCacheClient : public Client {
public:
    // Types
    struct QueryResult {
        // Constructors
        QueryResult(std::string original_path, std::string message, epochtime_t timestamp)
                : original_path(std::move(original_path)),
                  message(std::move(message)),
                  timestamp(timestamp) {}

        std::string original_path;
        std::string message;
        epochtime_t timestamp;
    };

    struct QueryResultGreaterTimestampComparator {
        bool operator()(
                std::unique_ptr<QueryResult> const& r1,
                std::unique_ptr<QueryResult> const& r2
        ) const {
            return r1->timestamp > r2->timestamp;
        }
    };

    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "ResultsCacheClient operation failed"; }
    };

    // Constructors
    ResultsCacheClient(
            std::string const& uri,
            std::string const& collection,
            uint64_t batch_size,
            uint64_t max_num_results
    );

    // Methods
    /**
     * @return The earliest (smallest) timestamp in the heap of latest results
     */
    [[nodiscard]] epochtime_t get_smallest_timestamp() const {
        return m_latest_results.empty() ? cEpochTimeMin : m_latest_results.top()->timestamp;
    }

    [[nodiscard]] uint64_t get_max_num_results() const { return m_max_num_results; }

    /**
     * @return Whether the heap of latest results is full.
     */
    [[nodiscard]] bool is_latest_results_full() const {
        return m_latest_results.size() >= m_max_num_results;
    }

    // Methods inherited from Client
    ErrorCode add_result(
            std::string const& original_path,
            std::string const& message,
            epochtime_t timestamp
    ) override;

    void flush() override;

private:
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::vector<bsoncxx::document::value> m_results;
    uint64_t m_batch_size;
    uint64_t m_max_num_results;
    // The search results with the latest timestamps
    std::priority_queue<
            std::unique_ptr<QueryResult>,
            std::vector<std::unique_ptr<QueryResult>>,
            QueryResultGreaterTimestampComparator>
            m_latest_results;
};
}  // namespace clp::clo

#endif  // CLP_CLO_RESULTSCACHECLIENT_HPP
