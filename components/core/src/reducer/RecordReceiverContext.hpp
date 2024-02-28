#ifndef REDUCER_RECORDRECEIVERCONTEXT_HPP
#define REDUCER_RECORDRECEIVERCONTEXT_HPP

#include <cstdint>
#include <memory>

#include <boost/asio/ip/tcp.hpp>

#include "ServerContext.hpp"

namespace reducer {
class RecordReceiverContext {
public:
    static constexpr size_t cMinBufSize = 1024;

    explicit RecordReceiverContext(std::shared_ptr<ServerContext> const& ctx)
            : m_server_ctx{ctx},
              m_socket{ctx->get_io_context()},
              m_buf(cMinBufSize) {}

    ~RecordReceiverContext() { m_socket.close(); }

    // Disallow copy and move
    RecordReceiverContext(RecordReceiverContext const&) = delete;
    RecordReceiverContext(RecordReceiverContext const&&) = delete;
    RecordReceiverContext const& operator=(RecordReceiverContext const&) = delete;
    RecordReceiverContext const& operator=(RecordReceiverContext const&&) = delete;

    static std::shared_ptr<RecordReceiverContext> new_receiver(
            std::shared_ptr<ServerContext> const& ctx
    ) {
        auto receiver = std::make_shared<RecordReceiverContext>(ctx);

        // Clear the v6_only flag to allow ipv4 and ipv6 connections, but only on Linux. For full
        // portability, we need separate v4 and v6 acceptors.
        // boost::asio::ip::v6_only option(false);
        // receiver->m_socket.set_option(option);

        return receiver;
    }

    /**
     * Reads a connection initiation packet.
     * @return false if there are an unexpected number of bytes in the buffer or the sender's job ID
     * doesn't match the one currently being processed.
     * @return true otherwise.
     */
    bool read_connection_init_packet();

    /**
     * Sends a connection accept packet.
     * @return Whether the acceptance was sent successfully.
     */
    bool send_connection_accept_packet();

    /**
     * Reads a packet containing record groups.
     * @return Whether the read was successful.
     */
    bool read_record_groups_packet();

    /**
     * NOTE: This method should only be called from handlers for boost::asio read tasks on the
     * receiver's socket.
     * @param num_bytes
     */
    void increment_buf_num_bytes_occupied(size_t num_bytes) {
        m_buf_num_bytes_occupied += num_bytes;
    }

    std::shared_ptr<ServerContext>& get_server_ctx() { return m_server_ctx; }

    boost::asio::ip::tcp::socket& get_socket() { return m_socket; }

    /**
     * @return A pointer to the next writable byte in the buffer.
     */
    char* get_buf_write_head() { return &m_buf[m_buf_num_bytes_occupied]; }

    size_t get_buf_num_bytes_avail() { return m_buf.size() - m_buf_num_bytes_occupied; }

private:
    static constexpr size_t cMaxRecordSize = 16ULL * 1024 * 1024;

    std::shared_ptr<ServerContext> m_server_ctx;
    boost::asio::ip::tcp::socket m_socket;
    std::vector<char> m_buf;
    size_t m_buf_num_bytes_occupied{0};
};
}  // namespace reducer
#endif  // REDUCER_RECORDRECEIVERCONTEXT_HPP
