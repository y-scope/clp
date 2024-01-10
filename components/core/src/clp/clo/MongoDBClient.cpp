#include "MongoDBClient.hpp"

namespace clp::clo {
MongoDBClient::MongoDBClient(
        std::string const& uri,
        std::string const& db,
        std::string const& collection,
        uint64_t batch_size
)
        : batch_size_(batch_size) {
    client_ = std::move(mongocxx::client((mongocxx::uri(uri))));
    collection_ = client_[db][collection];
}

void MongoDBClient::flush() {
    if (!results_.empty()) {
        collection_.insert_many(results_);
        results_.clear();
    }
}

void MongoDBClient::add_result(
        std::string const& original_path,
        std::string const& message,
        epochtime_t timestamp
) {
    auto document = bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("original_path", original_path),
            bsoncxx::builder::basic::kvp("message", message),
            bsoncxx::builder::basic::kvp("timestamp", timestamp)
    );

    results_.push_back(std::move(document));

    if (results_.size() >= batch_size_) {
        collection_.insert_many(results_);
        results_.clear();
    }
}
}  // namespace clp::clo
