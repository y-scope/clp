#include "reducer_server.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <chrono>
#include <map>
#include <mongocxx/bulk_write.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/model/replace_one.hpp>
#include <mongocxx/uri.hpp>
#include <msgpack.hpp>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <fmt/core.h>
#include <json/single_include/nlohmann/json.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../clp/MySQLDB.hpp"
#include "../clp/MySQLPreparedStatement.hpp"
#include "../clp/spdlog_with_specializations.hpp"
#include "../clp/type_utils.hpp"
#include "CommandLineArguments.hpp"
#include "CountOperator.hpp"
#include "Pipeline.hpp"
#include "RecordGroupSerdes.hpp"

using boost::asio::ip::tcp;

namespace reducer {

static std::string server_status_to_string(ServerStatus status) {
    switch (status) {
        case ServerStatus::IDLE:
            return "IDLE";
        case ServerStatus::RUNNING:
            return "RUNNING";
        case ServerStatus::FINISHING_SUCCESS:
            return "FINISHING_SUCCESS";
        case ServerStatus::FINISHING_REDUCER_ERROR:
            return "FINISHING_REDUCER_ERROR";
        case ServerStatus::FINISHING_REMOTE_ERROR:
            return "FINISHING_REMOTE_ERROR";
        case ServerStatus::FINISHING_CANCELLED:
            return "FINISHING_CANCELLED";
        default:
            assert(0);
    }
    return "";
}

// TODO: We should use tcp::v6 and set ip::v6_only to false. But this isn't
// guaranteed to work, so for now, we use v4 to be safe.
ServerContext::ServerContext(CommandLineArguments& args)
        : m_tcp_acceptor(m_ioctx, tcp::endpoint(tcp::v4(), args.get_reducer_port())),
          m_reducer_host(args.get_reducer_host()),
          m_reducer_port(args.get_reducer_port()),
          m_mongodb_job_metrics_collection(args.get_mongodb_jobs_metric_collection()),
          m_polling_interval_ms(args.get_polling_interval()),
          m_pipeline(nullptr),
          m_status(ServerStatus::IDLE),
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
        SPDLOG_ERROR("Failed to connect to MySQL database error=", e.what());
        throw OperationFailed(clp::ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_take_search_job = std::make_unique<clp::MySQLPreparedStatement>(
            m_db.prepare_statement(cTakeSearchJobStatement, strlen(cTakeSearchJobStatement))
    );
    m_update_job_status = std::make_unique<clp::MySQLPreparedStatement>(
            m_db.prepare_statement(cUpdateJobStatusStatement, strlen(cUpdateJobStatusStatement))
    );
    m_get_new_jobs
            = (fmt::format(cGetNewJobs, clp::enum_to_underlying_type(JobStatus::PENDING_REDUCER)));

    try {
        m_mongodb_client = mongocxx::client(mongocxx::uri(args.get_mongodb_uri()));
        m_mongodb_results_database
                = mongocxx::database(m_mongodb_client[args.get_mongodb_database()]);
    } catch (mongocxx::exception& e) {
        SPDLOG_ERROR("Failed to connect to MongoDB database error=", e.what());
        throw OperationFailed(clp::ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
}

ServerStatus ServerContext::execute_assign_new_job() {
    if (false == m_db.execute_query(m_get_new_jobs)) {
        return ServerStatus::FINISHING_REDUCER_ERROR;
    }

    auto it = m_db.get_iterator();
    std::string job_id_str, search_config;
    std::vector<std::pair<int64_t, std::string>> available_jobs;
    while (it.contains_element()) {
        it.get_field_as_string(0, job_id_str);
        it.get_field_as_string(1, search_config);
        available_jobs.push_back({stoi(job_id_str), std::move(search_config)});
        it.get_next();
    }

    for (auto& job : available_jobs) {
        clp::MySQLParamBindings& bindings = m_take_search_job->get_statement_bindings();
        int64_t reducer_ready = clp::enum_to_underlying_type(JobStatus::REDUCER_READY);
        int64_t pending_reducer = clp::enum_to_underlying_type(JobStatus::PENDING_REDUCER);
        bindings.bind_int64(0, reducer_ready);
        bindings.bind_int64(1, m_reducer_port);
        bindings.bind_varchar(2, m_reducer_host.c_str(), m_reducer_host.length());
        bindings.bind_int64(3, pending_reducer);
        bindings.bind_int64(4, job.first);
        if (false == m_take_search_job->execute()) {
            return ServerStatus::FINISHING_REDUCER_ERROR;
        }

        if (m_take_search_job->get_affected_rows() > 0) {
            SPDLOG_INFO("Taking job {}", job.first);
            m_job_id = job.first;

            msgpack::object_handle handle = msgpack::unpack(job.second.data(), job.second.length());

            // TODO: create a more robust method to specify reducer jobs
            // and create pipelines
            std::map<std::string, msgpack::type::variant> query_config = handle.get().convert();
            if (query_config.count("bucket_size")) {
                m_timeline_aggregation = true;
            } else {
                m_timeline_aggregation = false;
            }

            // For now all pipelines only perform count
            m_pipeline = std::make_unique<Pipeline>(PipelineInputMode::INTRA_STAGE);
            m_pipeline->add_pipeline_stage(std::shared_ptr<Operator>(new CountOperator()));

            // TODO: Inefficient to convert job_id back to a string since it was
            // a string in the first place. Might be negligible compared to
            // storing it as a string in addition to as an int.
            std::string collection_name = std::to_string(job.first);
            m_mongodb_results_collection
                    = mongocxx::collection(m_mongodb_results_database[collection_name]);

            return ServerStatus::RUNNING;
        }
    }
    return ServerStatus::IDLE;
}

bool ServerContext::execute_update_job_status(JobStatus new_status) {
    clp::MySQLParamBindings& bindings = m_update_job_status->get_statement_bindings();
    int64_t new_status_int64 = clp::enum_to_underlying_type(new_status);
    bindings.bind_int64(0, new_status_int64);
    bindings.bind_int64(1, m_job_id);

    return m_update_job_status->execute();
}

ServerStatus ServerContext::execute_poll_job_done() {
    if (false == m_db.execute_query(fmt::format(cPollJobDone, m_job_id))) {
        return ServerStatus::FINISHING_REDUCER_ERROR;
    }

    auto it = m_db.get_iterator();
    std::string job_status_str;
    JobStatus job_status;
    while (it.contains_element()) {
        it.get_field_as_string(0, job_status_str);
        job_status = static_cast<JobStatus>(stoi(job_status_str));
        it.get_next();
    }

    if (JobStatus::PENDING_REDUCER_DONE == job_status) {
        return ServerStatus::FINISHING_SUCCESS;
    }

    if (false
        == (JobStatus::RUNNING == job_status || JobStatus::REDUCER_READY == job_status
            || JobStatus::CANCELLING == job_status || JobStatus::WAITING_FOR_BATCH == job_status))
    {
        return ServerStatus::FINISHING_REMOTE_ERROR;
    }

    return ServerStatus::RUNNING;
}

bool ServerContext::publish_reducer_job_metrics(JobStatus finish_status) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = now.time_since_epoch();
    double timestamp_seconds = seconds.count();

    auto metrics_collection
            = mongocxx::collection(m_mongodb_results_database[m_mongodb_job_metrics_collection]);
    std::string status_string;
    switch (finish_status) {
        case JobStatus::SUCCESS:
            status_string = "success";
            break;
        case JobStatus::FAILED:
            status_string = "failed";
            break;
        case JobStatus::CANCELLED:
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
        return ServerStatus::RUNNING;
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
        return ServerStatus::FINISHING_REDUCER_ERROR;
    }
    return ServerStatus::RUNNING;
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
    m_status = ServerStatus::IDLE;
    m_job_id = -1;
    m_timeline_aggregation = false;
    m_updated_tags.clear();
}

struct RecordReceiverContext {
    std::shared_ptr<ServerContext> ctx;
    tcp::socket socket;
    char* buf;
    size_t buf_size;
    size_t bytes_occupied;

    RecordReceiverContext(std::shared_ptr<ServerContext> ctx)
            : ctx(ctx),
              socket(ctx->get_io_context()) {
        buf_size = 1024;
        bytes_occupied = 0;
        buf = new char[buf_size];
    }

    ~RecordReceiverContext() {
        delete buf;
        socket.close();
    }

    static std::shared_ptr<RecordReceiverContext> NewReceiver(std::shared_ptr<ServerContext> ctx) {
        // Note: we use shared instead of unique to make things work with boost::bind
        std::shared_ptr<RecordReceiverContext> receiver
                = std::make_shared<RecordReceiverContext>(ctx);
        // clear the v6_only flag to allow ipv4 and ipv6 connections
        // note this is linux-only -- for full portability need separate v4 and v6 acceptors
        // boost::asio::ip::v6_only option(false);
        // receiver->socket.set_option(option);

        return receiver;
    }
};

void accept_task(
        boost::system::error_code const& error,
        std::shared_ptr<ServerContext> ctx,
        std::shared_ptr<RecordReceiverContext> rctx
);
void queue_accept_task(std::shared_ptr<ServerContext> ctx);

void receive_task(
        boost::system::error_code const& error,
        size_t bytes_remaining,
        std::shared_ptr<RecordReceiverContext> rctx
);
void queue_receive_task(std::shared_ptr<RecordReceiverContext> rctx);

void validate_sender_task(
        boost::system::error_code const& error,
        size_t bytes_remaining,
        std::shared_ptr<RecordReceiverContext> rctx
);
void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> rctx);

void receive_task(
        boost::system::error_code const& error,
        size_t bytes_remaining,
        std::shared_ptr<RecordReceiverContext> rctx
) {
    size_t record_size = 0;
    char* buf_ptr = rctx->buf;

    // if no new bytes terminate
    if (0 == bytes_remaining || ServerStatus::RUNNING != rctx->ctx->get_status()) {
        return;
    }

    // account for leftover bytes from previous call
    bytes_remaining += rctx->bytes_occupied;

    while (bytes_remaining > 0) {
        if (bytes_remaining >= sizeof(record_size)) {
            memcpy(&record_size, buf_ptr, sizeof(record_size));
        } else {
            break;
        }

        // terminate if record group size is over 16MB
        if (record_size >= 16 * 1024 * 1024) {
            SPDLOG_ERROR("Record too large: {}B", record_size);
            return;
        }

        if (bytes_remaining >= (record_size + sizeof(record_size))) {
            buf_ptr += sizeof(record_size);
            auto record_group = deserialize(buf_ptr, record_size);
            rctx->ctx->push_record_group(record_group);
            bytes_remaining -= (record_size + sizeof(record_size));
            buf_ptr += record_size;
        } else {
            break;
        }
    }

    if (bytes_remaining > 0) {
        if (rctx->buf_size < (record_size + sizeof(record_size))) {
            char* new_buf = new char[record_size + sizeof(record_size)];
            memcpy(new_buf, buf_ptr, bytes_remaining);
            delete rctx->buf;
            rctx->buf = new_buf;
            rctx->bytes_occupied = bytes_remaining;
        } else {
            memmove(rctx->buf, buf_ptr, bytes_remaining);
            rctx->bytes_occupied = bytes_remaining;
        }
    }

    // only queue another receive if the connection is still open
    if (false == error.failed()) {
        queue_receive_task(std::move(rctx));
    }
}

void queue_receive_task(std::shared_ptr<RecordReceiverContext> rctx) {
    boost::asio::async_read(
            rctx->socket,
            boost::asio::buffer(
                    &rctx->buf[rctx->bytes_occupied],
                    rctx->buf_size - rctx->bytes_occupied
            ),
            boost::bind(
                    receive_task,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    rctx
            )
    );
}

void validate_sender_task(
        boost::system::error_code const& error,
        size_t bytes_remaining,
        std::shared_ptr<RecordReceiverContext> rctx
) {
    // if no new bytes terminate
    if (0 == bytes_remaining || error.failed() || ServerStatus::RUNNING != rctx->ctx->get_status())
    {
        SPDLOG_ERROR("Rejecting connection because of connection error");
        return;
    }

    // account for leftover bytes from previous call
    bytes_remaining += rctx->bytes_occupied;

    int32_t job_id;
    if (bytes_remaining > sizeof(int32_t)) {
        SPDLOG_ERROR("Rejecting connection because of invalid negotiation");
        return;
    } else if (bytes_remaining == sizeof(int32_t)) {
        memcpy(&job_id, rctx->buf, sizeof(int32_t));
        if (job_id != rctx->ctx->get_job_id()) {
            SPDLOG_ERROR(
                    "Rejecting connection from worker with job_id={} during processing of "
                    "job_id={}",
                    job_id,
                    rctx->ctx->get_job_id()
            );
            return;
        }
        rctx->bytes_occupied = 0;
        char const response = 'y';
        boost::system::error_code e;
        int transferred = boost::asio::write(rctx->socket, boost::asio::buffer(&response, 1), e);
        if (e || transferred < sizeof(response)) {
            SPDLOG_ERROR("Rejecting connection because of connection error while attempting to "
                         "send acceptance");
            return;
        }

        queue_receive_task(std::move(rctx));
    } else {
        rctx->bytes_occupied = bytes_remaining;
        queue_validate_sender_task(std::move(rctx));
    }
}

void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> rctx) {
    boost::asio::async_read(
            rctx->socket,
            boost::asio::buffer(
                    &rctx->buf[rctx->bytes_occupied],
                    sizeof(int32_t) - rctx->bytes_occupied
            ),
            boost::bind(
                    validate_sender_task,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    rctx
            )
    );
}

