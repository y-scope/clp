#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

//
#include <iostream>
#include <map>
#include <sstream>

//
// FIXME: boost also has a mysql connector library that works with asio we
// should consider switching to in the future
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/program_options.hpp>

//
#include <mariadb/mysql.h>

//
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

//
#include <msgpack.hpp>

//
#include "aggregation/CountOperator.hpp"
#include "aggregation/Pipeline.hpp"
#include "aggregation/RecordGroupSerdes.hpp"

using boost::asio::ip::tcp;

namespace po = boost::program_options;

enum class ServerStatus {
    IDLE,
    RUNNING,
    FINISHING_SUCCESS,
    FINISHING_ERROR,
    CANCELLING
};

std::ostream& operator<<(std::ostream& os, ServerStatus const& status) {
    os << static_cast<std::underlying_type<ServerStatus>::type>(status);
    return os;
}

/**
 * Status codes
 * PENDING 0
 * RUNNING 1
 * DONE 2
 * SUCCESS 3
 * SUCCESS_WITH_ERRORS 4
 * FAILED 5
 * CANCELLING 6
 * CANCELLED 7
 * PENDING_REDUCER 8
 * REDUCER_READY 9
 * PENDING_REDUCER_DONE 10
 */

struct ServerContext {
    boost::asio::io_context ioctx;
    boost::asio::ip::tcp::acceptor acceptor;
    Pipeline* p;
    MYSQL* db;
    ServerStatus status;
    int32_t job_id;
    int port;
    mongocxx::database results_database;
    mongocxx::collection results_collection;
    std::string host;
    std::string mongodb_collection;
    int polling_interval_ms;

    ServerContext(std::string& host, int port, std::string& mongodb_collection, int polling_interval_ms)
            : acceptor(ioctx, tcp::endpoint(tcp::v6(), port)),
              p(nullptr),
              db(nullptr),
              status(ServerStatus::IDLE),
              job_id(-1),
              port(port),
              host(std::move(host)),
              mongodb_collection(std::move(mongodb_collection)),
              polling_interval_ms(polling_interval_ms) {}
};

struct RecordReceiverContext {
    ServerContext* ctx;
    tcp::socket socket;
    char* buf;
    size_t buf_size;
    size_t bytes_occupied;

    RecordReceiverContext(ServerContext* ctx) : ctx(ctx), socket(ctx->ioctx) {
        buf_size = 1024;
        bytes_occupied = 0;
        buf = new char[buf_size];
    }

    ~RecordReceiverContext() {
        delete buf;
        socket.close();
    }

    static RecordReceiverContext* NewReceiver(ServerContext* ctx) {
        RecordReceiverContext* receiver = new RecordReceiverContext(ctx);
        // clear the v6_only flag to allow ipv4 and ipv6 connections
        // note this is linux-only -- for full portability need separate v4 and v6 acceptors
        // boost::asio::ip::v6_only option(false);
        // receiver->socket.set_option(option);

        return receiver;
    }
};

void accept_task(
        boost::system::error_code const& error,
        ServerContext* ctx,
        RecordReceiverContext* rctx
);
void queue_accept_task(ServerContext* ctx);

void receive_task(
        boost::system::error_code const& error,
        size_t bytes_remaining,
        RecordReceiverContext* rctx
);
void queue_receive_task(RecordReceiverContext* rctx);

void validate_sender_task(
        boost::system::error_code const& error,
        size_t bytes_remaining,
        RecordReceiverContext* rctx
);
void queue_validate_sender_task(RecordReceiverContext* rctx);

