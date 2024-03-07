#ifndef CLP_S_SEARCH_OUTPUTHANDLER_HPP
#define CLP_S_SEARCH_OUTPUTHANDLER_HPP

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <queue>
#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "../../reducer/Pipeline.hpp"
#include "../Defs.hpp"
#include "../TraceableException.hpp"

namespace clp_s::search {
/**
 * Abstract class for handling output from the search command.
 */
class OutputHandler {
public:
    // Constructors
    explicit OutputHandler(bool should_output_timestamp, bool should_marshal_records)
            : m_should_output_timestamp(should_output_timestamp),
              m_should_marshal_records(should_marshal_records){};

    // Destructor
    virtual ~OutputHandler() = default;

    // Methods
    /**
     * Writes a log event to the output handler.
     * @param message The message in the log event.
     * @param timestamp The timestamp of the log event.
     */
    virtual void write(std::string const& message, epochtime_t timestamp) = 0;

    /**
     * Writes a message to the output handler.
     * @param message The message to write.
     */
    virtual void write(std::string const& message) = 0;

    /**
     * Flushes the output handler after each table that gets searched.
     */
    virtual void flush() = 0;

    /**
     * Performs any final operations after all tables have been searched.
     */
    virtual void finish() {}

    [[nodiscard]] bool should_output_timestamp() const { return m_should_output_timestamp; }

    [[nodiscard]] bool should_marshal_records() const { return m_should_marshal_records; }

protected:
    bool m_should_output_timestamp;
    bool m_should_marshal_records;
};

/**
 * Output handler that writes to standard output.
 */
class StandardOutputHandler : public OutputHandler {
public:
    // Constructors
    explicit StandardOutputHandler(bool should_output_timestamp = false)
            : OutputHandler(should_output_timestamp, true) {}

    // Methods inherited from OutputHandler
    void write(std::string const& message, epochtime_t timestamp) override {
        printf("%" EPOCHTIME_T_PRINTF_FMT " %s", timestamp, message.c_str());
    }

    void write(std::string const& message) override { printf("%s", message.c_str()); }

    void flush() override {}
};

/**
 * Output handler that writes to a network destination.
 */
class NetworkOutputHandler : public OutputHandler {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    explicit NetworkOutputHandler(
            std::string const& host,
            std::string const& port,
            bool should_output_timestamp = false
    );

    // Destructor
    ~NetworkOutputHandler() override {
        if (-1 != m_socket_fd) {
            close(m_socket_fd);
        }
    }

    // Methods inherited from OutputHandler
    void write(std::string const& message, epochtime_t timestamp) override {
        std::string message_with_timestamp(std::to_string(timestamp) + " " + message);
        if (-1
            == send(m_socket_fd, message_with_timestamp.c_str(), message_with_timestamp.size(), 0))
        {
            throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILE__, __LINE__);
        }
    }

    void write(std::string const& message) override {
        if (-1 == send(m_socket_fd, message.c_str(), message.size(), 0)) {
            throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILE__, __LINE__);
        }
    }

    void flush() override{};

private:
    std::string m_host;
    std::string m_port;
    int m_socket_fd;
};

/**
 * Output handler that writes to a MongoDB collection.
 */
class ResultsCacheOutputHandler : public OutputHandler {
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
    };

    // Constructor
    ResultsCacheOutputHandler(
            std::string const& uri,
            std::string const& collection,
            uint64_t batch_size,
            uint64_t max_num_results,
            bool should_output_timestamp = true
    );

    // Methods inherited from OutputHandler
    void flush() override;

    void write(std::string const& message, epochtime_t timestamp) override;

    void write(std::string const& message) override { write(message, 0); }

private:
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::vector<bsoncxx::document::value> m_results;
    uint64_t m_batch_size;
    uint64_t m_max_num_results;
    std::priority_queue<
            std::unique_ptr<QueryResult>,
            std::vector<std::unique_ptr<QueryResult>>,
            QueryResultGreaterTimestampComparator>
            m_latest_results;
};

/**
 * Output handler that performs a count aggregation and sends the results to a reducer.
 */
class CountOutputHandler : public OutputHandler {
public:
    // Constructor
    CountOutputHandler(int reducer_socket_fd);

    // Methods inherited from OutputHandler
    void flush() override {}

    void write(std::string const& message, epochtime_t timestamp) override {}

    void write(std::string const& message) override;

    void finish() override;

private:
    int m_reducer_socket_fd;
    reducer::Pipeline m_pipeline;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_OUTPUTHANDLER_HPP
