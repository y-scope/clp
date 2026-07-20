#include "AggregationSink.hpp"

#include <iostream>
#include <string_view>
#include <variant>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <nlohmann/json.hpp>

#include <clp_s/archive_constants.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/ResultsCacheUtils.hpp>

using std::string_view;

namespace clp_s {
auto StdoutSink::write(AggregationResult const& result) -> void {
    nlohmann::json document;
    document[constants::results_cache::search::cArchiveId] = m_archive_id;
    for (auto const& [key, value] : result) {
        std::visit([&](auto const& field_value) { document[key] = field_value; }, value);
    }
    std::cout << document.dump() << '\n';
}

ResultsCacheSink::ResultsCacheSink(string_view uri, string_view collection, string_view archive_id)
        : m_archive_id{archive_id} {
    m_collection = connect_to_results_cache(uri, collection, m_client);
}

auto ResultsCacheSink::write(AggregationResult const& result) -> void {
    bsoncxx::builder::basic::document document;
    document.append(
            bsoncxx::builder::basic::kvp(constants::results_cache::search::cArchiveId, m_archive_id)
    );
    for (auto const& [key, value] : result) {
        std::visit(
                [&](auto const& field_value) {
                    document.append(bsoncxx::builder::basic::kvp(key, field_value));
                },
                value
        );
    }
    m_results.push_back(document.extract());
}

auto ResultsCacheSink::finish() -> ErrorCode {
    if (m_results.empty()) {
        return ErrorCode::ErrorCodeSuccess;
    }

    try {
        m_collection.insert_many(m_results);
        m_results.clear();
    } catch (mongocxx::exception const& e) {
        return ErrorCode::ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCode::ErrorCodeSuccess;
}
}  // namespace clp_s
