#ifndef CLP_S_AGGREGATIONSINK_HPP
#define CLP_S_AGGREGATIONSINK_HPP

#include <string>
#include <string_view>
#include <vector>

#include <bsoncxx/document/value.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

#include "Aggregation.hpp"
#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp_s {
/**
 * Consumes a search aggregation's result documents and writes them to a destination.
 */
class AggregationSink {
public:
    // Constructors
    AggregationSink() = default;

    // Destructor
    virtual ~AggregationSink() = default;

    // Delete copy and move
    AggregationSink(AggregationSink const&) = delete;
    auto operator=(AggregationSink const&) -> AggregationSink& = delete;
    AggregationSink(AggregationSink&&) = delete;
    auto operator=(AggregationSink&&) -> AggregationSink& = delete;

    // Methods
    /**
     * Writes one result document. The archive id is added by the sink.
     * @param result
     */
    virtual auto write(AggregationResult const& result) -> void = 0;

    /**
     * Flushes any buffered results.
     * @return ErrorCodeSuccess on success
     */
    [[nodiscard]] virtual auto finish() -> ErrorCode = 0;
};

/**
 * Sink that writes aggregation results to standard output as newline-delimited JSON.
 */
class StdoutSink : public AggregationSink {
public:
    // Constructors
    explicit StdoutSink(std::string_view archive_id) : m_archive_id{archive_id} {}

    // Methods implementing AggregationSink
    auto write(AggregationResult const& result) -> void override;

    [[nodiscard]] auto finish() -> ErrorCode override { return ErrorCode::ErrorCodeSuccess; }

private:
    // Data members
    std::string m_archive_id;
};

/**
 * Sink that writes aggregation results to a MongoDB results-cache collection.
 */
class ResultsCacheSink : public AggregationSink {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    ResultsCacheSink(
            std::string_view uri,
            std::string_view collection,
            std::string_view archive_id
    );

    // Methods implementing AggregationSink
    auto write(AggregationResult const& result) -> void override;

    /**
     * Flushes the buffered result documents to the results cache.
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeFailureDbBulkWrite on database error
     */
    [[nodiscard]] auto finish() -> ErrorCode override;

private:
    // Data members
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::string m_archive_id;
    std::vector<bsoncxx::document::value> m_results;
};
}  // namespace clp_s

#endif  // CLP_S_AGGREGATIONSINK_HPP
