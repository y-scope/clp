#include "AggregationSink.hpp"

#include <cstdint>
#include <iostream>
#include <string_view>
#include <system_error>
#include <variant>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/archive_constants.hpp>
#include <clp_s/ResultsCacheUtils.hpp>

using std::string_view;

namespace clp_s {
auto StdoutSink::write(AggregationResult const& result) -> ystdlib::error_handling::Result<void> {
    nlohmann::json document;
    document[constants::results_cache::search::cArchiveId] = m_archive_id;
    for (auto const& [key, value] : result) {
        std::visit([&](auto const& field_value) { document[key] = field_value; }, value);
    }
    std::cout << document.dump() << '\n';
    return ystdlib::error_handling::success();
}

ResultsCacheSink::ResultsCacheSink(
        string_view uri,
        string_view collection,
        uint64_t batch_size,
        string_view archive_id
)
        : m_batch_size{batch_size},
          m_archive_id{archive_id} {
    m_collection = connect_to_results_cache(uri, collection, m_client);
}

auto ResultsCacheSink::flush_buffer() -> ystdlib::error_handling::Result<void> {
    if (m_results.empty()) {
        return ystdlib::error_handling::success();
    }

    try {
        m_collection.insert_many(m_results);
    } catch (mongocxx::exception const& e) {
        SPDLOG_ERROR("ResultsCacheSink failed flush:{}", e.what());
        return std::errc::io_error;
    }
    m_results.clear();
    return ystdlib::error_handling::success();
}

auto ResultsCacheSink::write(AggregationResult const& result)
        -> ystdlib::error_handling::Result<void> {
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

    if (m_results.size() >= m_batch_size) {
        YSTDLIB_ERROR_HANDLING_TRYV(flush_buffer());
    }

    return ystdlib::error_handling::success();
}
}  // namespace clp_s
