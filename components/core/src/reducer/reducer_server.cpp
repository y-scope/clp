#include <cassert>
#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <mongocxx/instance.hpp>
#include <msgpack.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../clp/spdlog_with_specializations.hpp"
#include "CommandLineArguments.hpp"
#include "RecordGroupSerdes.hpp"
#include "ServerContext.hpp"

using boost::asio::ip::tcp;

namespace reducer {

namespace {
std::string server_status_to_string(ServerStatus status) {
    switch (status) {
        case ServerStatus::Idle:
            return "Idle";
        case ServerStatus::Running:
            return "Running";
        case ServerStatus::ReceivedAllResults:
            return "ReceivedAllResults";
        case ServerStatus::RecoverableFailure:
            return "RecoverableFailure";
        case ServerStatus::UnrecoverableFailure:
            return "UnrecoverableFailure";
        default:
            assert(0);
    }
    return "";
}
}  // namespace

struct RecordReceiverContext {
    RecordReceiverContext(std::shared_ptr<ServerContext> ctx)
            : ctx(ctx),
              socket(ctx->get_io_context()),
              buf_size(cDefaultBufSize),
              buf(new char[cDefaultBufSize]) {}

    ~RecordReceiverContext() {
        delete buf;
        socket.close();
    }

    // Disallow copy and move
    RecordReceiverContext(RecordReceiverContext const&) = delete;
    RecordReceiverContext(RecordReceiverContext const&&) = delete;
    RecordReceiverContext const& operator=(RecordReceiverContext const&) = delete;
    RecordReceiverContext const& operator=(RecordReceiverContext const&&) = delete;

    static std::shared_ptr<RecordReceiverContext> new_receiver(std::shared_ptr<ServerContext> ctx) {
        // Note: we use shared instead of unique to make things work with boost::bind
        std::shared_ptr<RecordReceiverContext> receiver
                = std::make_shared<RecordReceiverContext>(ctx);
        // clear the v6_only flag to allow ipv4 and ipv6 connections
        // note this is linux-only -- for full portability need separate v4 and v6 acceptors
        // boost::asio::ip::v6_only option(false);
        // receiver->socket.set_option(option);

        return receiver;
    }

    static constexpr size_t cDefaultBufSize = 1024;
    std::shared_ptr<ServerContext> ctx;
    tcp::socket socket;
    char* buf;
    size_t buf_size;
    size_t bytes_occupied{0};
};

void queue_accept_task(std::shared_ptr<ServerContext> ctx);
void queue_receive_task(std::shared_ptr<RecordReceiverContext> rctx);
void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> rctx);
void queue_scheduler_update_listener_task(
        std::shared_ptr<ServerContext> ctx,
        size_t current_buffer_occupancy
);

struct ReceiveTask {
    ReceiveTask(std::shared_ptr<RecordReceiverContext> rctx) : rctx(rctx) {}

