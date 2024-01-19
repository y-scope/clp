
#include <chrono>
#include <map>

#include <boost/asio.hpp>
#include <mongocxx/instance.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../clp/spdlog_with_specializations.hpp"
#include "CommandLineArguments.hpp"
#include "RecordGroupSerdes.hpp"
#include "ServerContext.hpp"

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

void queue_accept_task(std::shared_ptr<ServerContext> ctx);
void queue_receive_task(std::shared_ptr<RecordReceiverContext> rctx);
void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> rctx);

struct ReceiveTask {
    ReceiveTask(std::shared_ptr<RecordReceiverContext> rctx) : rctx(rctx) {}

    void operator()(boost::system::error_code const& error, size_t bytes_remaining) {
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

private:
    std::shared_ptr<RecordReceiverContext> rctx;
};

void queue_receive_task(std::shared_ptr<RecordReceiverContext> rctx) {
    boost::asio::async_read(
            rctx->socket,
            boost::asio::buffer(
                    &rctx->buf[rctx->bytes_occupied],
                    rctx->buf_size - rctx->bytes_occupied
            ),
            ReceiveTask(rctx)
    );
}

struct ValidateSenderTask {
    ValidateSenderTask(std::shared_ptr<RecordReceiverContext> rctx) : rctx(rctx) {}

    void operator()(boost::system::error_code const& error, size_t bytes_remaining) {
        // if no new bytes terminate
        if (0 == bytes_remaining || error.failed()
            || ServerStatus::RUNNING != rctx->ctx->get_status())
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
            int transferred
                    = boost::asio::write(rctx->socket, boost::asio::buffer(&response, 1), e);
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

private:
    std::shared_ptr<RecordReceiverContext> rctx;
};

void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> rctx) {
    boost::asio::async_read(
            rctx->socket,
            boost::asio::buffer(
                    &rctx->buf[rctx->bytes_occupied],
                    sizeof(int32_t) - rctx->bytes_occupied
            ),
            ValidateSenderTask(rctx)
    );
}

struct AcceptTask {
    AcceptTask(std::shared_ptr<ServerContext> ctx, std::shared_ptr<RecordReceiverContext> rctx)
            : ctx(ctx),
              rctx(rctx) {}

    void operator()(boost::system::error_code const& error) {
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

private:
    std::shared_ptr<ServerContext> ctx;
    std::shared_ptr<RecordReceiverContext> rctx;
};

void queue_accept_task(std::shared_ptr<ServerContext> ctx) {
    auto rctx = RecordReceiverContext::NewReceiver(ctx);

    ctx->get_tcp_acceptor().async_accept(rctx->socket, AcceptTask(ctx, rctx));
}

struct PollDbTask {
    PollDbTask(std::shared_ptr<ServerContext> ctx, boost::asio::steady_timer* poll_timer)
            : ctx(ctx),
              poll_timer(poll_timer) {}

    void operator()(boost::system::error_code const& e) {
        if (ServerStatus::IDLE == ctx->get_status()) {
            ctx->set_status(ctx->take_job());
        } else if (ServerStatus::RUNNING == ctx->get_status()) {
            ctx->set_status(ctx->poll_job_done());
        }

        if (ServerStatus::RUNNING == ctx->get_status() && ctx->is_timeline_aggregation()) {
            ctx->set_status(ctx->upsert_timeline_results());
        }

        if (ServerStatus::IDLE == ctx->get_status() || ServerStatus::RUNNING == ctx->get_status()) {
            poll_timer->expires_at(
                    poll_timer->expiry()
                    + boost::asio::chrono::milliseconds(ctx->get_polling_interval())
            );
            poll_timer->async_wait(PollDbTask(ctx, poll_timer));
        } else {
            SPDLOG_INFO(
                    "Cancelling operations on acceptor socket in state {}",
                    server_status_to_string(ctx->get_status())
            );
            ctx->get_tcp_acceptor().cancel();
        }
    }

private:
    std::shared_ptr<ServerContext> ctx;
    boost::asio::steady_timer* poll_timer;
};
}  // namespace reducer

int main(int argc, char const* argv[]) {
    // Program-wide initialization
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return -1;
    }

    // mongocxx instance must be created before and destroyed after all other mongocxx classes
    mongocxx::instance inst;

    reducer::CommandLineArguments args("reducer-server");
    auto parsing_result = args.parse_arguments(argc, argv);
    if (clp::CommandLineArgumentsBase::ParsingResult::Failure == parsing_result) {
        SPDLOG_CRITICAL("Failed to parse arguments for reducer");
        return -1;
    } else if (clp::CommandLineArgumentsBase::ParsingResult::InfoCommand == parsing_result) {
        return 0;
    }

    std::shared_ptr<reducer::ServerContext> ctx;

    try {
        ctx = std::make_shared<reducer::ServerContext>(args);
    } catch (reducer::ServerContext::OperationFailed& exception) {
        SPDLOG_CRITICAL("Failed to initialize reducer on error {}", exception.what());
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
            SPDLOG_CRITICAL("Failed to bind acceptor socket");
            return -1;
        }

        // Queue up polling and tcp accepting
        polling_timer.async_wait(reducer::PollDbTask(ctx, &polling_timer));
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

            bool done_success = ctx->update_job_status(reducer::JobStatus::SUCCESS);
            bool metrics_done_success
                    = ctx->publish_reducer_job_metrics(reducer::JobStatus::SUCCESS);
            if (false == results_success || false == done_success || false == metrics_done_success)
            {
                SPDLOG_CRITICAL("Database operation failed");
                return -1;
            }
        } else {
            SPDLOG_INFO("Job {} finished unsuccesfully", ctx->get_job_id());
            bool done_success = true, metrics_done_success = true;
            if (reducer::ServerStatus::FINISHING_REDUCER_ERROR == ctx->get_status()) {
                done_success = ctx->update_job_status(reducer::JobStatus::FAILED);
                metrics_done_success = ctx->publish_reducer_job_metrics(reducer::JobStatus::FAILED);
            } else if (reducer::ServerStatus::FINISHING_CANCELLED == ctx->get_status()) {
                metrics_done_success
                        = ctx->publish_reducer_job_metrics(reducer::JobStatus::CANCELLED);
            } else if (reducer::ServerStatus::FINISHING_REMOTE_ERROR == ctx->get_status()) {
                metrics_done_success = ctx->publish_reducer_job_metrics(reducer::JobStatus::FAILED);
            }

            if (false == done_success || false == metrics_done_success) {
                SPDLOG_CRITICAL("Database operation failed");
                return -1;
            }
        }

        // cleanup reducer state for next job
        ctx->reset();
    }
}
