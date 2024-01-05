#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>
#include <chrono>
#include <cstdio>
#include <ctime>
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
#include <set>

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/program_options.hpp>
#include <fmt/core.h>
#include <json/single_include/nlohmann/json.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../clp/MySQLDB.hpp"
#include "../clp/MySQLPreparedStatement.hpp"
#include "../clp/spdlog_with_specializations.hpp"
#include "../clp/type_utils.hpp"
#include "CountOperator.hpp"
#include "Pipeline.hpp"
#include "RecordGroupSerdes.hpp"

using boost::asio::ip::tcp;

namespace po = boost::program_options;

namespace reducer {

enum class ServerStatus {
    IDLE,
    RUNNING,
    FINISHING_SUCCESS,
    FINISHING_REDUCER_ERROR,
    FINISHING_REMOTE_ERROR,
    FINISHING_CANCELLED
};

std::string server_status_to_string(ServerStatus status) {
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

// This enum is a hidden binding between the python
// scheduler and this c++ reducer
enum class JobStatus : int {
    PENDING = 0,
    RUNNING,
    DONE,
    SUCCESS,
    SUCCESS_WITH_ERRORS,
    FAILED,
    CANCELLING,
    CANCELLED,
    PENDING_REDUCER,
    REDUCER_READY,
    PENDING_REDUCER_DONE,
    NO_MATCHING_ARCHIVE,
    WAITING_FOR_BATCH
};

class ServerContext {
public:
    boost::asio::io_context ioctx;
    boost::asio::ip::tcp::acceptor acceptor;
    std::unique_ptr<Pipeline> pipeline;
    clp::MySQLDB db;  // TODO: consider switching to boost asio mysql connector
    ServerStatus status;
    int64_t job_id;
    int64_t port;
    mongocxx::client mongodb_client;
    mongocxx::database results_database;
    mongocxx::collection results_collection;
    mongocxx::collection jobs_metric_collection;
    std::string host;
    std::string mongodb_collection;
    std::string mongodb_jobs_metric_collection;
    int polling_interval_ms;
    bool timeline_aggregation;
    std::set<GroupTags> updated_tags;

    // TODO: We should use tcp::v6 and set ip::v6_only to false. But this isn't
    // guaranteed to work, so for now, we use v4 to be safe.
    ServerContext(
            std::string& host,
            int port,
            std::string& mongodb_collection,
            std::string& mongodb_jobs_metric_collection,
            int polling_interval_ms
    )
            : acceptor(ioctx, tcp::endpoint(tcp::v4(), port)),
              pipeline(nullptr),
              status(ServerStatus::IDLE),
              job_id(-1),
              port(port),
              host(std::move(host)),
              mongodb_collection(std::move(mongodb_collection)),
              mongodb_jobs_metric_collection(std::move(mongodb_jobs_metric_collection)),
              polling_interval_ms(polling_interval_ms),
              timeline_aggregation(false) {}

    void initialize_database_statements() {
        m_take_search_job = std::make_unique<clp::MySQLPreparedStatement>(
                db.prepare_statement(cTakeSearchJobStatement, strlen(cTakeSearchJobStatement))
        );
        m_update_job_status = std::make_unique<clp::MySQLPreparedStatement>(
                db.prepare_statement(cUpdateJobStatusStatement, strlen(cUpdateJobStatusStatement))
        );
        m_get_new_jobs = (fmt::format(
                cGetNewJobs,
                clp::enum_to_underlying_type(JobStatus::PENDING_REDUCER)
        ));
    }

    ServerStatus execute_assign_new_job() {
        if (false == db.execute_query(m_get_new_jobs)) {
            return ServerStatus::FINISHING_REDUCER_ERROR;
        }

        auto it = db.get_iterator();
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
            bindings.bind_int64(1, port);
            bindings.bind_varchar(2, host.c_str(), host.length());
            bindings.bind_int64(3, pending_reducer);
            bindings.bind_int64(4, job.first);
            if (false == m_take_search_job->execute()) {
                return ServerStatus::FINISHING_REDUCER_ERROR;
            }

            if (m_take_search_job->get_affected_rows() > 0) {
                SPDLOG_INFO("Taking job {}", job.first);
                job_id = job.first;

                msgpack::object_handle handle
                        = msgpack::unpack(job.second.data(), job.second.length());

                // TODO: create a more robust method to specify reducer jobs
                // and create pipelines
                std::map<std::string, msgpack::type::variant> query_config = handle.get().convert();
                if (query_config.count("bucket_size")) {
                    timeline_aggregation = true;
                } else {
                    timeline_aggregation = false;
                }

                // For now all pipelines only perform count
                pipeline = std::make_unique<Pipeline>(PipelineInputMode::INTRA_STAGE);
                pipeline->add_pipeline_stage(std::shared_ptr<Operator>(new CountOperator()));

                // TODO: Inefficient to convert job_id back to a string since it was
                // a string in the first place. Might be negligible compared to
                // storing it as a string in addition to as an int.
                std::string collection_name = std::to_string(job.first);
                results_collection = mongocxx::collection(results_database[collection_name]);

                return ServerStatus::RUNNING;
            }
        }
        return ServerStatus::IDLE;
    }

    bool execute_update_job_status(JobStatus new_status) {
        clp::MySQLParamBindings& bindings = m_update_job_status->get_statement_bindings();
        int64_t new_status_int64 = clp::enum_to_underlying_type(new_status);
        bindings.bind_int64(0, new_status_int64);
        bindings.bind_int64(1, job_id);

        return m_update_job_status->execute();
    }

    ServerStatus execute_poll_job_done() {
        if (false == db.execute_query(fmt::format(cPollJobDone, job_id))) {
            return ServerStatus::FINISHING_REDUCER_ERROR;
        }

        auto it = db.get_iterator();
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
                || JobStatus::CANCELLING == job_status
                || JobStatus::WAITING_FOR_BATCH == job_status))
        {
            return ServerStatus::FINISHING_REMOTE_ERROR;
        }

