#include "ServerContext.hpp"

#include <chrono>

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

#include "../clp/MySQLDB.hpp"
#include "../clp/MySQLPreparedStatement.hpp"
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
          m_reducer_host(args.get_reducer_host()),
          m_reducer_port(args.get_reducer_port()),
          m_mongodb_job_metrics_collection(args.get_mongodb_jobs_metric_collection()),
          m_polling_interval_ms(args.get_polling_interval()),
          m_pipeline(nullptr),
          m_status(ServerStatus::Idle),
          m_job_id(-1),
          m_timeline_aggregation(false) {
    try {
        m_db.open(
                args.get_db_host(),
                args.get_db_port(),
                args.get_db_user(),
                args.get_db_password(),
                args.get_db_database()
        );
    } catch (clp::MySQLDB::OperationFailed& e) {
        SPDLOG_ERROR(
                "Failed to connect to {}:{} - {}",
                args.get_db_host(),
                args.get_db_port(),
                e.what()
        );
        throw OperationFailed(e.get_error_code(), __FILENAME__, __LINE__);
    }
    std::string take_search_job_stmt
            = fmt::format(cTakeSearchJobStatement, args.get_db_jobs_table());
    m_take_search_job_stmt = std::make_unique<clp::MySQLPreparedStatement>(
            m_db.prepare_statement(take_search_job_stmt.data(), take_search_job_stmt.length())
    );
    std::string update_job_status_stmt
            = fmt::format(cUpdateJobStatusStatement, args.get_db_jobs_table());
    m_update_job_status_stmt = std::make_unique<clp::MySQLPreparedStatement>(
            m_db.prepare_statement(update_job_status_stmt.data(), update_job_status_stmt.length())
    );
    m_get_new_jobs_sql = fmt::format(
            cGetNewJobs,
            args.get_db_jobs_table(),
            clp::enum_to_underlying_type(JobStatus::PendingReducer)
    );
    m_poll_job_done_sql = fmt::format(cPollJobDone, args.get_db_jobs_table(), "{}");

    try {
        m_mongodb_client = mongocxx::client(mongocxx::uri(args.get_mongodb_uri()));
    } catch (mongocxx::exception& e) {
        SPDLOG_ERROR("Failed to connect to {} - {}", args.get_mongodb_uri(), e.what());
        throw OperationFailed(clp::ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_mongodb_results_database = mongocxx::database(m_mongodb_client[args.get_mongodb_database()]);
}

ServerStatus ServerContext::take_job() {
    if (false == m_db.execute_query(m_get_new_jobs_sql)) {
        return ServerStatus::FinishingReducerError;
    }

    auto it = m_db.get_iterator();
    std::string job_id_str, search_config;
    std::vector<std::pair<int64_t, std::string>> available_jobs;
    while (it.contains_element()) {
        it.get_field_as_string(0, job_id_str);
        it.get_field_as_string(1, search_config);
        int64_t job_id;
        if (!clp::string_utils::convert_string_to_int(job_id_str, job_id)) {
            return ServerStatus::FinishingReducerError;
        }
        available_jobs.emplace_back(job_id, std::move(search_config));
        it.get_next();
    }

    for (auto& job : available_jobs) {
        clp::MySQLParamBindings& bindings = m_take_search_job_stmt->get_statement_bindings();
        int64_t reducer_ready = clp::enum_to_underlying_type(JobStatus::ReducerReady);
        int64_t pending_reducer = clp::enum_to_underlying_type(JobStatus::PendingReducer);
        bindings.bind_int64(0, reducer_ready);
        bindings.bind_int(1, m_reducer_port);
        bindings.bind_varchar(2, m_reducer_host.c_str(), m_reducer_host.length());
        bindings.bind_int64(3, pending_reducer);
        bindings.bind_int64(4, job.first);
        if (false == m_take_search_job_stmt->execute()) {
            return ServerStatus::FinishingReducerError;
        }

        uint64_t num_affected_rows;
        if (false == m_take_search_job_stmt->get_affected_rows(num_affected_rows)) {
            return ServerStatus::FinishingReducerError;
        }

        if (num_affected_rows > 0) {
            SPDLOG_INFO("Taking job {}", job.first);
            m_job_id = job.first;

            msgpack::object_handle handle = msgpack::unpack(job.second.data(), job.second.length());

            // TODO: create a more robust method to specify reducer jobs and create pipelines
            std::map<std::string, msgpack::type::variant> query_config = handle.get().convert();
            if (query_config.count(JobAttributes::cBucketSize)) {
                m_timeline_aggregation = true;
            } else {
                m_timeline_aggregation = false;
            }

            // TODO: For now all pipelines only perform count. We will need to implement more
            // general pipeline initialization once more operators are implemented.
            m_pipeline = std::make_unique<Pipeline>(PipelineInputMode::INTRA_STAGE);
            m_pipeline->add_pipeline_stage(std::make_shared<CountOperator>());

            // TODO: Inefficient to convert job_id back to a string since it was a string in the
            // first place. Might be negligible compared to storing it as a string in addition to as
            // an int.
            auto collection_name = std::to_string(job.first);
            m_mongodb_results_collection = m_mongodb_results_database[collection_name];

            return ServerStatus::Running;
        }
    }
    return ServerStatus::Idle;
}

bool ServerContext::update_job_status(JobStatus new_status) {
    clp::MySQLParamBindings& bindings = m_update_job_status_stmt->get_statement_bindings();
    int64_t new_status_int64 = clp::enum_to_underlying_type(new_status);
    bindings.bind_int64(0, new_status_int64);
    bindings.bind_int64(1, m_job_id);

    return m_update_job_status_stmt->execute();
}

ServerStatus ServerContext::poll_job_done() {
    if (false == m_db.execute_query(fmt::format(m_poll_job_done_sql, m_job_id))) {
        return ServerStatus::FinishingReducerError;
    }

    auto it = m_db.get_iterator();
    std::string job_status_str;
    JobStatus job_status;
    while (it.contains_element()) {
        it.get_field_as_string(0, job_status_str);
        job_status = static_cast<JobStatus>(stoi(job_status_str));
        it.get_next();
    }

    if (JobStatus::PendingReducerDone == job_status) {
        return ServerStatus::FinishingSuccess;
    }

    if (false
        == (JobStatus::Running == job_status || JobStatus::ReducerReady == job_status
            || JobStatus::Cancelling == job_status))
    {
        return ServerStatus::FinishingRemoteError;
    }

    return ServerStatus::Running;
}

bool ServerContext::publish_reducer_job_metrics(JobStatus finish_status) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = now.time_since_epoch();
    double timestamp_seconds = seconds.count();

    auto metrics_collection
            = mongocxx::collection(m_mongodb_results_database[m_mongodb_job_metrics_collection]);
    std::string status_string;
    switch (finish_status) {
        case JobStatus::Success:
            status_string = "success";
            break;
        case JobStatus::Failed:
            status_string = "failed";
            break;
        case JobStatus::Cancelled:
            status_string = "cancelled";
            break;
        default:
            SPDLOG_ERROR("Unexpected done status: {}", static_cast<int>(finish_status));
            return false;
    }

    bsoncxx::builder::stream::document filter_builder;
    filter_builder << "job_id" << m_job_id;
    bsoncxx::document::value filter = filter_builder << bsoncxx::builder::stream::finalize;
    bsoncxx::builder::stream::document update_builder;
    update_builder << "$set" << bsoncxx::builder::stream::open_document << "status" << status_string
                   << "reducer_end_time" << timestamp_seconds
                   << bsoncxx::builder::stream::close_document;
    bsoncxx::document::value update = update_builder << bsoncxx::builder::stream::finalize;

    try {
        auto result = metrics_collection.update_one(filter.view(), update.view());
        if (result) {
            if (result->modified_count() == 0) {
                SPDLOG_ERROR("No matching metrics document found for the given filter.");
            }
        } else {
            SPDLOG_ERROR("Failed to update metrics document.");
        }
    } catch (mongocxx::bulk_write_exception const& e) {
        SPDLOG_ERROR("MongoDB bulk write exception during metrics update: {}", e.what());
        return false;
    }

    return true;
}

ServerStatus ServerContext::upsert_timeline_results() {
    if (m_updated_tags.empty()) {
        return ServerStatus::Running;
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
        return ServerStatus::FinishingReducerError;
    }
    return ServerStatus::Running;
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

void ServerContext::reset() {
    m_ioctx.reset();
    m_pipeline.reset(nullptr);
    m_status = ServerStatus::Idle;
    m_job_id = -1;
    m_timeline_aggregation = false;
    m_updated_tags.clear();
}

}  // namespace reducer
