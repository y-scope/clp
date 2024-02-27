#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <mongocxx/instance.hpp>
#include <msgpack.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../clp/spdlog_with_specializations.hpp"
#include "CommandLineArguments.hpp"
#include "DeserializedRecordGroup.hpp"
#include "ServerContext.hpp"
#include "types.hpp"

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
    explicit RecordReceiverContext(std::shared_ptr<ServerContext> const& ctx)
            : ctx(ctx),
              socket(ctx->get_io_context()),
              buf_size(cDefaultBufSize),
              buf(new char[cDefaultBufSize]) {}

    ~RecordReceiverContext() {
        delete[] buf;
        socket.close();
    }

    // Disallow copy and move
    RecordReceiverContext(RecordReceiverContext const&) = delete;
    RecordReceiverContext(RecordReceiverContext const&&) = delete;
    RecordReceiverContext const& operator=(RecordReceiverContext const&) = delete;
    RecordReceiverContext const& operator=(RecordReceiverContext const&&) = delete;

    static std::shared_ptr<RecordReceiverContext> new_receiver(
            std::shared_ptr<ServerContext> const& ctx
    ) {
        // Note: we use shared instead of unique to make things work with boost::bind
        std::shared_ptr<RecordReceiverContext> receiver
                = std::make_shared<RecordReceiverContext>(ctx);
        // clear the v6_only flag to allow ipv4 and ipv6 connections
        // note this is linux-only -- for full portability need separate v4 and v6 acceptors
        // boost::asio::ip::v6_only option(false);
        // receiver->socket.set_option(option);

        return receiver;
    }

    /**
     * Reads a connection initiation packet.
     * @param num_bytes_read The number of new bytes read into the buffer.
     * @return false if there are an unexpected number of bytes in the buffer or the sender's job ID
     * doesn't match the one currently being processed.
     * @return true otherwise.
     */
    bool read_connection_init_packet(size_t num_bytes_read);

    /**
     * Sends a connection accept packet.
     * @return Whether the acceptance was sent successfully.
     */
    bool send_connection_accept_packet();

    /**
     * Reads a packet containing record groups.
     * @param num_bytes_read The number of new bytes read into the buffer.
     * @return Whether the read was successful.
     */
    bool read_record_groups_packet(size_t num_bytes_read);

    static constexpr size_t cMaxRecordSize = 16ULL * 1024 * 1024;
    static constexpr size_t cDefaultBufSize = 1024;

    std::shared_ptr<ServerContext> ctx;
    tcp::socket socket;
    char* buf;
    size_t buf_size;
    size_t bytes_occupied{0};
};

bool RecordReceiverContext::read_connection_init_packet(size_t num_bytes_read) {
    bytes_occupied += num_bytes_read;

    job_id_t job_id{0};

    if (bytes_occupied != sizeof(job_id)) {
        SPDLOG_ERROR("Rejecting connection due to invalid negotiation");
        return false;
    }

    memcpy(&job_id, buf, sizeof(job_id));
    if (job_id != ctx->get_job_id()) {
        SPDLOG_ERROR(
                "Rejecting connection from worker with job_id={} during processing of "
                "job_id={}",
                job_id,
                ctx->get_job_id()
        );
        return false;
    }
    bytes_occupied = 0;

    return true;
}

bool RecordReceiverContext::send_connection_accept_packet() {
    char const response = 'y';
    boost::system::error_code e;
    auto transferred
            = boost::asio::write(socket, boost::asio::buffer(&response, sizeof(response)), e);
    if (e || transferred < sizeof(response)) {
        SPDLOG_ERROR("Rejecting connection due to failure to send acceptance - {}", e.message());
        return false;
    }

    return true;
}