void accept_task(
        boost::system::error_code const& error,
        std::shared_ptr<ServerContext> ctx,
        std::shared_ptr<RecordReceiverContext> rctx
) {
    if (false == error.failed() && ServerStatus::RUNNING == ctx->get_status()) {
        queue_validate_sender_task(std::move(rctx));
        queue_accept_task(std::move(ctx));
    } else if (error.failed() && boost::system::errc::operation_canceled == error.value()) {
        SPDLOG_INFO("Accept task cancelled");
    } else if (error.failed()) {
        // tcp acceptor socket was closed -- don't re-queue accept task
        SPDLOG_ERROR("TCP acceptor socket closed");
        if (ctx->get_tcp_acceptor().is_open()) {
            ctx->get_tcp_acceptor().close();
        }
    } else {
        SPDLOG_WARN(
                "Rejecting connection while not in RUNNING state, state={}",
                server_status_to_string(ctx->get_status())
        );
        queue_accept_task(std::move(ctx));
    }
}

void queue_accept_task(std::shared_ptr<ServerContext> ctx) {
    auto rctx = RecordReceiverContext::NewReceiver(ctx);

    ctx->get_tcp_acceptor().async_accept(
            rctx->socket,
            boost::bind(accept_task, boost::asio::placeholders::error, ctx, rctx)
    );
}

