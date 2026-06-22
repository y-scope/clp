#include "OutputHandlerImpl.hpp"

#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "../clp/networking/socket_utils.hpp"
#include "../reducer/CountOperator.hpp"
#include "../reducer/network_utils.hpp"
#include "../reducer/Record.hpp"
#include "../reducer/RecordGroup.hpp"
#include "../reducer/RecordGroupIterator.hpp"
#include "archive_constants.hpp"
#include "search/ast/SearchUtils.hpp"
#include "search/OutputHandler.hpp"
#include "TraceableException.hpp"

using std::string;
using std::string_view;

namespace clp_s {
using AggregationType = CommandLineArguments::AggregationType;

namespace {
/**
 * Connects to the results cache and returns the requested collection.
 * @tparam OperationFailedT The type of exception to throw on failure.
 * @param uri
 * @param collection
 * @param client Returns the connected client.
 * @return The collection.
 */
template <typename OperationFailedT>
auto connect_to_results_cache(string const& uri, string const& collection, mongocxx::client& client)
        -> mongocxx::collection;

template <typename OperationFailedT>
auto connect_to_results_cache(string const& uri, string const& collection, mongocxx::client& client)
        -> mongocxx::collection {
    try {
        auto mongo_uri = mongocxx::uri{uri};
        client = mongocxx::client(mongo_uri);
        return client[mongo_uri.database()][collection];
    } catch (mongocxx::exception const& e) {
        throw OperationFailedT(ErrorCode::ErrorCodeBadParamDbUri, __FILENAME__, __LINE__);
    }
}

/**
 * @param type
 * @return Whether the aggregation needs per-record metadata (i.e. the timestamp).
 */
[[nodiscard]] auto aggregation_needs_metadata(AggregationType type) -> bool {
    return AggregationType::CountByTime == type;
}

/**
 * @param type
 * @return Whether the aggregation needs the record marshalled into its JSON message.
 */
[[nodiscard]] auto aggregation_needs_marshalling(AggregationType type) -> bool {
    return AggregationType::Min == type || AggregationType::Max == type;
}
}  // namespace

FieldMinMaxAggregator::FieldMinMaxAggregator(bool find_max, string_view field)
        : m_find_max{find_max} {
    string descriptor_namespace;
    if (false
        == search::ast::tokenize_column_descriptor(
                string{field},
                m_field_path,
                descriptor_namespace
        ))
    {
        // A malformed field tokenizes to nothing, so `update` never matches and min/max yields no
        // result. The field is validated during command-line parsing, so this is just defensive.
        m_field_path.clear();
        return;
    }
    if (false == descriptor_namespace.empty()) {
        // Namespaced fields (e.g. the auto-generated `@` namespace) are marshalled under a
        // top-level object keyed by the namespace string, so navigate into it first.
        m_field_path.insert(m_field_path.begin(), descriptor_namespace);
    }
}

auto FieldMinMaxAggregator::beats_extreme(Value candidate) const -> bool {
    auto const& current{m_extreme.value()};
    // Compare integers exactly; only fall back to a (lossy) double comparison when the types
    // differ.
    if (std::holds_alternative<int64_t>(candidate) && std::holds_alternative<int64_t>(current)) {
        auto const lhs{std::get<int64_t>(candidate)};
        auto const rhs{std::get<int64_t>(current)};
        return m_find_max ? lhs > rhs : lhs < rhs;
    }
    auto const as_double{[](Value value) {
        return std::visit([](auto held) { return static_cast<double>(held); }, value);
    }};
    return m_find_max ? as_double(candidate) > as_double(current)
                      : as_double(candidate) < as_double(current);
}

auto FieldMinMaxAggregator::update(string_view message) -> void {
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(message);
    } catch (nlohmann::json::exception const&) {
        return;
    }

    nlohmann::json const* node{&doc};
    for (auto const& key : m_field_path) {
        if (false == node->is_object()) {
            return;
        }
        auto const it{node->find(key)};
        if (node->end() == it) {
            return;
        }
        node = &it.value();
    }

