#include "AggregationSink.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/document/element.hpp>
#include <bsoncxx/document/view.hpp>
#include <mongocxx/bulk_write.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/model/replace_one.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/archive_constants.hpp>
#include <clp_s/ResultsCacheUtils.hpp>

namespace clp_s {
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using std::string_view;

auto StdoutSink::write(AggregationResult const& result) -> ystdlib::error_handling::Result<void> {
    nlohmann::json document;
    document[constants::results_cache::search::cArchiveId] = m_archive_id;
    for (auto const& [key, value] : result) {
        std::visit([&](auto const& field_value) -> void { document[key] = field_value; }, value);
    }
    std::cout << document.dump() << '\n';
    return ystdlib::error_handling::success();
}

ResultsCacheSink::ResultsCacheSink(
        string_view uri,
        string_view collection,
        uint64_t batch_size,
        string_view archive_id,
        string_view dataset,
        WriteMode write_mode
)
        : m_batch_size{batch_size},
          m_archive_id{archive_id},
          m_dataset{dataset},
          m_write_mode{write_mode} {
    m_collection = connect_to_results_cache(uri, collection, m_client);
}

auto ResultsCacheSink::flush_buffer() -> ystdlib::error_handling::Result<void> {
    if (m_results.empty()) {
        return ystdlib::error_handling::success();
    }

    try {
        if (WriteMode::UpsertCountByTime == m_write_mode) {
            auto bulk{m_collection.create_bulk_write()};
            for (auto const& document : m_results) {
                auto const filter{bsoncxx::builder::basic::make_document(
                        bsoncxx::builder::basic::kvp(
                                constants::results_cache::search::cId,
                                document.view()[constants::results_cache::search::cId]
                                        .get_document()
                        )
                )};
                mongocxx::model::replace_one replacement{filter.view(), document.view()};
                replacement.upsert(true);
                bulk.append(replacement);
            }
            if (false == bulk.execute().has_value()) {
                return std::errc::io_error;
            }
        } else {
            m_collection.insert_many(m_results);
        }
    } catch (mongocxx::exception const& e) {
        SPDLOG_ERROR("Failed to flush results to Results Cache: {}", e.what());
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
                [&](auto const& field_value) -> void {
                    document.append(bsoncxx::builder::basic::kvp(key, field_value));
                },
                value
        );
    }
    if (WriteMode::UpsertCountByTime == m_write_mode) {
        namespace fields = constants::results_cache::search;
        // Embedded-document equality is field-order sensitive. Keep this order stable.
        auto const identity{make_document(
                kvp(std::string{fields::cDataset}, m_dataset),
                kvp(fields::cArchiveId, m_archive_id),
                kvp(fields::cTimestamp, document.view()[fields::cTimestamp].get_int64())
        )};
        document.append(
                kvp(fields::cId, identity.view()),
                kvp(std::string{fields::cDataset}, m_dataset)
        );
    }
    m_results.push_back(document.extract());

    if (m_results.size() >= m_batch_size) {
        YSTDLIB_ERROR_HANDLING_TRYV(flush_buffer());
    }

    return ystdlib::error_handling::success();
}
}  // namespace clp_s
