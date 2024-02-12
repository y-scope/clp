#include "ServerContext.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <fmt/core.h>
#include <json/single_include/nlohmann/json.hpp>
#include <mongocxx/bulk_write.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/model/replace_one.hpp>
#include <mongocxx/uri.hpp>
#include <msgpack.hpp>
#include <string_utils/string_utils.hpp>

#include "../clp/spdlog_with_specializations.hpp"
#include "../clp/type_utils.hpp"
#include "CommandLineArguments.hpp"
#include "CountOperator.hpp"
#include "RecordGroupSerdes.hpp"

using boost::asio::ip::tcp;

namespace reducer {

// TODO: We should use tcp::v6 and set ip::v6_only to false. But this isn't
// guaranteed to work, so for now, we use v4 to be safe.
ServerContext::ServerContext(CommandLineArguments& args)
        : m_tcp_acceptor(m_ioctx, tcp::endpoint(tcp::v4(), args.get_reducer_port())),
          m_scheduler_socket(m_ioctx),
          m_upsert_timer(m_ioctx),
          m_reducer_host(args.get_reducer_host()),
          m_reducer_port(args.get_reducer_port()),
          m_polling_interval_ms(args.get_polling_interval()),
          m_pipeline(nullptr),
          m_status(ServerStatus::Idle),
          m_job_id(-1),
          m_timeline_aggregation(false) {
    mongocxx::uri mongodb_uri = mongocxx::uri(args.get_mongodb_uri());
    try {
        m_mongodb_client = mongocxx::client(mongodb_uri);
    } catch (mongocxx::exception& e) {
        SPDLOG_ERROR("Failed to connect to {} - {}", args.get_mongodb_uri(), e.what());
        throw OperationFailed(clp::ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_mongodb_results_database = mongocxx::database(m_mongodb_client[mongodb_uri.database()]);
}

void ServerContext::set_up_pipeline(std::map<std::string, msgpack::type::variant>& query_config) {
    m_job_id = query_config["job_id"].as_int64_t();

    SPDLOG_INFO("Taking job {}", m_job_id);

    // TODO: create a more robust method to specify reducer jobs and create pipelines
    if (query_config.count(JobAttributes::cBucketSize)) {
        m_timeline_aggregation = true;
    } else {
        m_timeline_aggregation = false;
    }

    // TODO: For now all pipelines only perform count. We will need to implement more
    // general pipeline initialization once more operators are implemented.
    m_pipeline = std::make_unique<Pipeline>(PipelineInputMode::INTRA_STAGE);
    m_pipeline->add_pipeline_stage(std::make_shared<CountOperator>());

    auto collection_name = std::to_string(m_job_id);
    m_mongodb_results_collection = m_mongodb_results_database[collection_name];
}

bool ServerContext::ack_search_scheduler() {
    boost::system::error_code e;
    char const scheduler_response = 'y';
    m_scheduler_socket.write_some(boost::asio::buffer(&scheduler_response, 1), e);
    if (e) {
        SPDLOG_INFO("Failed to notify search scheduler - {}", e.message());
        return false;
    }
    return true;
}

bool ServerContext::upsert_timeline_results() {
    if (m_updated_tags.empty()) {
        return true;
    }

    auto bulk_write = m_mongodb_results_collection.create_bulk_write();

    bool any_updates = false;
    std::vector<std::vector<uint8_t>> results;
    for (auto group_it = m_pipeline->finish(m_updated_tags); !group_it->done(); group_it->next()) {
        int64_t timestamp = std::stoll(group_it->get()->get_tags()[0]);
        results.push_back(serialize_timeline(*group_it->get()));
        std::vector<uint8_t>& encoded_result = results.back();
        mongocxx::model::replace_one replace_op(
                bsoncxx::builder::basic::make_document(
                        bsoncxx::builder::basic::kvp("timestamp", timestamp)
                ),
                bsoncxx::document::view(encoded_result.data(), encoded_result.size())
        );
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
        SPDLOG_ERROR("MongoDB bulk write exception during upsert: {}", e.what());
        return false;
    }
    return true;
}

bool ServerContext::publish_pipeline_results() {
    std::vector<std::vector<uint8_t>> results;
    std::vector<bsoncxx::document::view> result_documents;
    for (auto group_it = m_pipeline->finish(); !group_it->done(); group_it->next()) {
        results.push_back(serialize(*group_it->get(), nlohmann::json::to_bson));
        std::vector<uint8_t>& encoded_result = results.back();
        result_documents.push_back(
                bsoncxx::document::view(encoded_result.data(), encoded_result.size())
        );
    }

    try {
        if (result_documents.size() > 0) {
            m_mongodb_results_collection.insert_many(result_documents);
        }
    } catch (mongocxx::bulk_write_exception const& e) {
        SPDLOG_ERROR("MongoDB bulk write exception during while dumping results: {}", e.what());
        return false;
    }
    return true;
}

bool ServerContext::try_finalize_results() {
    if (ServerStatus::ReceivedAllResults == m_status && 0 == m_remaining_receiver_tasks) {
        bool pushed_results_succesfully{false};
        if (m_timeline_aggregation) {
            pushed_results_succesfully = upsert_timeline_results();
        } else {
            pushed_results_succesfully = publish_pipeline_results();
        }

        if (false == pushed_results_succesfully) {
            SPDLOG_ERROR("Failed to push results to results cache");
            return false;
        }

        // notify the search scheduler that the results have been pushed
        return ack_search_scheduler();
    }
    return true;
}

void ServerContext::reset() {
    m_ioctx.reset();
    m_pipeline.reset(nullptr);
    m_status = ServerStatus::Idle;
    m_job_id = -1;
    m_timeline_aggregation = false;
    m_updated_tags.clear();
    m_remaining_receiver_tasks = 0;
}

void ServerContext::decrement_remaining_receiver_tasks() {
    if (0 == --m_remaining_receiver_tasks && ServerStatus::ReceivedAllResults == m_status) {
        if (false == try_finalize_results()) {
            m_status = ServerStatus::UnrecoverableFailure;
        }
    }
}

bool ServerContext::register_with_scheduler(
        boost::asio::ip::tcp::resolver::results_type const& endpoint
) {
    try {
        boost::asio::connect(m_scheduler_socket, endpoint);
    } catch (boost::system::system_error& error) {
        SPDLOG_ERROR("Failed to connect to search scheduler - {}", error.what());
        return false;
    }

    nlohmann::json reducer_advertisement;
    reducer_advertisement["host"] = m_reducer_host;
    reducer_advertisement["port"] = m_reducer_port;
    auto serialized_advertisement = nlohmann::json::to_msgpack(reducer_advertisement);
    size_t message_size = serialized_advertisement.size();
    boost::system::error_code error;
    m_scheduler_socket.write_some(boost::asio::buffer(&message_size, sizeof(message_size)), error);
    if (error) {
        m_scheduler_socket.close();
        return false;
    }

    m_scheduler_socket.write_some(boost::asio::buffer(serialized_advertisement), error);
    if (error) {
        m_scheduler_socket.close();
        return false;
    }
    return true;
}

void ServerContext::stop_event_loop() {
    m_tcp_acceptor.cancel();
    m_scheduler_socket.close();
}
}  // namespace reducer