bool RecordReceiverContext::read_record_groups_packet(size_t num_bytes_read) {
    bytes_occupied += num_bytes_read;

    size_t record_size{0};
    auto* read_head = buf;
    while (bytes_occupied > 0) {
        if (bytes_occupied < sizeof(record_size)) {
            break;
        }
        memcpy(&record_size, read_head, sizeof(record_size));

        // terminate if record group size is over 16MB
        if (record_size >= cMaxRecordSize) {
            SPDLOG_ERROR("Record too large: {}B", record_size);
            return false;
        }

        if (bytes_occupied < record_size + sizeof(record_size)) {
            break;
        }
        read_head += sizeof(record_size);

        auto record_group = DeserializedRecordGroup{read_head, record_size};
        ctx->push_record_group(record_group.get_tags(), record_group.record_iter());
        bytes_occupied -= (record_size + sizeof(record_size));
        read_head += record_size;
    }

    if (bytes_occupied > 0) {
        if (buf_size < record_size + sizeof(record_size)) {
            auto new_buf = new char[record_size + sizeof(record_size)];
            memcpy(new_buf, read_head, bytes_occupied);
            delete[] buf;
            buf = new_buf;
        } else {
            memmove(buf, read_head, bytes_occupied);
        }
    }

    return true;
}

void queue_accept_task(std::shared_ptr<ServerContext> const& ctx);
void queue_receive_task(std::shared_ptr<RecordReceiverContext> const& rctx);
void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> const& rctx);
void queue_scheduler_update_listener_task(
        std::shared_ptr<ServerContext>& ctx,
        size_t current_buffer_occupancy
);

struct ReceiveTask {
    explicit ReceiveTask(std::shared_ptr<RecordReceiverContext> rctx) : rctx(std::move(rctx)) {}

    void operator()(boost::system::error_code const& error, size_t num_bytes_read) {
        // if no new bytes terminate
        if (0 == num_bytes_read || ServerStatus::Running != rctx->ctx->get_status()) {
            rctx->ctx->decrement_num_active_receiver_tasks();
            return;
        }

        if (false == rctx->read_record_groups_packet(num_bytes_read)) {
            rctx->ctx->decrement_num_active_receiver_tasks();
            return;
        }

        // Only queue another receive if the connection is still open
        if (false == error.failed()) {
            queue_receive_task(rctx);
        } else {
            rctx->ctx->decrement_num_active_receiver_tasks();
        }
    }

private:
    std::shared_ptr<RecordReceiverContext> rctx;
};

void queue_receive_task(std::shared_ptr<RecordReceiverContext> const& rctx) {
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
    explicit ValidateSenderTask(std::shared_ptr<RecordReceiverContext> rctx)
            : rctx(std::move(rctx)) {}

    void operator()(boost::system::error_code const& error, size_t num_bytes_read) {
        // if no new bytes terminate
        if ((0 == num_bytes_read && error.failed())
            || ServerStatus::Running != rctx->ctx->get_status())
        {
            SPDLOG_ERROR("Rejecting connection because of connection error");
            rctx->ctx->decrement_num_active_receiver_tasks();
            return;
        }

        if (false == rctx->read_connection_init_packet(num_bytes_read)) {
            rctx->ctx->decrement_num_active_receiver_tasks();
            return;
        }
        if (false == rctx->send_connection_accept_packet()) {
            rctx->ctx->decrement_num_active_receiver_tasks();
            return;
        }

        queue_receive_task(rctx);
    }

private:
    std::shared_ptr<RecordReceiverContext> rctx;
};

void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> const& rctx) {
    rctx->ctx->increment_num_active_receiver_tasks();
    boost::asio::async_read(
            rctx->socket,
            boost::asio::buffer(rctx->buf, sizeof(job_id_t)),
            ValidateSenderTask(rctx)
    );
}

struct AcceptTask {
    AcceptTask(std::shared_ptr<ServerContext> ctx, std::shared_ptr<RecordReceiverContext> rctx)
            : ctx(std::move(ctx)),
              rctx(std::move(rctx)) {}

    void operator()(boost::system::error_code const& error) {
        if (false == error.failed() && ServerStatus::Running == ctx->get_status()) {
            queue_validate_sender_task(rctx);
            queue_accept_task(ctx);
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
            queue_accept_task(ctx);
        }
    }

private:
    std::shared_ptr<ServerContext> ctx;
    std::shared_ptr<RecordReceiverContext> rctx;
};