    if (false == node->is_number()) {
        return;
    }
    Value const candidate{
            node->is_number_integer() ? Value{node->get<int64_t>()} : Value{node->get<double>()}
    };
    if (false == m_extreme.has_value() || beats_extreme(candidate)) {
        m_extreme = candidate;
    }
}

void FileOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) {
    static constexpr string_view cOrigFilePathPlaceholder{""};
    msgpack::type::tuple<epochtime_t, string, string, string, int64_t> const
            src(timestamp, message, cOrigFilePathPlaceholder, archive_id, log_event_idx);
    msgpack::pack(m_file_writer, src);
}

NetworkOutputHandler::NetworkOutputHandler(
        string const& host,
        int port,
        bool should_output_timestamp
)
        : ::clp_s::search::OutputHandler(should_output_timestamp, true) {
    m_socket_fd = clp::networking::connect_to_server(host, std::to_string(port));
    if (-1 == m_socket_fd) {
        SPDLOG_ERROR("Failed to connect to the server, errno={}", errno);
        throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILENAME__, __LINE__);
    }
}

void NetworkOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) {
    static constexpr string_view cOrigFilePathPlaceholder{""};
    msgpack::type::tuple<epochtime_t, string, string, string, int64_t> const
            src(timestamp, message, cOrigFilePathPlaceholder, archive_id, log_event_idx);
    msgpack::sbuffer m;
    msgpack::pack(m, src);

    if (-1 == send(m_socket_fd, m.data(), m.size(), 0)) {
        throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILENAME__, __LINE__);
    }
}

ResultsCacheOutputHandler::ResultsCacheOutputHandler(
        string const& uri,
        string const& collection,
        uint64_t batch_size,
        uint64_t max_num_results,
        string dataset,
        bool should_output_timestamp
)
        : ::clp_s::search::OutputHandler{should_output_timestamp, true},
          m_batch_size{batch_size},
          m_max_num_results{max_num_results},
          m_dataset{std::move(dataset)} {
    m_collection = connect_to_results_cache<OperationFailed>(uri, collection, m_client);
    m_results.reserve(m_batch_size);
}

ErrorCode ResultsCacheOutputHandler::finish() {
    size_t count = 0;
    while (false == m_latest_results.empty()) {
        auto result = std::move(*m_latest_results.top());
        m_latest_results.pop();

        try {
            m_results.emplace_back(
                    std::move(
                            bsoncxx::builder::basic::make_document(
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cOrigFilePath,
                                            std::move(result.original_path)
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cMessage,
                                            std::move(result.message)
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cTimestamp,
                                            result.timestamp
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cArchiveId,
                                            std::move(result.archive_id)
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cLogEventIx,
                                            result.log_event_idx
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            std::string{constants::results_cache::search::cDataset},
                                            std::move(result.dataset)
                                    )
                            )
                    )
            );
            count++;

            if (count == m_batch_size) {
                m_collection.insert_many(m_results);
                m_results.clear();
                count = 0;
            }
        } catch (mongocxx::exception const& e) {
            return ErrorCode::ErrorCodeFailureDbBulkWrite;
        }
    }

    try {
        if (false == m_results.empty()) {
            m_collection.insert_many(m_results);
            m_results.clear();
        }
    } catch (mongocxx::exception const& e) {
        return ErrorCode::ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCode::ErrorCodeSuccess;
}

void ResultsCacheOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) {
    if (m_latest_results.size() < m_max_num_results) {
        m_latest_results.emplace(
                std::make_unique<QueryResult>(
                        string_view{},
                        message,
                        timestamp,
                        archive_id,
                        log_event_idx,
                        m_dataset
                )
        );
    } else if (m_latest_results.top()->timestamp < timestamp) {
        m_latest_results.pop();
        m_latest_results.emplace(
                std::make_unique<QueryResult>(
                        string_view{},
                        message,
                        timestamp,
                        archive_id,
                        log_event_idx,
                        m_dataset
                )
        );
    }
}

