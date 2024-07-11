#ifndef CLP_CLO_OUTPUTHANDLER_HPP
#define CLP_CLO_OUTPUTHANDLER_HPP

#include <unistd.h>

#include <queue>
#include <string>
#include <string_view>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/uri.hpp>

#include "../../reducer/Pipeline.hpp"
#include "../Defs.h"
#include "../streaming_archive/MetadataDB.hpp"
#include "../streaming_archive/reader/Message.hpp"
#include "../TraceableException.hpp"

namespace clp::clo {
/**
 * Abstract class for handling output from a search.
 */
class OutputHandler {
public:
    // Destructor
    virtual ~OutputHandler() = default;

    // Methods
    /**
     * Adds a query result to a batch or sends it to the destination.
     * @param orig_file_path Path of the original file that contains the result.
     * @param orig_file_id ID of the original file that contains the result.
     * @param encoded_message The encoded result.
     * @param decompressed_message The decompressed result.
     * @return ErrorCode_Success if the result was added successfully, or an error code if specified
     * by the derived class.
     */
    virtual ErrorCode add_result(
            std::string_view orig_file_path,
            std::string_view orig_file_id,
            streaming_archive::reader::Message const& encoded_message,
            std::string_view decompressed_message
    ) = 0;

    /**
     * Flushes any buffered output. Called once at the end of search.
     * @return ErrorCode_Success on success or relevant error code on error
     */
    virtual ErrorCode flush() = 0;

    /**
     * @param it
     * @return Whether a file can be skipped based on the current state of the output handler, and
     * metadata about the file
     */
    [[nodiscard]] virtual bool can_skip_file(
            [[maybe_unused]] ::clp::streaming_archive::MetadataDB::FileIterator const& it
    ) {
        return false;
    }
};

/**
 * Class encapsulating a network client used to send query results to a network destination.
 */
class NetworkOutputHandler : public OutputHandler {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "NetworkOutputHandler operation failed";
        }
    };

    // Constructors
    NetworkOutputHandler(std::string const& host, int port);

    // Destructor
    ~NetworkOutputHandler() override { close(m_socket_fd); }

    // Methods inherited from Client
    /**
     * Sends a result to the network destination.
     * @param orig_file_path Path of the original file that contains the result.
     * @param orig_file_id ID of the original file that contains the result.
     * @param encoded_message The encoded result.
     * @param decompressed_message The decompressed result.
     * @return Same as networking::try_send
     */
    ErrorCode add_result(
            std::string_view orig_file_path,
            std::string_view orig_file_id,
            streaming_archive::reader::Message const& encoded_message,
            std::string_view decompressed_message
    ) override;

    /**
     * Closes the connection.
     * @return ErrorCode_Success
     */
    ErrorCode flush() override {
        close(m_socket_fd);
        return ErrorCode::ErrorCode_Success;
    }

private:
    int m_socket_fd;
};

/**
 * Class encapsulating a MongoDB client used to send query results to the results cache.
 */
class ResultsCacheOutputHandler : public OutputHandler {
public:
    // Types
    struct QueryResult {
        // Constructors
        QueryResult(
                std::string_view orig_file_path,
                std::string_view orig_file_id,
                size_t log_event_ix,
                epochtime_t timestamp,
                std::string_view decompressed_message
        )
                : orig_file_path(orig_file_path),
                  orig_file_id(orig_file_id),
                  log_event_ix(log_event_ix),
                  timestamp(timestamp),
                  decompressed_message(decompressed_message) {}

        std::string orig_file_path;
        std::string orig_file_id;
        int64_t log_event_ix;
        epochtime_t timestamp;
        std::string decompressed_message;
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
        char const* what() const noexcept override {
            return "ResultsCacheOutputHandler operation failed";
        }
    };

    // Constructors
    ResultsCacheOutputHandler(
            std::string const& uri,
            std::string const& collection,
            uint64_t batch_size,
            uint64_t max_num_results
    );

    // Methods inherited from OutputHandler
    ErrorCode add_result(
            std::string_view orig_file_path,
            std::string_view orig_file_id,
            streaming_archive::reader::Message const& encoded_message,
            std::string_view decompressed_message
    ) override;

    /**
     * Flushes any buffered output.
     * @return ErrorCode_Success on success
     * @return ErrorCode_Failure_DB_Bulk_Write on failure to write results to the results cache
     */
    ErrorCode flush() override;

    [[nodiscard]] bool can_skip_file(::clp::streaming_archive::MetadataDB::FileIterator const& it
    ) override {
        return is_latest_results_full() && get_smallest_timestamp() > it.get_end_ts();
    }

private:
    /**
     * @return The earliest (smallest) timestamp in the heap of latest results
     */
    [[nodiscard]] epochtime_t get_smallest_timestamp() const {
        return m_latest_results.empty() ? cEpochTimeMin : m_latest_results.top()->timestamp;
    }

    /**
     * @return Whether the heap of latest results is full.
     */
    [[nodiscard]] bool is_latest_results_full() const {
        return m_latest_results.size() >= m_max_num_results;
    }

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

/**
 * Class encapsulating a reducer client used to send count aggregation results to the reducer.
 */
class CountOutputHandler : public OutputHandler {
public:
    // Constructor
    explicit CountOutputHandler(int reducer_socket_fd);

    // Methods inherited from OutputHandler
    ErrorCode add_result(
            std::string_view orig_file_path,
            std::string_view orig_file_id,
            streaming_archive::reader::Message const& encoded_message,
            std::string_view decompressed_message
    ) override;

    /**
     * Flushes the count.
     * @return ErrorCode_Success on success
     * @return ErrorCode_Failure_Network on network error
     */
    ErrorCode flush() override;

private:
    int m_reducer_socket_fd;
    reducer::Pipeline m_pipeline;
};

/**
 * Output handler that performs a count aggregation bucketed by time and sends the results to a
 * reducer.
 */
class CountByTimeOutputHandler : public OutputHandler {
public:
    // Constructors
    CountByTimeOutputHandler(int reducer_socket_fd, int64_t count_by_time_bucket_size)
            : m_reducer_socket_fd{reducer_socket_fd},
              m_count_by_time_bucket_size{count_by_time_bucket_size} {}

    // Methods inherited from OutputHandler
    ErrorCode add_result(
            std::string_view orig_file_path,
            std::string_view orig_file_id,
            streaming_archive::reader::Message const& encoded_message,
            std::string_view decompressed_message
    ) override {
        int64_t bucket = (encoded_message.get_ts_in_milli() / m_count_by_time_bucket_size)
                         * m_count_by_time_bucket_size;
        m_bucket_counts[bucket] += 1;
        return ErrorCode::ErrorCode_Success;
    }

    /**
     * Flushes the counts.
     * @return ErrorCode_Success on success
     * @return ErrorCode_Failure_Network on network error
     */
    ErrorCode flush() override;

private:
    int m_reducer_socket_fd;
    std::map<int64_t, int64_t> m_bucket_counts;
    int64_t m_count_by_time_bucket_size;
};
}  // namespace clp::clo

#endif  // CLP_CLO_OUTPUTHANDLER_HPP
