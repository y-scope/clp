#ifndef CLP_S_OUTPUTHANDLERIMPL_HPP
#define CLP_S_OUTPUTHANDLERIMPL_HPP

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/options/insert.hpp>
#include <msgpack.hpp>

#include <clp_s/AggregationSink.hpp>
#include <clp_s/aggregators.hpp>
#include <clp_s/CommandLineArguments.hpp>
#include <clp_s/ErrorCode.hpp>

#include "../reducer/Pipeline.hpp"
#include "../reducer/RecordGroupIterator.hpp"
#include "Defs.hpp"
#include "FileWriter.hpp"
#include "search/OutputHandler.hpp"

namespace clp_s {
/**
 * Output handler that writes to standard output.
 */
class StandardOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Constructors
    explicit StandardOutputHandler(bool should_output_metadata = false)
            : ::clp_s::search::OutputHandler(should_output_metadata, true) {}

    // Methods inherited from OutputHandler
    [[nodiscard]] auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> ErrorCode override {
        std::cout << archive_id << ": " << log_event_idx << ": " << timestamp << " " << message;
        return ErrorCode::ErrorCodeSuccess;
    }

    [[nodiscard]] auto write(std::string_view message) -> ErrorCode override {
        std::cout << message;
        return ErrorCode::ErrorCodeSuccess;
    }
};

/**
 * Output handler that writes to a file.
 */
class FileOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Constructors
    explicit FileOutputHandler(std::string const& path, bool should_output_metadata = false)
            : ::clp_s::search::OutputHandler(should_output_metadata, true),
              m_file_writer() {
        m_file_writer.open(path, FileWriter::OpenMode::CreateForWriting);
    }

    ~FileOutputHandler() { m_file_writer.close(); }

    // Methods inherited from OutputHandler
    [[nodiscard]] auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> ErrorCode override;

    [[nodiscard]] auto write(std::string_view message) -> ErrorCode override {
        return write(message, 0, {}, 0);
    }

private:
    FileWriter m_file_writer;
};

/**
 * Output handler that streams the results of searching a single archive to a network destination.
 *
 * The handler connects to the destination when the first result is written, so a search without
 * results never opens a connection. Each message is a msgpack array:
 * - the handshake, sent once before the first result:
 *   `[protocol_version, session_token, task_index, archive_id]`;
 * - one result message per result, `[result_index, timestamp, message]`, where `result_index`
 *   counts the results from 0 and `timestamp` is in epoch milliseconds;
 * - the trailer, sent once after the last result: `[num_results]`.
 *
 * After sending the trailer, the handler shuts down the write side of the connection and waits for
 * the destination's acknowledgement, `[num_results]` encoded as `0x91 0xcf` followed by the count
 * as a big-endian u64.
 */
class NetworkOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Types
    /**
     * Function that opens a connection to the network destination.
     * @return The connected socket's file descriptor on success, or -1 on failure.
     */
    using Connector = std::function<int()>;

    // Constants
    static constexpr uint8_t cProtocolVersion{1};

    // Constructors
    /**
     * @param host
     * @param port
     * @param session_token Token identifying the destination's session.
     * @param task_idx Index of the search task within its query job.
     */
    NetworkOutputHandler(
            std::string const& host,
            int port,
            std::string session_token,
            uint64_t task_idx
    );

    /**
     * @param connector Function to open the connection with, when the first result is written.
     * @param destination Name of the network destination, for log messages.
     * @param session_token Token identifying the destination's session.
     * @param task_idx Index of the search task within its query job.
     */
    NetworkOutputHandler(
            Connector connector,
            std::string destination,
            std::string session_token,
            uint64_t task_idx
    )
            : ::clp_s::search::OutputHandler{true, true},
              m_connector{std::move(connector)},
              m_destination{std::move(destination)},
              m_session_token{std::move(session_token)},
              m_task_idx{task_idx} {}

    // Destructor
    ~NetworkOutputHandler() override {
        if (-1 != m_socket_fd) {
            close(m_socket_fd);
        }
    }

    // Delete copy & move constructors and assignment operators
    NetworkOutputHandler(NetworkOutputHandler const&) = delete;
    NetworkOutputHandler(NetworkOutputHandler&&) = delete;
    auto operator=(NetworkOutputHandler const&) -> NetworkOutputHandler& = delete;
    auto operator=(NetworkOutputHandler&&) -> NetworkOutputHandler& = delete;

    // Methods inherited from OutputHandler
    /**
     * Sends a result to the network destination, connecting to it and sending the handshake first
     * if this is the first result.
     * @param message
     * @param timestamp
     * @param archive_id
     * @param log_event_idx Unused.
     * @return ErrorCodeSuccess on success.
     * @return ErrorCodeFailureNetwork if connecting or sending to the network destination fails.
     */
    [[nodiscard]] auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> ErrorCode override;

    /**
     * Unsupported, since every result must carry its timestamp and archive ID.
     * @param message
     * @return ErrorCodeUnsupported unconditionally.
     */
    [[nodiscard]] auto write(std::string_view message) -> ErrorCode override;

    // Methods overriding OutputHandler
    /**
     * Sends the trailer, shuts down the write side of the connection, and waits for the network
     * destination to acknowledge the results. Does nothing if no result was written.
     * @return ErrorCodeSuccess on success.
     * @return ErrorCodeFailureNetwork if sending the trailer, shutting down the connection, or
     * receiving the acknowledgement fails, or if the acknowledgement is malformed or doesn't count
     * the number of results sent.
     */
    [[nodiscard]] auto finish() -> ErrorCode override;

