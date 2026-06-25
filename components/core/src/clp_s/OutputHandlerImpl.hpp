#ifndef CLP_S_OUTPUTHANDLERIMPL_HPP
#define CLP_S_OUTPUTHANDLERIMPL_HPP

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

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
            std::string_view uri,
            std::string_view collection,
            uint64_t batch_size,
            uint64_t max_num_results,
            std::string_view dataset,
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
class CountReducerOutputHandler : public search::OutputHandler {
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
class CountByTimeReducerOutputHandler : public search::OutputHandler {
public:
    // Constructors
    CountByTimeReducerOutputHandler(int reducer_socket_fd, int64_t count_by_time_bucket_size_ms)
            : search::OutputHandler{true, false},
              m_reducer_socket_fd{reducer_socket_fd},
              m_count_by_time_bucket_size_ms{count_by_time_bucket_size_ms} {}

    // Methods implementing OutputHandler
    auto write(
            std::string_view message,
            epochtime_t timestamp_ms,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> void override {
        int64_t bucket
                = (timestamp_ms / m_count_by_time_bucket_size_ms) * m_count_by_time_bucket_size_ms;
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
 * Output handler that performs a count aggregation and writes the results to the results cache.
 */
class CountResultsCacheOutputHandler : public search::OutputHandler {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    CountResultsCacheOutputHandler(
            std::string_view uri,
            std::string_view collection,
            std::string_view archive_id
    );

    // Methods implementing OutputHandler
    auto write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> void override {}

    auto write(std::string_view message) -> void override { m_count += 1; }

    // Methods overriding OutputHandler
    /**
     * Flushes the count.
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeFailureDbBulkWrite on database error
     */
    auto finish() -> ErrorCode override;

private:
    // Data members
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::string m_archive_id;
    int64_t m_count{};
};

/**
 * Output handler that performs a count aggregation bucketed by time and writes the results to the
 * results cache.
 */
class CountByTimeResultsCacheOutputHandler : public search::OutputHandler {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    CountByTimeResultsCacheOutputHandler(
            std::string_view uri,
            std::string_view collection,
            std::string_view archive_id,
            int64_t count_by_time_bucket_size_ms
    );

    // Methods implementing OutputHandler
    auto write(
            std::string_view message,
            epochtime_t timestamp_ms,
            std::string_view archive_id,
            int64_t log_event_idx
    ) -> void override {
        int64_t bucket
                = (timestamp_ms / m_count_by_time_bucket_size_ms) * m_count_by_time_bucket_size_ms;
        m_bucket_counts[bucket] += 1;
    }

    auto write(std::string_view message) -> void override {}

    // Methods overriding OutputHandler
    /**
     * Flushes the counts.
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeFailureDbBulkWrite on database error
     */
    auto finish() -> ErrorCode override;

private:
    // Data members
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::string m_archive_id;
    std::map<int64_t, int64_t> m_bucket_counts;
    int64_t m_count_by_time_bucket_size_ms;
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
