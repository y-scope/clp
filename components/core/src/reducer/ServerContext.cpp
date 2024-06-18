#include "ServerContext.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <json/single_include/nlohmann/json.hpp>
#include <mongocxx/bulk_write.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/model/replace_one.hpp>
#include <mongocxx/uri.hpp>
#include <msgpack.hpp>

#include "../clp/spdlog_with_specializations.hpp"
#include "CommandLineArguments.hpp"
#include "CountOperator.hpp"
#include "DeserializedRecordGroup.hpp"

using boost::asio::ip::tcp;
using std::vector;

namespace reducer {
namespace {
/**
 * Serializes a record group representing a bucket of the timeline into a BSON object with two
 * kv-pairs:
 * - timestamp: The beginning of the bucket's time range as milliseconds since the UNIX epoch.
 * - count: The number of records in the bucket.
 * @param tags The tags in the record group which should contain a single tag representing the
 * bucket's timestamp.
 * @param record_it An iterator for the records in the record group which should point to a single
 * record with a kv-pair, "count", indicating the bucket's count.
 * @return The serialized data.
 */
vector<uint8_t> serialize_timeline_result(GroupTags const& tags, ConstRecordIterator& record_it);

vector<uint8_t> serialize_timeline_result(GroupTags const& tags, ConstRecordIterator& record_it) {
    nlohmann::json json;
    json["timestamp"] = std::stoll(tags.front());
    auto records = nlohmann::json::array();

    int64_t count = 0;
    for (; false == record_it.done(); record_it.next()) {
        count = record_it.get().get_int64_value(CountOperator::cRecordElementKey);
    }
    json[CountOperator::cRecordElementKey] = count;

    return nlohmann::json::to_bson(json);
}
}  // namespace

// TODO: We should use tcp::v6 and set ip::v6_only to false, but this isn't guaranteed to work; so
// for now, we use v4 to be safe.
ServerContext::ServerContext(CommandLineArguments& args)
        : m_tcp_acceptor{m_ioctx, tcp::endpoint(tcp::v4(), args.get_reducer_port())},
          m_scheduler_socket{m_ioctx},
          m_upsert_timer{m_ioctx},
          m_reducer_host{args.get_reducer_host()},
          m_reducer_port{args.get_reducer_port()},
          m_upsert_interval{args.get_upsert_interval()} {
    mongocxx::uri mongodb_uri = mongocxx::uri(args.get_mongodb_uri());
    try {
        m_mongodb_client = mongocxx::client(mongodb_uri);
    } catch (mongocxx::exception& e) {
        SPDLOG_ERROR("Failed to connect to {} - {}", args.get_mongodb_uri(), e.what());
        throw OperationFailed(clp::ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_mongodb_results_database = mongocxx::database(m_mongodb_client[mongodb_uri.database()]);
}

void ServerContext::reset() {
    m_ioctx.restart();
    m_pipeline.reset(nullptr);
    m_status = ServerStatus::Idle;
    m_job_id = -1;
    m_is_timeline_aggregation = false;
    m_updated_tags.clear();
    m_num_active_receiver_tasks = 0;
}

void ServerContext::stop_event_loop() {
    m_tcp_acceptor.cancel();
    m_scheduler_socket.close();
}

bool ServerContext::register_with_scheduler(
        boost::asio::ip::tcp::resolver::results_type const& endpoints
) {
    try {
        boost::asio::connect(m_scheduler_socket, endpoints);
    } catch (boost::system::system_error& error) {
        SPDLOG_ERROR("Failed to connect to query scheduler - {}", error.what());
        return false;
    }

    boost::system::error_code error;

    nlohmann::json reducer_advertisement;
    reducer_advertisement["host"] = m_reducer_host;
    reducer_advertisement["port"] = m_reducer_port;
    auto serialized_advertisement = nlohmann::json::to_msgpack(reducer_advertisement);
    auto message_size = serialized_advertisement.size();

    boost::asio::write(
            m_scheduler_socket,
            boost::asio::buffer(&message_size, sizeof(message_size)),
            error
    );
    if (error) {
        m_scheduler_socket.close();
        return false;
    }

    boost::asio::write(m_scheduler_socket, boost::asio::buffer(serialized_advertisement), error);
    if (error) {
        m_scheduler_socket.close();
        return false;
    }

    return true;
}

bool ServerContext::ack_query_scheduler() {
    boost::system::error_code e;
    char const scheduler_response = 'y';
    boost::asio::write(m_scheduler_socket, boost::asio::buffer(&scheduler_response, 1), e);
    if (e) {
        SPDLOG_ERROR("Failed to notify query scheduler - {}", e.message());
        return false;
    }
    return true;
}

void ServerContext::decrement_num_active_receiver_tasks() {
    --m_num_active_receiver_tasks;
    if (0 == m_num_active_receiver_tasks && ServerStatus::ReceivedAllResults == m_status) {
        if (false == try_finalize_results()) {
            m_status = ServerStatus::UnrecoverableFailure;
        }
    }
}

void ServerContext::set_up_pipeline(nlohmann::json const& query_config) {
    m_job_id = query_config[cJobAttributes::JobId];

    SPDLOG_INFO("Setting up pipeline for job {}", m_job_id);

    // For now, all pipelines only perform count and optionally, group-by time and count for the
    // timeline aggregation.
    // TODO: We'll need to implement more general pipeline initialization once more operators are
    // needed.
    m_pipeline = std::make_unique<Pipeline>(PipelineInputMode::IntraStage);
    m_pipeline->add_pipeline_stage(std::make_shared<CountOperator>());

    if (query_config.count(cJobAttributes::TimeBucketSize) > 0
        && false == query_config[cJobAttributes::TimeBucketSize].is_null())
    {
        m_is_timeline_aggregation = true;
    }

    auto collection_name = std::to_string(m_job_id);
    m_mongodb_results_collection = m_mongodb_results_database[collection_name];
}

void ServerContext::push_record_group(GroupTags const& tags, ConstRecordIterator& record_it) {
    if (m_is_timeline_aggregation) {
        m_updated_tags.insert(tags);
    }
    m_pipeline->push_record_group(tags, record_it);
}

bool ServerContext::upsert_timeline_results() {
    if (m_updated_tags.empty()) {
        return true;
    }

    bool any_updates = false;
    auto bulk_write = m_mongodb_results_collection.create_bulk_write();
    vector<vector<uint8_t>> results;
    for (auto group_it = m_pipeline->finish(m_updated_tags); false == group_it->done();
         group_it->next())
    {
        int64_t timestamp{std::stoll(group_it->get().get_tags().front())};

        auto& group = group_it->get();
        results.emplace_back(serialize_timeline_result(group.get_tags(), group.record_iter()));

        auto& result = results.back();
        mongocxx::model::replace_one replace_op{
                bsoncxx::builder::basic::make_document(
                        bsoncxx::builder::basic::kvp("timestamp", timestamp)
                ),
                bsoncxx::document::view{result.data(), result.size()}
        };
        replace_op.upsert(true);
        bulk_write.append(replace_op);

        any_updates = true;
    }
    try {
        if (any_updates) {
            bulk_write.execute();
            m_updated_tags.clear();
        }
    } catch (mongocxx::bulk_write_exception const& e) {
        SPDLOG_ERROR("Failed to upsert timeline results - {}", e.what());
        return false;
    }

    return true;
}

bool ServerContext::publish_pipeline_results() {
    vector<vector<uint8_t>> results;
    vector<bsoncxx::document::view> result_documents;
    for (auto group_it = m_pipeline->finish(); false == group_it->done(); group_it->next()) {
        auto& group = group_it->get();
        results.push_back(serialize(group.get_tags(), group.record_iter(), nlohmann::json::to_bson)
        );

        vector<uint8_t>& encoded_result = results.back();
        result_documents.emplace_back(encoded_result.data(), encoded_result.size());
    }
    try {
        if (result_documents.empty() == false) {
            m_mongodb_results_collection.insert_many(result_documents);
        }
    } catch (mongocxx::bulk_write_exception const& e) {
        SPDLOG_ERROR("Failed to publish pipeline results - {}", e.what());
        return false;
    }

    return true;
}

bool ServerContext::try_finalize_results() {
    if (ServerStatus::Running == m_status) {
        // The pipeline's still running
        return true;
    } else if (ServerStatus::ReceivedAllResults != m_status) {
        // The pipeline isn't running
        return false;
    }
    if (m_num_active_receiver_tasks > 0) {
        // We haven't received all results yet
        return true;
    }

    bool published_results_successfully
            = m_is_timeline_aggregation ? upsert_timeline_results() : publish_pipeline_results();
    if (false == published_results_successfully) {
        SPDLOG_ERROR("Failed to publish results to results cache.");
        return false;
    }

    // Notify the query scheduler that the results have been pushed
    return ack_query_scheduler();
}
}  // namespace reducer
