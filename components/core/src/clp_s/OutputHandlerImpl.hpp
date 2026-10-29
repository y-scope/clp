#ifndef CLP_S_OUTPUTHANDLERIMPL_HPP
#define CLP_S_OUTPUTHANDLERIMPL_HPP

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
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

#include <clp_s/AggregationSink.hpp>
#include <clp_s/aggregators.hpp>
#include <clp_s/CommandLineArguments.hpp>
#include <clp_s/ErrorCode.hpp>

#include "../reducer/Pipeline.hpp"
#include "../reducer/RecordGroupIterator.hpp"
#include "Defs.hpp"
#include "FileWriter.hpp"
#include "search/OutputHandler.hpp"
#include "TraceableException.hpp"

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
 * Output handler that writes to a network destination.
 */
class NetworkOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    NetworkOutputHandler(
            std::string const& host,
            int port,
            std::string session_token,
            uint64_t task_idx,
            bool should_output_metadata = false
    );

    // Destructor
    ~NetworkOutputHandler() override {
        if (-1 != m_socket_fd) {
            close(m_socket_fd);
        }
    }

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
    std::string m_host;
    std::string m_port;
    int m_socket_fd;
    std::string m_session_token;
    uint64_t m_task_idx{};
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