private:
    // Methods
    /**
     * Sends the serialized messages in the buffer to the network destination, and then clears the
     * buffer.
     * @return ErrorCodeSuccess on success.
     * @return ErrorCodeFailureNetwork if sending fails.
     */
    [[nodiscard]] auto send_buffer() -> ErrorCode;

    /**
     * Receives the network destination's acknowledgement and validates it against the number of
     * results sent.
     * @return ErrorCodeSuccess on success.
     * @return ErrorCodeFailureNetwork if receiving fails, the connection is closed before the whole
     * acknowledgement is received, or the acknowledgement is malformed or doesn't count the number
     * of results sent.
     */
    [[nodiscard]] auto receive_ack() const -> ErrorCode;

    // Variables
    Connector m_connector;
    std::string m_destination;
    std::string m_session_token;
    uint64_t m_task_idx{};
    uint64_t m_num_results_sent{};
    int m_socket_fd{-1};
    msgpack::sbuffer m_buffer;
};

/**
 * Output handler that writes to a MongoDB collection.
 */
class ResultsCacheOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Types
    struct QueryResult {
        // Constructors
        QueryResult(
                std::string_view original_path,
                std::string_view message,
                epochtime_t timestamp,
                std::string_view archive_id,
                int64_t log_event_idx,
                std::string_view dataset
        )
                : original_path{original_path},
                  message{message},
                  timestamp{timestamp},
                  archive_id{archive_id},
                  log_event_idx{log_event_idx},
                  dataset{dataset} {}

        std::string original_path;
        std::string message;
        epochtime_t timestamp;
        std::string archive_id;
        int64_t log_event_idx;
        std::string dataset;
    };

    struct QueryResultGreaterTimestampComparator {
        bool operator()(
                std::unique_ptr<QueryResult> const& r1,
                std::unique_ptr<QueryResult> const& r2
        ) const {
            return r1->timestamp > r2->timestamp;
        }
    };

    // Constructor
    ResultsCacheOutputHandler(
            std::string_view uri,
            std::string_view collection,
            uint64_t batch_size,
            std::optional<uint64_t> max_num_results,
            std::string_view dataset,
            bool should_output_metadata = true
    );

    // Methods inherited from OutputHandler
    /**
     * Flushes the output handler after all tables are searched.
     * @return ErrorCodeSuccess on success.
     * @return ErrorCodeFailureDbBulkWrite on failure to write results to the results cache.
     */
    auto finish() -> ErrorCode override;

    [[nodiscard]] auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> ErrorCode override;

    [[nodiscard]] auto write(std::string_view message) -> ErrorCode override {
        return write(message, 0, {}, 0);
    }

private:
    /**
     * Converts a result to BSON and appends it to `m_results`. Flushes MongoDB batch when it
     * reaches `m_batch_size` documents.
     *
     * @return ErrorCodeSuccess on success.
     * @return ErrorCodeFailureDbBulkWrite if document construction or batch insertion fails.
     */
    [[nodiscard]] auto append_result(QueryResult result) -> ErrorCode;

    /**
     * Inserts the pending results as an unordered batch. Duplicate-key errors are treated as
     * success so that retries converge on the complete result set.
     * @return Whether insertion succeeded or produced only duplicate-key errors.
     */
    [[nodiscard]] auto insert_results() -> bool;

    mongocxx::client m_client;
    mongocxx::collection m_collection;
    mongocxx::options::insert m_insert_options;
    std::vector<bsoncxx::document::value> m_results;
    uint64_t m_batch_size;
    std::optional<uint64_t> m_max_num_results;
    std::string m_dataset;
    std::priority_queue<
            std::unique_ptr<QueryResult>,
            std::vector<std::unique_ptr<QueryResult>>,
            QueryResultGreaterTimestampComparator
    >
            m_latest_results;
};

/**
 * Output handler that performs a count aggregation and sends the results to a reducer.
 */
class CountReducerOutputHandler : public search::OutputHandler {
public:
    // Constructors
    CountReducerOutputHandler(int reducer_socket_fd);

    // Methods implementing OutputHandler
    [[nodiscard]] auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> ErrorCode override {
        return ErrorCodeSuccess;
    }

    [[nodiscard]] auto write(std::string_view message) -> ErrorCode override;

    // Methods overriding OutputHandler
    /**
     * Flushes the count.
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeFailureNetwork on network error
     */
    auto finish() -> ErrorCode override;

private:
    // Data members
    int m_reducer_socket_fd;
    reducer::Pipeline m_pipeline;
};

/**
 * Output handler that performs a count aggregation bucketed by time and sends the results to a
 * reducer.
 */
