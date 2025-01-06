#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <json/single_include/nlohmann/json.hpp>
#include <mongocxx/instance.hpp>
#include <msgpack.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../clp/spdlog_with_specializations.hpp"
#include "CommandLineArguments.hpp"
#include "RecordReceiverContext.hpp"
#include "ServerContext.hpp"
#include "types.hpp"

using boost::asio::ip::tcp;

namespace reducer { namespace {
class AcceptTask {
public:
    explicit AcceptTask(std::shared_ptr<RecordReceiverContext> ctx)
            : m_record_recv_ctx{std::move(ctx)} {}

    void operator()(boost::system::error_code const& error);

private:
    std::shared_ptr<RecordReceiverContext> m_record_recv_ctx;
};

class PeriodicUpsertTask {
public:
    explicit PeriodicUpsertTask(std::shared_ptr<ServerContext> ctx)
            : m_server_ctx{std::move(ctx)} {}

    void operator()(boost::system::error_code const& e);

private:
    std::shared_ptr<ServerContext> m_server_ctx;
};

class ReceiveTask {
public:
    explicit ReceiveTask(std::shared_ptr<RecordReceiverContext> ctx)
            : m_record_recv_ctx{std::move(ctx)} {}

    void operator()(boost::system::error_code const& error, size_t num_bytes_read);

private:
    std::shared_ptr<RecordReceiverContext> m_record_recv_ctx;
};

class SchedulerUpdateListenerTask {
public:
    SchedulerUpdateListenerTask(std::shared_ptr<ServerContext> ctx, size_t buf_num_bytes_occupied)
            : m_server_ctx{std::move(ctx)},
              m_buf_num_bytes_occupied{buf_num_bytes_occupied} {}

    void operator()(boost::system::error_code const& error, size_t num_bytes_read);

private:
    static constexpr size_t cMaxMessageSize = 16ULL * 1024 * 1024;

    std::shared_ptr<ServerContext> m_server_ctx;
    size_t m_buf_num_bytes_occupied;
};

class ValidateSenderTask {
public:
    explicit ValidateSenderTask(std::shared_ptr<RecordReceiverContext> ctx)
            : m_record_recv_ctx{std::move(ctx)} {}