CountReducerOutputHandler::CountReducerOutputHandler(int reducer_socket_fd)
        : ::clp_s::search::OutputHandler(false, false),
          m_reducer_socket_fd(reducer_socket_fd),
          m_pipeline(reducer::PipelineInputMode::InterStage) {
    m_pipeline.add_pipeline_stage(std::make_shared<reducer::CountOperator>());
}

auto CountReducerOutputHandler::write(string_view message) -> void {
    m_pipeline.push_record(reducer::EmptyRecord{});
}

auto CountReducerOutputHandler::finish() -> ErrorCode {
    if (false
        == reducer::send_pipeline_results(m_reducer_socket_fd, std::move(m_pipeline.finish())))
    {
        return ErrorCode::ErrorCodeFailureNetwork;
    }
    return ErrorCode::ErrorCodeSuccess;
}

auto CountByTimeReducerOutputHandler::finish() -> ErrorCode {
    if (false
        == reducer::send_pipeline_results(
                m_reducer_socket_fd,
                std::make_unique<reducer::Int64Int64MapRecordGroupIterator>(
                        m_bucket_counts,
                        reducer::CountOperator::cRecordElementKey
                )
        ))
    {
        return ErrorCode::ErrorCodeFailureNetwork;
    }
    return ErrorCode::ErrorCodeSuccess;
}

AggregationToStdoutOutputHandler::AggregationToStdoutOutputHandler(
        string archive_id,
        CommandLineArguments::AggregationType aggregation_type,
        int64_t count_by_time_bucket_size_ms,
        string_view aggregation_field
)
        : ::clp_s::search::
                  OutputHandler{aggregation_needs_metadata(aggregation_type), aggregation_needs_marshalling(aggregation_type)},
          m_archive_id{std::move(archive_id)},
          m_aggregation_type{aggregation_type},
          m_count_by_time_bucket_size_ms{count_by_time_bucket_size_ms},
          m_aggregation_field{aggregation_field} {
    if (AggregationType::Count == m_aggregation_type) {
        m_pipeline.emplace(reducer::PipelineInputMode::InterStage);
        m_pipeline->add_pipeline_stage(std::make_shared<reducer::CountOperator>());
    } else if (AggregationType::Min == m_aggregation_type
               || AggregationType::Max == m_aggregation_type)
    {
        m_min_max.emplace(AggregationType::Max == m_aggregation_type, aggregation_field);
    }
}

void AggregationToStdoutOutputHandler::write(string_view message) {
    if (m_pipeline.has_value()) {
        m_pipeline->push_record(reducer::EmptyRecord{});
    } else if (m_min_max.has_value()) {
        m_min_max->update(message);
    }
}

void AggregationToStdoutOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) {
    int64_t const bucket
            = (timestamp / m_count_by_time_bucket_size_ms) * m_count_by_time_bucket_size_ms;
    m_bucket_counts[bucket] += 1;
}

ErrorCode AggregationToStdoutOutputHandler::finish() {
    if (AggregationType::Min == m_aggregation_type || AggregationType::Max == m_aggregation_type) {
        if (m_min_max.has_value() && m_min_max->has_value()) {
            nlohmann::json result;
            result[constants::results_cache::search::cArchiveId] = m_archive_id;
            result[constants::results_cache::search::cField] = m_aggregation_field;
            auto const* const key = AggregationType::Max == m_aggregation_type
                                            ? constants::results_cache::search::cMax
                                            : constants::results_cache::search::cMin;
            std::visit([&](auto value) { result[key] = value; }, m_min_max->get_value());
            std::cout << result.dump() << '\n';
        }
        return ErrorCode::ErrorCodeSuccess;
    }

    // count results come from the CountOperator pipeline; count-by-time results are serialized from
    // the bucket counts.
    std::unique_ptr<reducer::RecordGroupIterator> results;
    if (AggregationType::CountByTime == m_aggregation_type) {
        results = std::make_unique<reducer::Int64Int64MapRecordGroupIterator>(
                m_bucket_counts,
                reducer::CountOperator::cRecordElementKey
        );
    } else {
        results = m_pipeline->finish();
    }
    for (; false == results->done(); results->next()) {
        auto& group{results->get()};
        nlohmann::json result;
        result[constants::results_cache::search::cArchiveId] = m_archive_id;

        auto const& tags{group.get_tags()};
        if (false == tags.empty()) {
            // For count-by-time the group tag is the (stringified) time bucket.
            result[constants::results_cache::search::cTimestamp] = std::stoll(tags.front());
        }

        // A count group contains exactly one record holding the count.
        auto& record_it{group.record_iter()};
        if (false == record_it.done()) {
            result[constants::results_cache::search::cCount]
                    = record_it.get().get_int64_value(reducer::CountOperator::cRecordElementKey);
        }

        std::cout << result.dump() << '\n';
    }
    return ErrorCode::ErrorCodeSuccess;
}