void receive_task(
        boost::system::error_code const& error,
        size_t bytes_remaining,
        RecordReceiverContext* rctx
) {
    size_t record_size = 0;
    char* buf_ptr = rctx->buf;

    // if no new bytes terminate
    if (bytes_remaining == 0
        || !(rctx->ctx->status == ServerStatus::RUNNING
             || rctx->ctx->status != ServerStatus::CANCELLING))
    {
        delete rctx;
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
            std::cout << "Record too large: " << record_size << "B" << std::endl;
            delete rctx;
            return;
        }

        if (bytes_remaining >= (record_size + sizeof(record_size))) {
            buf_ptr += sizeof(record_size);
            rctx->ctx->p->push_record_group(deserialize(buf_ptr, record_size));
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
    if (!error.failed()) {
        queue_receive_task(rctx);
    }
}

void queue_receive_task(RecordReceiverContext* rctx) {
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
        RecordReceiverContext* rctx
) {
    // if no new bytes terminate
    if (bytes_remaining == 0 || error.failed()
        || !(rctx->ctx->status == ServerStatus::RUNNING
             || rctx->ctx->status != ServerStatus::CANCELLING))
    {
        delete rctx;
        return;
    }

    // account for leftover bytes from previous call
    bytes_remaining += rctx->bytes_occupied;

    int32_t job_id;
    if (bytes_remaining > sizeof(int32_t)) {
        delete rctx;
        return;
    } else if (bytes_remaining == sizeof(int32_t)) {
        memcpy(&job_id, rctx->buf, sizeof(int32_t));
        if (job_id != rctx->ctx->job_id) {
            std::cout << "Rejecting connection from worker with job_id="
                      << job_id << " during processing of job_id="
                      << rctx->ctx->job_id << std::endl;
            delete rctx;
            return;
        }
        rctx->bytes_occupied = 0;
        char response = 'y';
        boost::system::error_code e;
        int transferred = boost::asio::write(rctx->socket, boost::asio::buffer(&response, 1), e);
        if (e || transferred < sizeof(response)) {
            delete rctx;
            return;
        }

        queue_receive_task(rctx);
    } else {
        rctx->bytes_occupied = bytes_remaining;
        queue_validate_sender_task(rctx);
    }
}

void queue_validate_sender_task(RecordReceiverContext *rctx) {
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

void print_result(RecordGroup const& group) {
    auto tags = group.get_tags();
    auto record = group.record_it();

    std::cout << "tags[";
    for (auto& tag : tags) {
        std::cout << tag << ",";
    }
    std::cout << "]" << std::endl;
    for (; !record->done(); record->next()) {
        std::cout << "Count: " << record->get()->get_int64_value("count") << std::endl;
    }
}

void accept_task(
        boost::system::error_code const& error,
        ServerContext* ctx,
        RecordReceiverContext* rctx
) {
    if (!error.failed() && ctx->status == ServerStatus::RUNNING) {
        queue_validate_sender_task(rctx);
        queue_accept_task(ctx);
    } else {
        // tcp acceptor socket was closed
        // clean up rctx and don't re-queue accept task
        delete rctx;
    }
}

void queue_accept_task(ServerContext* ctx) {
    auto rctx = RecordReceiverContext::NewReceiver(ctx);

    ctx->acceptor.async_accept(
            rctx->socket,
            boost::bind(accept_task, boost::asio::placeholders::error, ctx, rctx)
    );
}

// Taken from MySQL::Iterator in CLP
class SQLIterator {
public:
    // Constructors
    explicit SQLIterator(MYSQL* m_db_handle)
            : m_row(nullptr),
              m_field_lengths(nullptr),
              m_num_fields(0) {
        m_query_result = mysql_use_result(m_db_handle);
        fetch_next_row();
    }

    // Delete copy constructor and assignment
    SQLIterator(SQLIterator const&) = delete;
    SQLIterator& operator=(SQLIterator const&) = delete;

    // Destructors
    ~SQLIterator() {
        if (nullptr != m_query_result) {
            m_row = nullptr;
            m_field_lengths = nullptr;
            m_num_fields = 0;
            mysql_free_result(m_query_result);
            m_query_result = nullptr;
        }
    }

    // Methods
    bool contains_element() const { return nullptr != m_row; }

    void get_next() {
        if (contains_element()) {
            fetch_next_row();
        }
    }

    void get_field_as_string(size_t field_ix, std::string& field_value) {
        field_value.assign(m_row[field_ix], m_field_lengths[field_ix]);
    }

private:
    // Methods
    /**
     * Fetches the next row from the database server
     */
    void fetch_next_row() {
        m_row = mysql_fetch_row(m_query_result);
        if (nullptr != m_row) {
            m_field_lengths = mysql_fetch_lengths(m_query_result);
            m_num_fields = mysql_num_fields(m_query_result);
        }
    }

    // Variables
    MYSQL_RES* m_query_result;
    MYSQL_ROW m_row;
    unsigned int m_num_fields;
    unsigned long* m_field_lengths;
};

ServerStatus assign_new_job(ServerContext* ctx) {
    // status 8 = PENDING_REDUCER
    std::string query = "SELECT id, search_config FROM distributed_search_jobs WHERE status=8";
    if (0 != mysql_real_query(ctx->db, query.c_str(), query.length())) {
        std::cout << "SQL ERROR: " << mysql_error(ctx->db) << std::endl;
        return ServerStatus::FINISHING_ERROR;
    }

    std::map<int32_t, std::string> jobs;
    SQLIterator* it = new SQLIterator(ctx->db);
    std::string id;
    std::string config;
    while (it->contains_element()) {
        int32_t job_id;
        it->get_field_as_string(0, id);
        it->get_field_as_string(1, config);
        job_id = stoi(id);
        jobs[job_id] = std::move(config);
        it->get_next();
    }
    delete it;

    for (auto job = jobs.begin(); job != jobs.end(); ++job) {
        // set reducer_ready where pending_reducer and id=job_id
        std::stringstream ss;
        ss << "UPDATE distributed_search_jobs SET status=9, reducer_port=" << ctx->port
           << ", reducer_host=\"" << ctx->host << "\" WHERE status=8 and id=" << job->first;
        std::string update = ss.str();
        if (0 != mysql_real_query(ctx->db, update.c_str(), update.length())) {
            std::cout << "SQL ERROR: " << mysql_error(ctx->db) << std::endl;
            return ServerStatus::FINISHING_ERROR;
        }

        if (mysql_affected_rows(ctx->db) > 0) {
            std::cout << "Taking job: " << job->first << std::endl;
            ctx->job_id = job->first;

            msgpack::object_handle handle
                    = msgpack::unpack(job->second.data(), job->second.length());

            std::map<std::string, msgpack::type::variant> query_config = handle.get().convert();

            // TODO: initialize pipeline based off of query config
            ctx->p = new Pipeline(PipelineInputMode::INTRA_STAGE);
            ctx->p->add_pipeline_stage(std::shared_ptr<Operator>(new CountOperator()));

            std::string collection_name = query_config["results_collection_name"].as_string();
            ctx->results_collection = mongocxx::collection(ctx->results_database[collection_name]);

            return ServerStatus::RUNNING;
        }
    }

    return ServerStatus::IDLE;
}

int set_job_done(ServerContext* ctx, int status = 3) {
    std::stringstream ss;
    ss << "UPDATE distributed_search_jobs SET status=" << status << " WHERE id=\"" << ctx->job_id
       << "\"";
    std::string update = ss.str();
    if (0 != mysql_real_query(ctx->db, update.c_str(), update.length())) {
        std::cout << "SQL ERROR: " << mysql_error(ctx->db) << std::endl;
        return -1;
    }
    return 0;
}

ServerStatus poll_for_job_done(ServerContext* ctx) {
    std::string query = "SELECT status FROM distributed_search_jobs WHERE id=" + std::to_string(ctx->job_id);
    if (0 != mysql_real_query(ctx->db, query.c_str(), query.length())) {
        std::cout << "SQL ERROR: " << mysql_error(ctx->db) << std::endl;
        return ServerStatus::FINISHING_ERROR;
    }

    // should only be one result, but iterate over all results
    // to make sure we don't break the connection
    int status = -1;
    SQLIterator* it = new SQLIterator(ctx->db);
    std::string status_str;
    while (it->contains_element()) {
        it->get_field_as_string(0, status_str);
        status = stoi(status_str);
        it->get_next();
    }
    delete it;

    if (status == -1) {
        return ServerStatus::FINISHING_ERROR;
    }

    // check if waiting for reducer to push results
    if (status == 10) {
        return ServerStatus::FINISHING_SUCCESS;
    }

    // if not either waiting to run or running we must have failed
    // given that we already checked success codes
    if (!(status == 1 || status == 9)) {
        return ServerStatus::FINISHING_ERROR;
    }

    return ServerStatus::RUNNING;
}

void poll_db(
        boost::system::error_code const& e,
        ServerContext* ctx,
        boost::asio::steady_timer* poll_timer
) {
    if (ctx->status == ServerStatus::IDLE) {
        ctx->status = assign_new_job(ctx);
    } else if (ctx->status == ServerStatus::RUNNING) {
        ctx->status = poll_for_job_done(ctx);
    }

    if (ctx->status == ServerStatus::IDLE || ctx->status == ServerStatus::RUNNING) {
        poll_timer->expires_at(poll_timer->expiry() + boost::asio::chrono::milliseconds(ctx->polling_interval_ms));
        poll_timer->async_wait(
                boost::bind(poll_db, boost::asio::placeholders::error, ctx, poll_timer)
        );
    } else {
        std::cout << "Closing acceptor socket in state " << ctx->status << std::endl;
        ctx->acceptor.close();
    }
}

int connect_to_db(
        ServerContext* ctx,
        std::string const& host,
        int port,
        std::string const& username,
        std::string const& password,
        std::string const& database
) {
    ctx->db = mysql_init(nullptr);
    auto result = mysql_real_connect(
            ctx->db,
            host.c_str(),
            username.c_str(),
            password.c_str(),
            database.c_str(),
            port,
            nullptr,
            CLIENT_COMPRESS
    );
    if (result == nullptr) {
        return -1;
    }
    return 0;
}

int main(int argc, char const* argv[]) {
    // mongocxx instance must be created before and destroyed after
    // all other mongocxx classes
    mongocxx::instance inst;

    std::string host = "127.0.0.1";
    int port = 14'009;
    std::string db_host = "127.0.0.1";
    int db_port = 3306;
    std::string db_user = "clp-user";
    std::string db_pass = "password";
    std::string database = "clp-db";
    std::string mongodb_database = "clp-search";
    std::string mongodb_uri = "mongodb://localhost:27017/";
    std::string mongodb_collection = "results";
    int polling_interval_ms = 100;

    po::options_description arguments("Arguments");
    arguments.add_options()
        ("host", po::value<std::string>(&host)->default_value("127.0.0.1"))
        ("port", po::value<int>(&port)->default_value(14'009))
        ("db-host", po::value<std::string>(&db_host)->default_value("127.0.0.1"))
        ("db-port", po::value<int>(&db_port)->default_value(3306))
        ("db-user", po::value<std::string>(&db_user)->default_value("clp-user"))
        ("db-pass", po::value<std::string>(&db_pass)->default_value("password"))
        ("database", po::value<std::string>(&database)->default_value("clp-db"))
        ("mongodb-database", po::value<std::string>(&mongodb_database)->default_value("clp-search"))
        ("mongodb-uri", po::value<std::string>(&mongodb_uri)->default_value("mongodb://localhost:27017/"))
        ("mongodb-collection", po::value<std::string>(&mongodb_collection)->default_value("results"))
        ("polling-interval-ms", po::value<int>(&polling_interval_ms)->default_value(100))
        ;

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, arguments), opts);

    try {
        po::notify(opts);
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    ServerContext ctx(host, port, mongodb_collection, polling_interval_ms);

    if (connect_to_db(&ctx, db_host, db_port, db_user, db_pass, database) < 0) {
        std::cout << "Failed to connect to the database... exiting" << db_user << " " << db_pass
                  << " " << database << std::endl;
        return -1;
    }

    mongocxx::client client;
    // if not performing aggregation connect to mongodb
    try {
        client = mongocxx::client(mongocxx::uri(mongodb_uri));
        ctx.results_database = mongocxx::database(client[mongodb_database]);
    } catch (mongocxx::exception& e) {
        std::cout << "Failed to connect to the MongoDB database" << std::endl;
        ;
        return -1;
    }
    // Polling interval
    boost::asio::steady_timer poll_timer(ctx.ioctx, boost::asio::chrono::milliseconds(ctx.polling_interval_ms));

    std::cout << "Starting on host " << ctx.host << " port " << port << " listening successfully " << ctx.acceptor.is_open() << std::endl;

    // Job acquisition loop
    while (true) {
        // Queue up polling and tcp accepting
        poll_timer.async_wait(
                boost::bind(poll_db, boost::asio::placeholders::error, &ctx, &poll_timer)
        );
        queue_accept_task(&ctx);

        std::cout << "Waiting for job..." << std::endl;

        // Start running the event processing loop
        ctx.ioctx.run();

        if (ctx.status == ServerStatus::FINISHING_SUCCESS) {
            std::cout << "Job " << ctx.job_id << " finished successfully, dumping results"
                      << std::endl;
            std::vector<std::vector<uint8_t>> results;
            std::vector<bsoncxx::document::view> result_documents;
            for (auto group_it = ctx.p->finish(); !group_it->done(); group_it->next()) {
                results.push_back(serialize(*group_it->get(), nlohmann::json::to_bson));
                std::vector<uint8_t>& encoded_result = results.back();
                result_documents.push_back(
                        bsoncxx::document::view(encoded_result.data(), encoded_result.size())
                );
            }

            if (result_documents.size() > 0) {
                ctx.results_collection.insert_many(result_documents);
            }

            if (set_job_done(&ctx) == -1) {
                return -1;
            }
        } else {
            std::cout << "Job " << ctx.job_id << " finished unsuccessfully" << std::endl;
            int rc = 0;
            if (ctx.status == ServerStatus::FINISHING_ERROR) {
                rc = set_job_done(&ctx, /*FAILED*/ 5);
            } else {
                rc = set_job_done(&ctx, /*CANCELLED*/ 7);
            }

            if (rc == -1) {
                return -1;
            }
        }

        // cleanup
        ctx.ioctx.reset();
        delete ctx.p;
        ctx.p = nullptr;
        ctx.status = ServerStatus::IDLE;
        ctx.job_id = -1;
        ctx.acceptor
                = boost::asio::ip::tcp::acceptor(ctx.ioctx, tcp::endpoint(tcp::v6(), ctx.port));
    }

    mysql_close(ctx.db);
}
