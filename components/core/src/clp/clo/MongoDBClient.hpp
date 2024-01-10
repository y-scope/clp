#ifndef CLP_CLO_MONGODBCLIENT_HPP
#define CLP_CLO_MONGODBCLIENT_HPP

#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "../Defs.h"

namespace clp::clo {
class MongoDBClient {
public:
    // Constructors
    MongoDBClient(
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
    mongocxx::instance instance_{};
    mongocxx::client client_;
    mongocxx::collection collection_;
    std::vector<bsoncxx::document::value> results_;
    uint64_t batch_size_;
};
}  // namespace clp::clo

#endif  // CLP_CLO_MONGODBCLIENT_HPP