AggregationToResultsCacheOutputHandler::AggregationToResultsCacheOutputHandler(
        string const& uri,
        string const& collection,
        string archive_id,
        CommandLineArguments::AggregationType aggregation_type,
        int64_t count_by_time_bucket_size_ms,
        string_view aggregation_field
)
        : ::clp_s::search::
                  OutputHandler{aggregation_needs_metadata(aggregation_type), aggregation_needs_marshalling(aggregation_type)},
          m_archive_id{std::move(archive_id)},
          m_aggregation_type{aggregation_type},
          m_count_by_time_bucket_size_ms{count_by_time_bucket_size_ms},
          m_aggregation_field{aggregation_field} {
    if (AggregationType::Min == m_aggregation_type || AggregationType::Max == m_aggregation_type) {
        m_min_max.emplace(AggregationType::Max == m_aggregation_type, aggregation_field);
    }
    m_collection = connect_to_results_cache<OperationFailed>(uri, collection, m_client);
}

auto AggregationToResultsCacheOutputHandler::finish() -> ErrorCode {
    try {
        switch (m_aggregation_type) {
            case AggregationType::Count: {
                if (0 == m_count) {
                    return ErrorCode::ErrorCodeSuccess;
                }
                m_collection.insert_one(
                        bsoncxx::builder::basic::make_document(
                                bsoncxx::builder::basic::kvp(
                                        constants::results_cache::search::cArchiveId,
                                        m_archive_id
                                ),
                                bsoncxx::builder::basic::kvp(
                                        constants::results_cache::search::cCount,
                                        m_count
                                )
                        )
                );
                break;
            }
            case AggregationType::CountByTime: {
                if (m_bucket_counts.empty()) {
                    return ErrorCode::ErrorCodeSuccess;
                }
                std::vector<bsoncxx::document::value> documents;
                documents.reserve(m_bucket_counts.size());
                for (auto const& [bucket_timestamp, count] : m_bucket_counts) {
                    documents.emplace_back(
                            bsoncxx::builder::basic::make_document(
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cArchiveId,
                                            m_archive_id
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cTimestamp,
                                            bucket_timestamp
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cCount,
                                            count
                                    )
                            )
                    );
                }
                m_collection.insert_many(documents);
                break;
            }
            case AggregationType::Min:
            case AggregationType::Max: {
                if (false == m_min_max.has_value() || false == m_min_max->has_value()) {
                    return ErrorCode::ErrorCodeSuccess;
                }
                bsoncxx::builder::basic::document document;
                document.append(
                        bsoncxx::builder::basic::kvp(
                                constants::results_cache::search::cArchiveId,
                                m_archive_id
                        ),
                        bsoncxx::builder::basic::kvp(
                                constants::results_cache::search::cField,
                                m_aggregation_field
                        )
                );
                auto const append_extreme = [&](auto const& key) {
                    std::visit(
                            [&](auto value) {
                                document.append(bsoncxx::builder::basic::kvp(key, value));
                            },
                            m_min_max->get_value()
                    );
                };
                if (AggregationType::Max == m_aggregation_type) {
                    append_extreme(constants::results_cache::search::cMax);
                } else {
                    append_extreme(constants::results_cache::search::cMin);
                }
                m_collection.insert_one(document.view());
                break;
            }
        }
    } catch (mongocxx::exception const& e) {
        return ErrorCode::ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCode::ErrorCodeSuccess;
}
}  // namespace clp_s
