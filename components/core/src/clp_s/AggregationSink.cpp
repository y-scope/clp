#include "AggregationSink.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/model/insert_one.hpp>
#include <mongocxx/model/replace_one.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/archive_constants.hpp>
#include <clp_s/ResultsCacheUtils.hpp>

namespace clp_s {
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using std::string_view;

auto StdoutSink::write(AggregationResult const& result) -> ystdlib::error_handling::Result<void> {
    nlohmann::json document;
    document[constants::results_cache::search::cArchiveId] = m_archive_id;
    auto const append_fields{[&](AggregationFields const& fields) -> void {
        for (auto const& [key, value] : fields) {
            std::visit(
                    [&](auto const& field_value) -> void { document[key] = field_value; },
                    value
            );
        }
    }};
    if (result.optional_key.has_value()) {
        append_fields(result.optional_key.value());
    }
    append_fields(result.fields);
    std::cout << document.dump() << '\n';
    return ystdlib::error_handling::success();
}

ResultsCacheSink::ResultsCacheSink(
        string_view uri,
        string_view collection,
        uint64_t batch_size,
        string_view archive_id,
        string_view dataset
)
        : m_batch_size{batch_size},
          m_archive_id{archive_id},
          m_dataset{dataset} {
    m_collection = connect_to_results_cache(uri, collection, m_client);
}

auto ResultsCacheSink::flush_buffer() -> ystdlib::error_handling::Result<void> {
    if (m_writes.empty()) {
        return ystdlib::error_handling::success();
    }

    try {
        auto const result{m_collection.bulk_write(m_writes)};
        if (m_requires_acknowledgment && false == result.has_value()) {
            return std::errc::io_error;
        }
    } catch (mongocxx::exception const& e) {
        SPDLOG_ERROR("Failed to flush results to Results Cache: {}", e.what());
        return std::errc::io_error;
    }
    m_writes.clear();
    m_requires_acknowledgment = false;
    return ystdlib::error_handling::success();
}

auto ResultsCacheSink::write(AggregationResult const& result)
        -> ystdlib::error_handling::Result<void> {
    auto const append_fields{[](AggregationFields const& fields, document& output) -> void {
        for (auto const& [key, value] : fields) {
            std::visit(
                    [&](auto const& field_value) -> void { output.append(kvp(key, field_value)); },
                    value
            );
        }
    }};
    document output;
    append_fields(result.fields, output);
    if (result.optional_key.has_value()) {
        document identity;
        identity.append(
                kvp(std::string{constants::results_cache::search::cDataset}, m_dataset),
                kvp(constants::results_cache::search::cArchiveId, m_archive_id)
        );
        append_fields(result.optional_key.value(), identity);
        document filter;
        filter.append(kvp(constants::results_cache::search::cId, identity.view()));
        output.append(kvp(constants::results_cache::search::cId, identity.extract()));
        mongocxx::model::replace_one replacement{filter.extract(), output.extract()};
        replacement.upsert(true);
        m_writes.emplace_back(std::move(replacement));
        m_requires_acknowledgment = true;
    } else {
        output.append(kvp(constants::results_cache::search::cArchiveId, m_archive_id));
        m_writes.emplace_back(mongocxx::model::insert_one{output.extract()});
    }

    if (m_writes.size() >= m_batch_size) {
        YSTDLIB_ERROR_HANDLING_TRYV(flush_buffer());
    }

    return ystdlib::error_handling::success();
}
}  // namespace clp_s