class CountByTimeReducerOutputHandler : public search::OutputHandler {
public:
    // Constructors
    CountByTimeReducerOutputHandler(
            int reducer_socket_fd,
            int64_t count_by_time_bucket_size_millisecs
    )
            : search::OutputHandler{true, false},
              m_reducer_socket_fd{reducer_socket_fd},
              m_count_by_time_bucket_size_millisecs{count_by_time_bucket_size_millisecs} {}

    // Methods implementing OutputHandler
    [[nodiscard]] auto write(
            std::string_view message,
            epochtime_t timestamp_millisecs,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> ErrorCode override {
        int64_t bucket = (timestamp_millisecs / m_count_by_time_bucket_size_millisecs)
                         * m_count_by_time_bucket_size_millisecs;
        m_bucket_counts[bucket] += 1;
        return ErrorCodeSuccess;
    }

    [[nodiscard]] auto write(std::string_view message) -> ErrorCode override {
        return ErrorCodeSuccess;
    }

    // Methods overriding OutputHandler
    /**
     * Flushes the counts.
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeFailureNetwork on network error
     */
    auto finish() -> ErrorCode override;

private:
    // Data members
    int m_reducer_socket_fd;
    std::map<int64_t, int64_t> m_bucket_counts;
    int64_t m_count_by_time_bucket_size_millisecs;
};

/**
 * Output handler that runs an `Aggregation` and writes its results to an `AggregationSink`.
 * @tparam AggT The type of aggregator to run.
 */
template <AggregatorReq AggT>
class AggregationOutputHandler : public search::OutputHandler {
public:
    // Constructors
    AggregationOutputHandler(AggT aggregator, std::unique_ptr<AggregationSink> sink)
            : search::OutputHandler{AggT::cNeedsMetadata, AggT::cNeedsMarshalledRecord},
              m_aggregator{std::move(aggregator)},
              m_sink{std::move(sink)} {}

    // Methods implementing OutputHandler
    [[nodiscard]] auto write(
            std::string_view message,
            epochtime_t timestamp_millisecs,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> ErrorCode override {
        m_aggregator.add_record(message, timestamp_millisecs);
        return ErrorCodeSuccess;
    }

    [[nodiscard]] auto write(std::string_view message) -> ErrorCode override {
        m_aggregator.add_record(message, 0);
        return ErrorCodeSuccess;
    }

    // Methods overriding OutputHandler
    /**
     * Drains the aggregation's results into the sink.
     * @return ErrorCodeSuccess on success
     * @return The sink's error code on failure
     */
    auto finish() -> ErrorCode override {
        for (auto const& result : m_aggregator.get_results()) {
            if (auto err{m_sink->write(result)}; err.has_error()) {
                return ErrorCode::ErrorCodeFailure;
            }
        }
        if (auto err{m_sink->finish()}; err.has_error()) {
            return ErrorCode::ErrorCodeFailure;
        }
        return ErrorCode::ErrorCodeSuccess;
    }

private:
    // Data members
    AggT m_aggregator;
    std::unique_ptr<AggregationSink> m_sink;
};

/**
 * Creates an output handler that runs the given aggregation.
 * @param aggregator The aggregation to run.
 * @param sink The destination for the aggregation's results.
 * @return The constructed output handler.
 */
[[nodiscard]] inline auto
make_aggregation_output_handler(Aggregator aggregator, std::unique_ptr<AggregationSink> sink)
        -> std::unique_ptr<search::OutputHandler> {
    return std::visit(
            [&](auto&& agg) -> std::unique_ptr<search::OutputHandler> {
                using AggT = std::decay_t<decltype(agg)>;
                return std::make_unique<AggregationOutputHandler<AggT>>(
                        std::move(agg),
                        std::move(sink)
                );
            },
            std::move(aggregator)
    );
}

/**
 * Output handler that records all results in a provided vector.
 */
class VectorOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Types
    struct QueryResult {
        // Constructors
        QueryResult(
                std::string_view message,
                epochtime_t timestamp,
                std::string_view archive_id,
                int64_t log_event_idx
        )
                : message{message},
                  timestamp{timestamp},
                  archive_id{archive_id},
                  log_event_idx{log_event_idx} {}

        std::string message;
        epochtime_t timestamp;
        std::string archive_id;
        int64_t log_event_idx;
    };

    // Constructors
    VectorOutputHandler(std::vector<QueryResult>& output)
            : search::OutputHandler{true, true},
              m_output(output) {}

    // Methods inherited from OutputHandler
    [[nodiscard]] auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> ErrorCode override {
        m_output.emplace_back(message, timestamp, archive_id, log_event_idx);
        return ErrorCode::ErrorCodeSuccess;
    }

    [[nodiscard]] auto write(std::string_view message) -> ErrorCode override {
        m_output.emplace_back(message, epochtime_t{}, std::string_view{}, int64_t{});
        return ErrorCode::ErrorCodeSuccess;
    }

private:
    std::vector<QueryResult>& m_output;
};
}  // namespace clp_s

#endif  // CLP_S_OUTPUTHANDLERIMPL_HPP