void queue_accept_task(std::shared_ptr<ServerContext> const& ctx) {
    auto rctx = RecordReceiverContext::new_receiver(ctx);
    ctx->get_tcp_acceptor().async_accept(rctx->socket, AcceptTask(ctx, rctx));
}

struct PeriodicUpsertTask {
    explicit PeriodicUpsertTask(std::shared_ptr<ServerContext> ctx) : ctx(std::move(ctx)) {}

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
            : ctx(std::move(ctx)),
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
        size_t size_header{0};
        if (current_buffer_occupancy < sizeof(size_header)) {
            queue_scheduler_update_listener_task(ctx, current_buffer_occupancy);
            return;
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
            return;
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
        std::shared_ptr<ServerContext>& ctx,
        size_t current_buffer_occupancy
) {
    boost::asio::async_read(
            ctx->get_scheduler_update_socket(),
            boost::asio::dynamic_buffer(ctx->get_scheduler_update_buffer()),
            boost::asio::transfer_at_least(1),  // Makes boost::asio forward results right away
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
        return 1;
    }

    // mongocxx instance must be created before and destroyed after all other mongocxx classes
    mongocxx::instance mongo_instance;

    reducer::CommandLineArguments args{"reducer-server"};
    auto parsing_result = args.parse_arguments(argc, argv);
    if (clp::CommandLineArgumentsBase::ParsingResult::Failure == parsing_result) {
        return 1;
    } else if (clp::CommandLineArgumentsBase::ParsingResult::InfoCommand == parsing_result) {
        return 0;
    }

    std::shared_ptr<reducer::ServerContext> ctx;
    try {
        ctx = std::make_shared<reducer::ServerContext>(args);
    } catch (reducer::ServerContext::OperationFailed& exception) {
        SPDLOG_CRITICAL("Failed to initialize - {}", exception.what());
        return 1;
    }

    SPDLOG_INFO("Starting on host {} port {}", ctx->get_reducer_host(), ctx->get_reducer_port());

    // Job acquisition loop
    while (true) {
        if (false == ctx->get_tcp_acceptor().is_open()) {
            SPDLOG_CRITICAL("Failed to bind acceptor socket");
            return 1;
        }
        SPDLOG_INFO("Acceptor socket listening successfully");

        // Connect to scheduler and register this reducer as available
        tcp::resolver resolver(ctx->get_io_context());
        auto endpoints = resolver.resolve(
                args.get_scheduler_host(),
                std::to_string(args.get_scheduler_port())
        );
        if (false == ctx->register_with_scheduler(endpoints)) {
            SPDLOG_CRITICAL("Failed to communicate with scheduler");
            return 1;
        }

        // Queue up listening for scheduler updates, and tcp accepting
        reducer::queue_scheduler_update_listener_task(ctx, 0);
        reducer::queue_accept_task(ctx);

        SPDLOG_INFO("Waiting for job...");

        // Run the event processing loop
        ctx->run();

        if (reducer::ServerStatus::ReceivedAllResults == ctx->get_status()) {
            SPDLOG_INFO("Job {} finished successfully", ctx->get_job_id());
            ctx->get_scheduler_update_socket().close();
        } else if (reducer::ServerStatus::RecoverableFailure == ctx->get_status()) {
            SPDLOG_ERROR("Job {} finished with a recoverable error", ctx->get_job_id());
        } else if ((reducer::ServerStatus::UnrecoverableFailure == ctx->get_status())) {
            SPDLOG_CRITICAL("Job {} finished with an unrecoverable error", ctx->get_job_id());
            return 1;
        } else {
            SPDLOG_CRITICAL(
                    "Job {} finished in unexpected state {}",
                    ctx->get_job_id(),
                    reducer::server_status_to_string(ctx->get_status())
            );
            return 1;
        }

        // Reset reducer state to prepare for next job
        ctx->reset();
    }
}
