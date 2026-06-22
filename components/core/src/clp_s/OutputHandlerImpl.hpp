#ifndef CLP_S_OUTPUTHANDLERIMPL_HPP
#define CLP_S_OUTPUTHANDLERIMPL_HPP

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

#include "../reducer/Pipeline.hpp"
#include "../reducer/RecordGroupIterator.hpp"
#include "CommandLineArguments.hpp"
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
    void write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) override {
        std::cout << archive_id << ": " << log_event_idx << ": " << timestamp << " " << message;
    }

    void write(std::string_view message) override { std::cout << message; }
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
    void write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) override;

    void write(std::string_view message) override { write(message, 0, {}, 0); }

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
    explicit NetworkOutputHandler(
            std::string const& host,
            int port,
            bool should_output_metadata = false
    );

    // Destructor
    ~NetworkOutputHandler() override {
        if (-1 != m_socket_fd) {
            close(m_socket_fd);
        }
    }

    // Methods inherited from OutputHandler
    void write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) override;

    void write(std::string_view message) override { write(message, 0, {}, 0); }

private:
    std::string m_host;
    std::string m_port;
    int m_socket_fd;
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
            std::string dataset,
            bool should_output_metadata = true
    );

    // Methods inherited from OutputHandler
    /**
     * No-op for this handler. The results heap is drained in `finish()` so that
     * `max_num_results` is enforced across all ERTs in the archive.
     * @return ErrorCodeSuccess
     */
    [[nodiscard]] auto flush() -> ErrorCode override { return ErrorCode::ErrorCodeSuccess; }

    /**
     * Flushes the output handler after all tables are searched.
     * @return ErrorCodeSuccess on success.
     * @return ErrorCodeFailureDbBulkWrite on failure to write results to the results cache.
     */
    auto finish() -> ErrorCode override;

    void write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) override;

    void write(std::string_view message) override { write(message, 0, {}, 0); }

private:
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::vector<bsoncxx::document::value> m_results;
    uint64_t m_batch_size;
    uint64_t m_max_num_results;
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
class CountReducerOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Constructors
    CountReducerOutputHandler(int reducer_socket_fd);

    // Methods implementing OutputHandler
    auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> void override {}

    auto write(std::string_view message) -> void override;

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
class CountByTimeReducerOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Constructors
    CountByTimeReducerOutputHandler(int reducer_socket_fd, int64_t count_by_time_bucket_size_ms)
            : search::OutputHandler{true, false},
              m_reducer_socket_fd{reducer_socket_fd},
              m_count_by_time_bucket_size_ms{count_by_time_bucket_size_ms} {}

    // Methods implementing OutputHandler
    auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> void override {
        int64_t bucket
                = (timestamp / m_count_by_time_bucket_size_ms) * m_count_by_time_bucket_size_ms;
        m_bucket_counts[bucket] += 1;
    }

    auto write(std::string_view message) -> void override {}

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
    int64_t m_count_by_time_bucket_size_ms;
};

/**
 * Accumulates the minimum or maximum value of a single numeric field across the records it is fed.
 * The field is identified by a KQL column descriptor (e.g. `a.b.c`, or `@x` for the auto-generated
 * namespace) resolved against each record's marshalled JSON. Records that lack the field or whose
 * value is non-numeric are ignored. The extreme preserves whether it was an integer or a
 * floating-point value so integer fields aren't degraded to `double`.
 */
class FieldMinMaxAggregator {
public:
    // Types
    using Value = std::variant<int64_t, double>;

    // Constructors
    FieldMinMaxAggregator(bool find_max, std::string_view field);

    // Methods
    /**
     * Updates the running extreme with the target field's value extracted from `message`.
     * @param message The marshalled JSON of a matched record.
     */
    auto update(std::string_view message) -> void;

    [[nodiscard]] auto has_value() const -> bool { return m_extreme.has_value(); }

    /**
     * @return The accumulated extreme, preserving its integer or floating-point type.
     */
    [[nodiscard]] auto get_value() const -> Value { return m_extreme.value(); }

private:
    // Methods
    /**
     * @param candidate
     * @return Whether `candidate` is more extreme (per the min/max mode) than the current extreme.
     */
    [[nodiscard]] auto beats_extreme(Value candidate) const -> bool;

    // Data members
    bool m_find_max;
    std::vector<std::string> m_field_path;
    std::optional<Value> m_extreme;
};

/**
 * Output handler that performs an aggregation (count, count-by-time, min, or max) and writes the
 * results to the results cache (MongoDB). Each archive writes its own documents.
 */
class AggregationToResultsCacheOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    AggregationToResultsCacheOutputHandler(
            std::string const& uri,
            std::string const& collection,
            std::string archive_id,
            CommandLineArguments::AggregationType aggregation_type,
            int64_t count_by_time_bucket_size_ms,
            std::string_view aggregation_field
    );

    // Methods implementing OutputHandler
    auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> void override {
        int64_t const bucket
                = (timestamp / m_count_by_time_bucket_size_ms) * m_count_by_time_bucket_size_ms;
        m_bucket_counts[bucket] += 1;
    }

    auto write(std::string_view message) -> void override {
        if (CommandLineArguments::AggregationType::Count == m_aggregation_type) {
            m_count += 1;
        } else if (m_min_max.has_value()) {
            m_min_max->update(message);
        }
    }

    // Methods overriding OutputHandler
    /**
     * Writes the aggregation results to the results cache.
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeFailureDbBulkWrite on database error
     */
    auto finish() -> ErrorCode override;

private:
    // Data members
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::string m_archive_id;
    CommandLineArguments::AggregationType m_aggregation_type;
    int64_t m_count_by_time_bucket_size_ms;
    int64_t m_count{};
    std::map<int64_t, int64_t> m_bucket_counts;
    std::string m_aggregation_field;
    std::optional<FieldMinMaxAggregator> m_min_max;
};

/**
 * Output handler that performs an aggregation (count, count-by-time, min, or max) and writes the
 * results to standard output as newline-delimited JSON, one object per group.
 *
 * Count accumulates through a `reducer::CountOperator` pipeline (yielding a single total),
 * count-by-time accumulates a count per time bucket, and min/max accumulate the extreme value of a
 * target field.
 */
class AggregationToStdoutOutputHandler : public ::clp_s::search::OutputHandler {
public:
    // Constructors
    AggregationToStdoutOutputHandler(
            std::string archive_id,
            CommandLineArguments::AggregationType aggregation_type,
            int64_t count_by_time_bucket_size_ms,
            std::string_view aggregation_field
    );

    // Methods implementing OutputHandler
    void write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) override;

    void write(std::string_view message) override;

    // Methods overriding OutputHandler
    /**
     * Serializes the aggregation results to stdout as newline-delimited JSON.
     * @return ErrorCodeSuccess on success
     */
    ErrorCode finish() override;

private:
    // Data members
    std::string m_archive_id;
    CommandLineArguments::AggregationType m_aggregation_type;
    int64_t m_count_by_time_bucket_size_ms;
    std::optional<reducer::Pipeline> m_pipeline;
    std::map<int64_t, int64_t> m_bucket_counts;
    std::string m_aggregation_field;
    std::optional<FieldMinMaxAggregator> m_min_max;
};

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
    void write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) override {
        m_output.emplace_back(message, timestamp, archive_id, log_event_idx);
    }

    void write(std::string_view message) override {
        m_output.emplace_back(message, epochtime_t{}, std::string_view{}, int64_t{});
    }

private:
    std::vector<QueryResult>& m_output;
};
}  // namespace clp_s

#endif  // CLP_S_OUTPUTHANDLERIMPL_HPP
