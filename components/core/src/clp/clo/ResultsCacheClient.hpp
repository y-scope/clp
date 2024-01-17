#ifndef CLP_CLO_MONGODBCLIENT_HPP
#define CLP_CLO_MONGODBCLIENT_HPP

#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/uri.hpp>

#include "../Defs.h"
#include "../TraceableException.hpp"

namespace clp::clo {
/**
 * Class encapsulating a MongoDB client used to send query results to results cache.
 */
class ResultsCacheClient {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "ResultsCacheClient operation failed";
        }
    };

    // Constructors
    ResultsCacheClient(
            std::string const& uri,
            std::string const& db,
            std::string const& collection,
            uint64_t batch_size = 1000
    );

    // Methods
    /**
     * Adds a result to the batch.
     * @param original_path The original path of the log event.
     * @param message The content of the log event.
     * @param timestamp The timestamp of the log event.
     */
    void
    add_result(std::string const& original_path, std::string const& message, epochtime_t timestamp);

    /**
     * Flushes the batch.
     */
    void flush();

private:
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::vector<bsoncxx::document::value> m_results;
    uint64_t m_batch_size;
};
}  // namespace clp::clo

#endif  // CLP_CLO_MONGODBCLIENT_HPP