    void operator()(boost::system::error_code const& error, size_t num_bytes_read);

private:
    std::shared_ptr<RecordReceiverContext> m_record_recv_ctx;
};

void queue_accept_task(std::shared_ptr<ServerContext> const& ctx);
void queue_receive_task(std::shared_ptr<RecordReceiverContext> const& ctx);
void queue_scheduler_update_listener_task(
        std::shared_ptr<ServerContext>& ctx,
        size_t current_buffer_occupancy
);
void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> const& ctx);
std::string server_status_to_string(ServerStatus status);

void AcceptTask::operator()(boost::system::error_code const& error) {
    auto& server_ctx = m_record_recv_ctx->get_server_ctx();

    if (false == error.failed() && ServerStatus::Running == server_ctx->get_status()) {
        queue_validate_sender_task(m_record_recv_ctx);
        queue_accept_task(server_ctx);
    } else if (error.failed() && boost::system::errc::operation_canceled == error.value()) {
        SPDLOG_INFO("Accept task cancelled");
    } else if (error.failed()) {
        // tcp acceptor socket was closed -- don't re-queue accept task
        SPDLOG_ERROR("TCP acceptor socket closed - {}", error.message());
        if (server_ctx->get_tcp_acceptor().is_open()) {
            server_ctx->get_tcp_acceptor().close();
        }
    } else {
        SPDLOG_WARN(
                "Rejecting connection while not in Running state, state={}",
                server_status_to_string(server_ctx->get_status())
        );
        queue_accept_task(server_ctx);
    }
}

void PeriodicUpsertTask::operator()([[maybe_unused]] boost::system::error_code const& e) {
    if (ServerStatus::Running != m_server_ctx->get_status()) {
        return;
    }

    if (false == m_server_ctx->upsert_timeline_results()) {
        m_server_ctx->set_status(ServerStatus::UnrecoverableFailure);
        m_server_ctx->stop_event_loop();
        return;
    }

    auto& upsert_timer = m_server_ctx->get_upsert_timer();
    upsert_timer.expires_after(std::chrono::milliseconds(m_server_ctx->get_upsert_interval()));
    upsert_timer.async_wait(PeriodicUpsertTask(m_server_ctx));
}

void ReceiveTask::operator()(boost::system::error_code const& error, size_t num_bytes_read) {
    auto& server_ctx = m_record_recv_ctx->get_server_ctx();

    if (0 == num_bytes_read || ServerStatus::Running != server_ctx->get_status()) {
        server_ctx->decrement_num_active_receiver_tasks();
        return;
    }
    m_record_recv_ctx->increment_buf_num_bytes_occupied(num_bytes_read);

    if (false == m_record_recv_ctx->read_record_groups_packet()) {
        server_ctx->decrement_num_active_receiver_tasks();
        return;
    }

    // Only queue another receive if the connection is still open
    if (false == error.failed()) {
        queue_receive_task(m_record_recv_ctx);
    } else {
        server_ctx->decrement_num_active_receiver_tasks();
    }
}

void SchedulerUpdateListenerTask::operator()(
        boost::system::error_code const& error,
        size_t num_bytes_read
) {
    // This can include the scheduler closing the connection because the job has been cancelled
    if (0 == num_bytes_read || error.failed()) {
        SPDLOG_ERROR("Closing connection with scheduler due to connection error or shutdown");
        m_server_ctx->set_status(ServerStatus::RecoverableFailure);
        m_server_ctx->stop_event_loop();
        return;
    }

    m_buf_num_bytes_occupied += num_bytes_read;

    auto& buf = m_server_ctx->get_scheduler_update_buffer();
    auto* buf_read_head = buf.data();

    // Try to read a message from the scheduler
    size_t header_size{0};
    if (m_buf_num_bytes_occupied < sizeof(header_size)) {
        queue_scheduler_update_listener_task(m_server_ctx, m_buf_num_bytes_occupied);
        return;
    }

    memcpy(&header_size, buf_read_head, sizeof(header_size));
    if (header_size > cMaxMessageSize) {
        SPDLOG_ERROR("Message from scheduler too large {}B", header_size);
        m_server_ctx->set_status(ServerStatus::RecoverableFailure);
        m_server_ctx->stop_event_loop();
        return;
    }
    buf_read_head += sizeof(header_size);

    auto packet_size = sizeof(header_size) + header_size;
    if (m_buf_num_bytes_occupied < packet_size) {
        queue_scheduler_update_listener_task(m_server_ctx, m_buf_num_bytes_occupied);
        return;
    }

    nlohmann::json message;
    try {
        message = nlohmann::json::from_msgpack(buf_read_head, buf_read_head + header_size);
    } catch (nlohmann::json::parse_error const& e) {
        SPDLOG_ERROR("Failed to parse message from scheduler - {}", e.what());
        m_server_ctx->set_status(ServerStatus::RecoverableFailure);
        m_server_ctx->stop_event_loop();
        return;
    }
    buf.clear();
    m_buf_num_bytes_occupied = 0;

    auto status = m_server_ctx->get_status();
    if (ServerStatus::Idle == status) {
        m_server_ctx->set_up_pipeline(message);
        m_server_ctx->set_status(ServerStatus::Running);

        if (m_server_ctx->is_timeline_aggregation()) {
            auto& upsert_timer = m_server_ctx->get_upsert_timer();
            upsert_timer.expires_after(std::chrono::milliseconds(m_server_ctx->get_upsert_interval()
            ));
            upsert_timer.async_wait(PeriodicUpsertTask(m_server_ctx));
        }

        // Synchronously notify the scheduler that the reducer is ready
        if (false == m_server_ctx->ack_query_scheduler()) {
            m_server_ctx->set_status(ServerStatus::RecoverableFailure);
            m_server_ctx->stop_event_loop();
            return;
        }

        queue_scheduler_update_listener_task(m_server_ctx, m_buf_num_bytes_occupied);
    } else if (ServerStatus::Running == status) {
        // For now, if we receive a message while in the running state, we assume it's the "all
        // results sent" message without examining its contents.
        m_server_ctx->set_status(ServerStatus::ReceivedAllResults);

        if (false == m_server_ctx->try_finalize_results()) {
            m_server_ctx->set_status(ServerStatus::RecoverableFailure);
            m_server_ctx->stop_event_loop();
            return;
        }

        m_server_ctx->get_tcp_acceptor().cancel();
    }
}

void ValidateSenderTask::operator()(boost::system::error_code const& error, size_t num_bytes_read) {
    auto& server_ctx = m_record_recv_ctx->get_server_ctx();

    if (0 == num_bytes_read || error.failed() || ServerStatus::Running != server_ctx->get_status())
    {
        if (error.failed()) {
            SPDLOG_ERROR("Rejecting connection due to connection error - {}", error.message());
        } else {
            SPDLOG_ERROR("Rejecting connection due to empty read");
        }
        server_ctx->decrement_num_active_receiver_tasks();
        return;
    }
    m_record_recv_ctx->increment_buf_num_bytes_occupied(num_bytes_read);

    if (false == m_record_recv_ctx->read_connection_init_packet()) {
        server_ctx->decrement_num_active_receiver_tasks();
        return;
    }
    if (false == m_record_recv_ctx->send_connection_accept_packet()) {
        server_ctx->decrement_num_active_receiver_tasks();
        return;
    }

    queue_receive_task(m_record_recv_ctx);
}

void queue_accept_task(std::shared_ptr<ServerContext> const& ctx) {
    auto rctx = RecordReceiverContext::new_receiver(ctx);
    ctx->get_tcp_acceptor().async_accept(rctx->get_socket(), AcceptTask(rctx));
}

void queue_receive_task(std::shared_ptr<RecordReceiverContext> const& ctx) {
    boost::asio::async_read(
            ctx->get_socket(),
            boost::asio::buffer(ctx->get_buf_write_head(), ctx->get_buf_num_bytes_avail()),
            ReceiveTask(ctx)
    );
}

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

void queue_validate_sender_task(std::shared_ptr<RecordReceiverContext> const& ctx) {
    ctx->get_server_ctx()->increment_num_active_receiver_tasks();
    static_assert(sizeof(job_id_t) <= RecordReceiverContext::cMinBufSize);
    boost::asio::async_read(
            ctx->get_socket(),
            boost::asio::buffer(ctx->get_buf_write_head(), sizeof(job_id_t)),
            // transfer_at_least(1) helps us avoid hanging when a broken client sends fewer than
            // sizeof(job_id_t) bytes
            boost::asio::transfer_at_least(1),
            ValidateSenderTask(ctx)
    );
}

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
}}  // namespace reducer

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
        boost::system::error_code e;
        auto endpoints = resolver.resolve(
                args.get_scheduler_host(),
                std::to_string(args.get_scheduler_port()),
                e
        );
        if (e) {
            SPDLOG_CRITICAL(
                    "Failed to resolve endpoints for {}:{} - {}",
                    args.get_scheduler_host(),
                    args.get_scheduler_port(),
                    e.message()
            );
            return 1;
        }
        if (false == ctx->register_with_scheduler(endpoints)) {
            SPDLOG_CRITICAL("Failed to communicate with scheduler");
            return 1;
        }

        // Queue up listening for scheduler updates, and tcp accepting
        reducer::queue_scheduler_update_listener_task(ctx, 0);
        reducer::queue_accept_task(ctx);

        SPDLOG_INFO("Waiting for job...");

        // Run the event processing loop
        try {
            ctx->run();
        } catch (boost::system::system_error const& e) {
            SPDLOG_CRITICAL("Failed to run event loop - {}", e.what());
            return 1;
        }

        if (reducer::ServerStatus::ReceivedAllResults == ctx->get_status()) {
            SPDLOG_INFO("Job {} finished successfully", ctx->get_job_id());
            (void)ctx->get_scheduler_update_socket().close(e);
            if (e) {
                SPDLOG_WARN("Error when closing connection to scheduler - {}", e.message());
            }
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