        return ServerStatus::RUNNING;
    }

    void reset() {
        ioctx.reset();
        pipeline.reset(nullptr);
        status = ServerStatus::IDLE;
        job_id = -1;
        timeline_aggregation = false;
        updated_tags.clear();
    }

private:
    static constexpr char const cGetNewJobs[]
            = "SELECT id, search_config FROM distributed_search_jobs WHERE status={}";
    static constexpr char const cTakeSearchJobStatement[]
            = "UPDATE distributed_search_jobs SET status=?, reducer_port=?, reducer_host=? WHERE "
              "status=? and id=?";
    static constexpr char const cUpdateJobStatusStatement[]
            = "UPDATE distributed_search_jobs SET status=? WHERE id=?";
    static constexpr char const cPollJobDone[]
            = "SELECT status FROM distributed_search_jobs WHERE id={}";
    std::string m_get_new_jobs;
    std::unique_ptr<clp::MySQLPreparedStatement> m_take_search_job;
    std::unique_ptr<clp::MySQLPreparedStatement> m_update_job_status;
};

struct RecordReceiverContext {
    std::shared_ptr<ServerContext> ctx;
    tcp::socket socket;
    char* buf;
    size_t buf_size;
    size_t bytes_occupied;

    RecordReceiverContext(std::shared_ptr<ServerContext> ctx)
            : ctx(std::move(ctx)),
              socket(ctx->ioctx) {
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
    if (0 == bytes_remaining || ServerStatus::RUNNING != rctx->ctx->status) {
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
            if (rctx->ctx->timeline_aggregation) {
                rctx->ctx->updated_tags.insert(record_group.get_tags());
            }
            rctx->ctx->pipeline->push_record_group(record_group);
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
    if (0 == bytes_remaining || error.failed() || ServerStatus::RUNNING != rctx->ctx->status) {
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
        if (job_id != rctx->ctx->job_id) {
            SPDLOG_ERROR(
                    "Rejecting connection from worker with job_id={} during processing of "
                    "job_id={}",
                    job_id,
                    rctx->ctx->job_id
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
    if (false == error.failed() && ServerStatus::RUNNING == ctx->status) {
        queue_validate_sender_task(std::move(rctx));
        queue_accept_task(std::move(ctx));
    } else if (error.failed() && boost::system::errc::operation_canceled == error.value()) {
        SPDLOG_INFO("Accept task cancelled");
    } else if (error.failed()) {
        // tcp acceptor socket was closed -- don't re-queue accept task
        SPDLOG_ERROR("TCP acceptor socket closed");
        if (ctx->acceptor.is_open()) {
            ctx->acceptor.close();
        }
    } else {
        SPDLOG_WARN(
                "Rejecting connection while not in RUNNING state, state={}",
                server_status_to_string(ctx->status)
        );
        queue_accept_task(std::move(ctx));
    }
}

void queue_accept_task(std::shared_ptr<ServerContext> ctx) {
    auto rctx = RecordReceiverContext::NewReceiver(ctx);

    ctx->acceptor.async_accept(
            rctx->socket,
            boost::bind(
                    accept_task,
                    boost::asio::placeholders::error,
                    std::move(ctx),
                    std::move(rctx)
            )
    );
}

bool set_reducer_done_metrics(std::shared_ptr<ServerContext> ctx, JobStatus finish_status) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = now.time_since_epoch();
    double timestamp_seconds = seconds.count();

    ctx->jobs_metric_collection
            = mongocxx::collection(ctx->results_database[ctx->mongodb_jobs_metric_collection]);
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
            SPDLOG_ERROR("Unexpected done status: {}", finish_status);
            return false;
    }

    bsoncxx::builder::stream::document filter_builder;
    filter_builder << "job_id" << ctx->job_id;
    bsoncxx::document::value filter = filter_builder << bsoncxx::builder::stream::finalize;
    bsoncxx::builder::stream::document update_builder;
    update_builder << "$set" << bsoncxx::builder::stream::open_document << "status" << status_string
                   << "reducer_end_time" << timestamp_seconds
                   << bsoncxx::builder::stream::close_document;
    bsoncxx::document::value update = update_builder << bsoncxx::builder::stream::finalize;

    auto result = ctx->jobs_metric_collection.update_one(filter.view(), update.view());
    if (result) {
        if (result->modified_count() == 0) {
            SPDLOG_ERROR("No matching metrics document found for the given filter.");
        }
    } else {
        SPDLOG_ERROR("Failed to update metrics document.");
    }

    return true;
}

ServerStatus upsert_timeline_results(std::shared_ptr<ServerContext> ctx) {
    if (ctx->updated_tags.empty()) {
        return ServerStatus::RUNNING;
    }

    auto bulk_write = ctx->results_collection.create_bulk_write();

    bool any_updates = false;
    std::vector<std::vector<uint8_t>> results;
    for (auto group_it = ctx->pipeline->finish(ctx->updated_tags); !group_it->done();
         group_it->next())
    {
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
            ctx->updated_tags.clear();
        }
    } catch (mongocxx::bulk_write_exception const& e) {
        SPDLOG_ERROR("MongoDB bulk write exception during upsert: {}", e.what());
        return ServerStatus::FINISHING_REDUCER_ERROR;
    }
    return ServerStatus::RUNNING;
}

void poll_db(
        boost::system::error_code const& e,
        std::shared_ptr<ServerContext> ctx,
        boost::asio::steady_timer* poll_timer
) {
    if (ServerStatus::IDLE == ctx->status) {
        ctx->status = ctx->execute_assign_new_job();
    } else if (ServerStatus::RUNNING == ctx->status) {
        ctx->status = ctx->execute_poll_job_done();
    }

    if (ServerStatus::RUNNING == ctx->status && ctx->timeline_aggregation) {
        ctx->status = upsert_timeline_results(ctx);
    }

    if (ServerStatus::IDLE == ctx->status || ServerStatus::RUNNING == ctx->status) {
        poll_timer->expires_at(
                poll_timer->expiry() + boost::asio::chrono::milliseconds(ctx->polling_interval_ms)
        );
        poll_timer->async_wait(
                boost::bind(poll_db, boost::asio::placeholders::error, ctx, poll_timer)
        );
    } else {
        SPDLOG_INFO(
                "Cancelling operations on acceptor socket in state {}",
                server_status_to_string(ctx->status)
        );
        ctx->acceptor.cancel();
    }
}

std::shared_ptr<ServerContext> initialize_server_context(int argc, char const* argv[]) {
    std::string reducer_host, db_host, db_user, db_pass, database, mongodb_database, mongodb_uri,
            mongodb_collection, mongodb_jobs_metric_collection;
    int reducer_port, db_port;
    int polling_interval_ms;

    po::options_description arguments("Arguments");
    arguments.add_options()
        ("host", po::value<std::string>(&reducer_host)->default_value("127.0.0.1"))
        ("port", po::value<int>(&reducer_port)->default_value(14'009))
        ("db-host", po::value<std::string>(&db_host)->default_value("127.0.0.1"))
        ("db-port", po::value<int>(&db_port)->default_value(3306))
        ("db-user", po::value<std::string>(&db_user)->default_value("clp-user"))
        ("db-pass", po::value<std::string>(&db_pass)->default_value("password"))
        ("database", po::value<std::string>(&database)->default_value("clp-db"))
        ("mongodb-database", po::value<std::string>(&mongodb_database)->default_value("clp-search"))
        ("mongodb-uri", po::value<std::string>(&mongodb_uri)->default_value("mongodb://localhost:27017/"))
        ("mongodb-collection", po::value<std::string>(&mongodb_collection)->default_value("results"))
        ("mongodb-jobs-metric-collection", po::value<std::string>(&mongodb_jobs_metric_collection)->default_value("search_jobs_metrics"))
        ("polling-interval-ms", po::value<int>(&polling_interval_ms)->default_value(100))
        ;

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, arguments), opts);

    try {
        po::notify(opts);
    } catch (std::exception& e) {
        SPDLOG_ERROR("Failed to parse command line options error={}", e.what());
        return nullptr;
    }

    auto ctx = std::make_shared<ServerContext>(
            reducer_host,
            reducer_port,
            mongodb_collection,
            mongodb_jobs_metric_collection,
            polling_interval_ms
    );

    try {
        ctx->db.open(db_host, db_port, db_user, db_pass, database);
    } catch (clp::MySQLDB::OperationFailed& e) {
        SPDLOG_ERROR("Failed to connect to MySQL database error=", e.what());
        return nullptr;
    }
    ctx->initialize_database_statements();

    try {
        ctx->mongodb_client = mongocxx::client(mongocxx::uri(mongodb_uri));
        ctx->results_database = mongocxx::database(ctx->mongodb_client[mongodb_database]);
    } catch (mongocxx::exception& e) {
        SPDLOG_ERROR("Failed to connect to MongoDB database error=", e.what());
        return nullptr;
    }

    return ctx;
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

    std::shared_ptr<reducer::ServerContext> ctx = reducer::initialize_server_context(argc, argv);

    if (nullptr == ctx) {
        SPDLOG_ERROR("Failed to initialize reducer... exiting");
        return -1;
    }

    // Polling interval
    boost::asio::steady_timer poll_timer(
            ctx->ioctx,
            boost::asio::chrono::milliseconds(ctx->polling_interval_ms)
    );

    SPDLOG_INFO("Starting on host {} port {}", ctx->host, ctx->port);

    // Job acquisition loop
    while (true) {
        if (ctx->acceptor.is_open()) {
            SPDLOG_INFO("Acceptor socket listening successfully");
        } else {
            SPDLOG_ERROR("Failed to bind acceptor socket... exiting");
            return -1;
        }

        // Queue up polling and tcp accepting
        poll_timer.async_wait(
                boost::bind(reducer::poll_db, boost::asio::placeholders::error, ctx, &poll_timer)
        );
        reducer::queue_accept_task(ctx);

        SPDLOG_INFO("Waiting for job...");

        // Start running the event processing loop
        ctx->ioctx.run();

        if (reducer::ServerStatus::FINISHING_SUCCESS == ctx->status) {
            SPDLOG_INFO("Job {} finished successfully", ctx->job_id);

            if (ctx->timeline_aggregation) {
                reducer::upsert_timeline_results(ctx);
            } else {
                std::vector<std::vector<uint8_t>> results;
                std::vector<bsoncxx::document::view> result_documents;
                for (auto group_it = ctx->pipeline->finish(); !group_it->done(); group_it->next()) {
                    if (ctx->timeline_aggregation) {
                        results.push_back(serialize_timeline(*group_it->get()));
                    } else {
                        results.push_back(serialize(*group_it->get(), nlohmann::json::to_bson));
                    }
                    std::vector<uint8_t>& encoded_result = results.back();
                    result_documents.push_back(
                            bsoncxx::document::view(encoded_result.data(), encoded_result.size())
                    );
                }

                if (result_documents.size() > 0) {
                    ctx->results_collection.insert_many(result_documents);
                }
            }

            bool done_success = ctx->execute_update_job_status(reducer::JobStatus::SUCCESS);
            bool metrics_done_success = set_reducer_done_metrics(ctx, reducer::JobStatus::SUCCESS);
            if (false == done_success || false == metrics_done_success) {
                SPDLOG_ERROR("Database operation failed... exiting");
                return -1;
            }
        } else {
            SPDLOG_INFO("Job {} finished unsuccesfully", ctx->job_id);
            bool done_success = true, metrics_done_success = true;
            if (reducer::ServerStatus::FINISHING_REDUCER_ERROR == ctx->status) {
                done_success = ctx->execute_update_job_status(reducer::JobStatus::FAILED);
                metrics_done_success = set_reducer_done_metrics(ctx, reducer::JobStatus::FAILED);
            } else if (reducer::ServerStatus::FINISHING_CANCELLED == ctx->status) {
                metrics_done_success = set_reducer_done_metrics(ctx, reducer::JobStatus::CANCELLED);
            } else if (reducer::ServerStatus::FINISHING_REMOTE_ERROR == ctx->status) {
                metrics_done_success = set_reducer_done_metrics(ctx, reducer::JobStatus::FAILED);
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
