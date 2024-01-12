#ifndef CLP_S_SEARCH_OUTPUTHANDLER_HPP
#define CLP_S_SEARCH_OUTPUTHANDLER_HPP

#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "../Defs.hpp"

namespace clp_s::search {
class OutputHandler {
public:
    explicit OutputHandler(bool output_timestamp, bool decompress_full_log = true)
            : output_timestamp_(output_timestamp),
              decompress_full_log_(decompress_full_log) {}

    virtual ~OutputHandler() = default;

    virtual void Write(std::string const& message, epochtime_t timestamp) = 0;
    virtual void Write(std::string const& message) = 0;
    virtual void Flush() = 0;

    bool OutputTimestamp() const { return output_timestamp_; }

    bool DecompressFullLog() const { return decompress_full_log_; }

    void SetDecompressFullLog(bool decompress_full_log) {
        decompress_full_log_ = decompress_full_log;
    }

protected:
    bool output_timestamp_;
    bool decompress_full_log_;
};

class StandardOutputHandler : public OutputHandler {
public:
    explicit StandardOutputHandler(bool output_timestamp = false)
            : OutputHandler(output_timestamp) {}

    void Write(std::string const& message, epochtime_t timestamp) override {
        printf("%lld %s", timestamp, message.c_str());
    }

    void Flush() override {}

    void Write(std::string const& message) override { printf("%s", message.c_str()); }
};

class MongoOutputHandler : public OutputHandler {
public:
    MongoOutputHandler(
            std::string const& uri,
            std::string const& db,
            std::string const& collection,
            bool output_timestamp = false,
            uint64_t batch_size = 1000
    )
            : OutputHandler(output_timestamp),
              batch_size_(batch_size) {
        client_ = mongocxx::client((mongocxx::uri(uri)));
        collection_ = client_[db][collection];
    }

    void Flush() override {
        if (!results_.empty()) {
            collection_.insert_many(results_);
            results_.clear();
        }
    }

    void Write(std::string const& message, epochtime_t timestamp) override {
        auto document = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("path", "logs.ndjson"),
                bsoncxx::builder::basic::kvp("message", message),
                bsoncxx::builder::basic::kvp("timestamp", timestamp)
        );

        results_.push_back(std::move(document));

        if (results_.size() >= batch_size_) {
            collection_.insert_many(results_);
            results_.clear();
        }
    }

    void Write(std::string const& message) override { Write(message, 0); }

private:
    mongocxx::instance instance_{};
    mongocxx::client client_;
    mongocxx::collection collection_;
    std::vector<bsoncxx::document::value> results_;
    uint64_t batch_size_;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_OUTPUTHANDLER_HPP