void poll_db(
        boost::system::error_code const& e,
        std::shared_ptr<ServerContext> ctx,
        boost::asio::steady_timer* poll_timer
) {
    if (ServerStatus::IDLE == ctx->get_status()) {
        ctx->set_status(ctx->execute_assign_new_job());
    } else if (ServerStatus::RUNNING == ctx->get_status()) {
        ctx->set_status(ctx->execute_poll_job_done());
    }

    if (ServerStatus::RUNNING == ctx->get_status() && ctx->is_timeline_aggregation()) {
        ctx->set_status(ctx->upsert_timeline_results());
    }

    if (ServerStatus::IDLE == ctx->get_status() || ServerStatus::RUNNING == ctx->get_status()) {
        poll_timer->expires_at(
                poll_timer->expiry()
                + boost::asio::chrono::milliseconds(ctx->get_polling_interval())
        );
        poll_timer->async_wait(
                boost::bind(poll_db, boost::asio::placeholders::error, ctx, poll_timer)
        );
    } else {
        SPDLOG_INFO(
                "Cancelling operations on acceptor socket in state {}",
                server_status_to_string(ctx->get_status())
        );
        ctx->get_tcp_acceptor().cancel();
    }
}
}  // namespace reducer

int main(int argc, char const* argv[]) {
    // Program-wide initialization
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S,%e [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return -1;
    }

    // mongocxx instance must be created before and destroyed after
    // all other mongocxx classes
    mongocxx::instance inst;

    reducer::CommandLineArguments args("reducer_server");
    auto parsing_result = args.parse_arguments(argc, argv);
    if (parsing_result != clp::CommandLineArgumentsBase::ParsingResult::Success) {
        SPDLOG_ERROR("Failed to parse arguments for reducer... exiting");
        return -1;
    }

    std::shared_ptr<reducer::ServerContext> ctx = nullptr;

    try {
        ctx = std::make_shared<reducer::ServerContext>(args);
    } catch (reducer::ServerContext::OperationFailed& exception) {
        SPDLOG_ERROR("Failed to initialize reducer on error {}... exiting", exception.what());
        return -1;
    }

    // Polling interval
    boost::asio::steady_timer polling_timer(
            ctx->get_io_context(),
            boost::asio::chrono::milliseconds(ctx->get_polling_interval())
    );

    SPDLOG_INFO("Starting on host {} port {}", ctx->get_reducer_host(), ctx->get_reducer_port());

    // Job acquisition loop
    while (true) {
        if (ctx->get_tcp_acceptor().is_open()) {
            SPDLOG_INFO("Acceptor socket listening successfully");
        } else {
            SPDLOG_ERROR("Failed to bind acceptor socket... exiting");
            return -1;
        }

        // Queue up polling and tcp accepting
        polling_timer.async_wait(
                boost::bind(reducer::poll_db, boost::asio::placeholders::error, ctx, &polling_timer)
        );
        reducer::queue_accept_task(ctx);

        SPDLOG_INFO("Waiting for job...");

        // Start running the event processing loop
        ctx->run();

        if (reducer::ServerStatus::FINISHING_SUCCESS == ctx->get_status()) {
            SPDLOG_INFO("Job {} finished successfully", ctx->get_job_id());

            bool results_success = false;
            if (ctx->is_timeline_aggregation()) {
                results_success = ctx->upsert_timeline_results()
                                  != reducer::ServerStatus::FINISHING_REDUCER_ERROR;
            } else {
                results_success = ctx->publish_pipeline_results();
            }

            bool done_success = ctx->execute_update_job_status(reducer::JobStatus::SUCCESS);
            bool metrics_done_success
                    = ctx->publish_reducer_job_metrics(reducer::JobStatus::SUCCESS);
            if (false == results_success || false == done_success || false == metrics_done_success)
            {
                SPDLOG_ERROR("Database operation failed... exiting");
                return -1;
            }
        } else {
            SPDLOG_INFO("Job {} finished unsuccesfully", ctx->get_job_id());
            bool done_success = true, metrics_done_success = true;
            if (reducer::ServerStatus::FINISHING_REDUCER_ERROR == ctx->get_status()) {
                done_success = ctx->execute_update_job_status(reducer::JobStatus::FAILED);
                metrics_done_success = ctx->publish_reducer_job_metrics(reducer::JobStatus::FAILED);
            } else if (reducer::ServerStatus::FINISHING_CANCELLED == ctx->get_status()) {
                metrics_done_success
                        = ctx->publish_reducer_job_metrics(reducer::JobStatus::CANCELLED);
            } else if (reducer::ServerStatus::FINISHING_REMOTE_ERROR == ctx->get_status()) {
                metrics_done_success = ctx->publish_reducer_job_metrics(reducer::JobStatus::FAILED);
            }

            if (false == done_success || false == metrics_done_success) {
                SPDLOG_ERROR("Database operation failed... exiting");
                return -1;
            }
        }

        // cleanup reducer state for next job
        ctx->reset();
    }
}
