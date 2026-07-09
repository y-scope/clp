#ifndef CLP_S_AGGREGATIONSINK_HPP
#define CLP_S_AGGREGATIONSINK_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <bsoncxx/document/value.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

#include <clp_s/aggregators.hpp>
#include <clp_s/ErrorCode.hpp>

namespace clp_s {
/**
 * Consumes an aggregation's result documents and writes them to a destination.
 */
class AggregationSink {
public:
    // Constructors
    AggregationSink() = default;

    // Delete copy constructor and assignment operator
    AggregationSink(AggregationSink const&) = delete;
    auto operator=(AggregationSink const&) -> AggregationSink& = delete;

    // Delete move constructor and assignment operator
    AggregationSink(AggregationSink&&) = delete;
    auto operator=(AggregationSink&&) -> AggregationSink& = delete;

    // Destructor
    virtual ~AggregationSink() = default;

    // Methods
    /**
     * Writes one result document.
     * @param result The result document to write.
     */
    virtual auto write(AggregationResult const& result) -> void = 0;

    /**
     * Flushes any buffered results.
     * @return ErrorCodeSuccess on success or relevant error code on error
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
    // Constructors
    ResultsCacheSink(
            std::string_view uri,
            std::string_view collection,
            uint64_t batch_size,
            std::string_view archive_id
    );

    // Methods implementing AggregationSink
    /**
     * Buffers a result document, flushing the buffer to the database once it reaches the batch
     * size.
     * @param result The result document to write.
     */
    auto write(AggregationResult const& result) -> void override;

    /**
     * Flushes any remaining buffered result documents.
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeFailureDbBulkWrite if any flush (including an earlier batched one) failed
     */
    [[nodiscard]] auto finish() -> ErrorCode override;

private:
    // Methods
    /**
     * Inserts the buffered result documents into the collection and clears the buffer.
     * @return ErrorCodeSuccess on success, leaving the buffer empty
     * @return ErrorCodeFailureDbBulkWrite on database error, leaving the buffer intact
     */
    [[nodiscard]] auto flush_buffer() -> ErrorCode;

    // Data members
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    uint64_t m_batch_size;
    std::string m_archive_id;
    std::vector<bsoncxx::document::value> m_results;
    ErrorCode m_flush_error{ErrorCode::ErrorCodeSuccess};
};
}  // namespace clp_s

#endif  // CLP_S_AGGREGATIONSINK_HPP