    void operator()(boost::system::error_code const& error, size_t bytes_remaining) {
        size_t record_size = 0;
        char* buf_ptr = rctx->buf;

        // if no new bytes terminate
        if (0 == bytes_remaining || ServerStatus::Running != rctx->ctx->get_status()) {
            rctx->ctx->decrement_remaining_receiver_tasks();
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
            if (record_size >= cMaxRecordSize) {
                SPDLOG_ERROR("Record too large: {}B", record_size);
                rctx->ctx->decrement_remaining_receiver_tasks();
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
        } else {
            rctx->ctx->decrement_remaining_receiver_tasks();
        }
    }

private:
    static constexpr size_t cMaxRecordSize = 16ULL * 1024 * 1024;
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
        if ((0 == bytes_remaining && error.failed())
            || ServerStatus::Running != rctx->ctx->get_status())
        {
            SPDLOG_ERROR("Rejecting connection because of connection error");
            rctx->ctx->decrement_remaining_receiver_tasks();
            return;
        }

        // account for leftover bytes from previous call
        bytes_remaining += rctx->bytes_occupied;

        if (bytes_remaining > sizeof(int32_t)) {
            SPDLOG_ERROR("Rejecting connection because of invalid negotiation");
            rctx->ctx->decrement_remaining_receiver_tasks();
            return;
        } else if (bytes_remaining == sizeof(int32_t)) {
            int32_t job_id = 0;
            memcpy(&job_id, rctx->buf, sizeof(int32_t));
            if (job_id != rctx->ctx->get_job_id()) {
                SPDLOG_ERROR(
                        "Rejecting connection from worker with job_id={} during processing of "
                        "job_id={}",
                        job_id,
                        rctx->ctx->get_job_id()
                );
                rctx->ctx->decrement_remaining_receiver_tasks();
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
                rctx->ctx->decrement_remaining_receiver_tasks();
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
    rctx->ctx->increment_remaining_receiver_tasks();
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
        if (false == error.failed() && ServerStatus::Running == ctx->get_status()) {
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
                    "Rejecting connection while not in Running state, state={}",
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
    auto rctx = RecordReceiverContext::new_receiver(ctx);

    ctx->get_tcp_acceptor().async_accept(rctx->socket, AcceptTask(ctx, rctx));
}

struct PeriodicUpsertTask {
    PeriodicUpsertTask(std::shared_ptr<ServerContext> ctx) : ctx(ctx) {}

    void operator()(boost::system::error_code const& e) {
        if (ServerStatus::Running == ctx->get_status()) {
            if (false == ctx->upsert_timeline_results()) {
                ctx->set_status(ServerStatus::UnrecoverableFailure);
                ctx->stop_event_loop();
                return;
            }

            auto& upsert_timer = ctx->get_upsert_timer();
            upsert_timer.expires_from_now(std::chrono::milliseconds(ctx->get_polling_interval()));
            upsert_timer.async_wait(PeriodicUpsertTask(ctx));
        }
    }

private:
    std::shared_ptr<ServerContext> ctx;
};

struct SchedulerUpdateListenerTask {
    SchedulerUpdateListenerTask(std::shared_ptr<ServerContext> ctx, size_t current_buffer_occupancy)
            : ctx(ctx),
              current_buffer_occupancy(current_buffer_occupancy) {}

    void operator()(boost::system::error_code const& error, size_t bytes_read) {
        // This can include the scheduler closing the connection because the job has been cancelled
        if (0 == bytes_read && error.failed()) {
            SPDLOG_ERROR("Closing connection with scheduler due to connection error or shutdown");
            ctx->set_status(ServerStatus::RecoverableFailure);
            ctx->stop_event_loop();
            return;
        }

        current_buffer_occupancy += bytes_read;

        // Try to read the message from the scheduler
        size_t size_header;
        if (current_buffer_occupancy < sizeof(size_header)) {
            queue_scheduler_update_listener_task(ctx, current_buffer_occupancy);
        }
        auto& buffer = ctx->get_scheduler_update_buffer();
        memcpy(&size_header, buffer.data(), sizeof(size_header));

        if (size_header > cMaxMessageSize) {
            SPDLOG_ERROR("Message from scheduler too large {}B", size_header);
            ctx->set_status(ServerStatus::RecoverableFailure);
            ctx->stop_event_loop();
            return;
        }

        size_t total_message_size = sizeof(size_header) + size_header;
        if (current_buffer_occupancy < total_message_size) {
            queue_scheduler_update_listener_task(ctx, current_buffer_occupancy);
        }

        nlohmann::json message = nlohmann::json::from_msgpack(
                buffer.data() + sizeof(size_header),
                buffer.data() + sizeof(size_header) + size_header
        );
        buffer.clear();

        if (ServerStatus::Idle == ctx->get_status()) {
            ctx->set_up_pipeline(message);
            ctx->set_status(ServerStatus::Running);
            if (ctx->is_timeline_aggregation()) {
                auto& upsert_timer = ctx->get_upsert_timer();
                upsert_timer.expires_from_now(std::chrono::milliseconds(ctx->get_polling_interval())
                );
                upsert_timer.async_wait(PeriodicUpsertTask(ctx));
            }

            // Synchronously notify the scheduler that the reducer is ready
            if (false == ctx->ack_search_scheduler()) {
                ctx->set_status(ServerStatus::RecoverableFailure);
                ctx->stop_event_loop();
                return;
            }
        } else if (ServerStatus::Running == ctx->get_status()) {
            // Assuming for now that if we receive a message while in running state we know that it
            // is the "all results sent" message. No need to examine the contents.
            ctx->set_status(ServerStatus::ReceivedAllResults);

            // If there are no results still in flight this function will submit the final results
            // to the results cache and
            if (false == ctx->try_finalize_results()) {
                ctx->set_status(ServerStatus::RecoverableFailure);
                ctx->stop_event_loop();
                return;
            }

            ctx->get_tcp_acceptor().cancel();
        }

        if (ServerStatus::Running == ctx->get_status()) {
            queue_scheduler_update_listener_task(ctx, 0);
        }
    }

private:
    static constexpr size_t cMaxMessageSize = 16ULL * 1024 * 1024;
    std::shared_ptr<ServerContext> ctx;
    size_t current_buffer_occupancy;
};

void queue_scheduler_update_listener_task(
        std::shared_ptr<ServerContext> ctx,
        size_t current_buffer_occupancy
) {
    boost::asio::async_read(
            ctx->get_scheduler_update_socket(),
            boost::asio::dynamic_buffer(ctx->get_scheduler_update_buffer()),
            boost::asio::transfer_at_least(1),
            SchedulerUpdateListenerTask(ctx, current_buffer_occupancy)
    );
}
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

    SPDLOG_INFO("Starting on host {} port {}", ctx->get_reducer_host(), ctx->get_reducer_port());

    // Job acquisition loop
    while (true) {
        if (ctx->get_tcp_acceptor().is_open()) {
            SPDLOG_INFO("Acceptor socket listening successfully");
        } else {
            SPDLOG_CRITICAL("Failed to bind acceptor socket");
            return -1;
        }

        // connect to scheduler and register this reducer as available
        tcp::resolver resolver(ctx->get_io_context());
        auto endpoints = resolver.resolve(
                args.get_scheduler_host(),
                std::to_string(args.get_scheduler_port())
        );
        if (false == ctx->register_with_scheduler(endpoints)) {
            SPDLOG_CRITICAL("Failed to communicate with scheduler");
            return -1;
        }

        // Queue up listening for scheduler updates, and tcp accepting
        reducer::queue_scheduler_update_listener_task(ctx, 0);
        reducer::queue_accept_task(ctx);

        SPDLOG_INFO("Waiting for job...");

        // Start running the event processing loop
        ctx->run();

        if (reducer::ServerStatus::ReceivedAllResults == ctx->get_status()) {
            SPDLOG_INFO("Job {} finished successfully", ctx->get_job_id());
            ctx->get_scheduler_update_socket().close();
        } else if (reducer::ServerStatus::RecoverableFailure == ctx->get_status()) {
            SPDLOG_ERROR("Job {} finished with a recoverable error", ctx->get_job_id());
        } else if ((reducer::ServerStatus::UnrecoverableFailure == ctx->get_status())) {
            SPDLOG_CRITICAL("Job {} finished with an unrecoverable error", ctx->get_job_id());
            return -1;
        } else {
            SPDLOG_CRITICAL(
                    "Job {} finished in unexpected state {}",
                    ctx->get_job_id(),
                    ctx->get_status()
            );
            return -1;
        }

        // cleanup reducer state for next job
        ctx->reset();
    }
}
