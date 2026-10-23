#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/format.h>
#include <msgpack.hpp>

#include "../src/clp_s/Defs.hpp"
#include "../src/clp_s/ErrorCode.hpp"
#include "../src/clp_s/OutputHandlerImpl.hpp"

namespace {
using clp_s::ErrorCode;
using clp_s::NetworkOutputHandler;

constexpr std::string_view cDestination{"test-destination"};
constexpr std::string_view cSessionToken{"test-session-token"};
constexpr uint64_t cTaskIdx{7};
constexpr std::string_view cArchiveId{"test-archive-id"};
constexpr clp_s::epochtime_t cTimestampBase{1'700'000'000'000};
constexpr size_t cSmallMessageSize{16};
// Larger than a UNIX socket's default send buffer, so sending it takes several writes.
constexpr size_t cLargeMessageSize{4UL * 1024 * 1024};
constexpr uint64_t cNumResults{3};
// Bounds how long either end of a connection waits to receive, so a broken test fails instead of
// hanging.
constexpr std::chrono::seconds cReceiveTimeout{10};

/**
 * What the network destination received before the handler shut down its side of the connection.
 */
struct Received {
    std::string bytes;
    bool reached_eof{false};
};

/**
 * A connected pair of UNIX stream sockets standing in for the connection between a handler and its
 * network destination. The handler gets one end from its connector, and the test acts as the
 * destination on the other end.
 */
class SocketPair {
public:
    // Constructors
    SocketPair() {
        std::array<int, 2> fds{-1, -1};
        REQUIRE((0 == socketpair(AF_UNIX, SOCK_STREAM, 0, fds.data())));
        m_handler_fd = fds[0];
        m_destination_fd = fds[1];

        timeval const timeout{.tv_sec = cReceiveTimeout.count(), .tv_usec = 0};
        for (auto const fd : fds) {
            REQUIRE((0 == setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))));
        }
    }

    // Destructor
    ~SocketPair() {
        if (m_destination_thread.joinable()) {
            m_destination_thread.join();
        }
        if (-1 != m_handler_fd) {
            close(m_handler_fd);
        }
        if (-1 != m_destination_fd) {
            close(m_destination_fd);
        }
    }

    // Delete copy & move constructors and assignment operators
    SocketPair(SocketPair const&) = delete;
    SocketPair(SocketPair&&) = delete;
    auto operator=(SocketPair const&) -> SocketPair& = delete;
    auto operator=(SocketPair&&) -> SocketPair& = delete;

    // Methods
    /**
     * @return A handler whose connector hands it the handler's end of this pair. The handler must
     * be destroyed before this pair.
     */
    [[nodiscard]] auto make_handler() -> std::unique_ptr<NetworkOutputHandler> {
        return std::make_unique<NetworkOutputHandler>(
                [this]() -> int {
                    ++m_num_connections;
                    return std::exchange(m_handler_fd, -1);
                },
                std::string{cDestination},
                std::string{cSessionToken},
                cTaskIdx
        );
    }

    [[nodiscard]] auto get_num_connections() const -> size_t { return m_num_connections; }

    /**
     * Acts as the network destination in a background thread: receives until the handler shuts
     * down its side of the connection, then sends the given reply and closes the destination's
     * end.
     * @param reply
     */
    auto serve(std::string reply) -> void {
        m_destination_thread = std::thread{[this, reply{std::move(reply)}]() -> void {
            constexpr size_t cReceiveBufferSize{64UL * 1024};
            std::vector<char> buf(cReceiveBufferSize);
            while (true) {
                auto const num_bytes_received{recv(m_destination_fd, buf.data(), buf.size(), 0)};
                if (0 == num_bytes_received) {
                    m_received.reached_eof = true;
                    break;
                }
                if (0 > num_bytes_received) {
                    if (EINTR == errno) {
                        continue;
                    }
                    break;
                }
                m_received.bytes.append(buf.data(), static_cast<size_t>(num_bytes_received));
            }
            if (false == reply.empty()) {
                // The handler may have closed the connection already, so failures are ignored.
                send(m_destination_fd, reply.data(), reply.size(), MSG_NOSIGNAL);
            }
            close(m_destination_fd);
            m_destination_fd = -1;
        }};
    }

    /**
     * Waits for the destination started by `serve` to finish.
     * @return What the destination received.
     */
    [[nodiscard]] auto join() -> Received {
        m_destination_thread.join();
        return std::move(m_received);
    }

    /**
     * Closes the destination's end of the connection without receiving anything.
     */
    auto close_destination() -> void {
        close(m_destination_fd);
        m_destination_fd = -1;
    }

private:
    // Variables
    int m_handler_fd{-1};
    int m_destination_fd{-1};
    size_t m_num_connections{0};
    std::thread m_destination_thread;
    Received m_received;
};

/**
 * @param num_acked_results
 * @return A well-formed acknowledgement of the given number of results.
 */
[[nodiscard]] auto make_ack(uint64_t num_acked_results) -> std::string {
    constexpr size_t cNumBitsPerByte{8};
    constexpr uint64_t cByteMask{0xff};

    std::string ack{"\x91\xcf"};
    for (size_t i{0}; i < sizeof(uint64_t); ++i) {
        auto const shift{(sizeof(uint64_t) - 1 - i) * cNumBitsPerByte};
        ack.push_back(static_cast<char>((num_acked_results >> shift) & cByteMask));
    }
    return ack;
}

[[nodiscard]] auto make_message(uint64_t result_idx, size_t message_size) -> std::string {
    auto message{fmt::format("message-{}", result_idx)};
    if (message.size() < message_size) {
        message.resize(message_size, 'x');
    }
    return message;
}

[[nodiscard]] auto make_timestamp(uint64_t result_idx) -> clp_s::epochtime_t {
    return cTimestampBase + static_cast<clp_s::epochtime_t>(result_idx);
}

/**
 * Writes results to the handler, requiring each write to succeed.
 * @param handler
 * @param num_results
 * @param message_size
 */
auto write_results(NetworkOutputHandler& handler, uint64_t num_results, size_t message_size)
        -> void {
    for (uint64_t result_idx{0}; result_idx < num_results; ++result_idx) {
        REQUIRE(
                (ErrorCode::ErrorCodeSuccess
                 == handler.write(
                         make_message(result_idx, message_size),
                         make_timestamp(result_idx),
                         cArchiveId,
                         static_cast<int64_t>(result_idx)
                 ))
        );
    }
}

/**
 * @tparam Fields
 * @param object
 * @return The fields of the given msgpack array, after requiring that it has as many fields.
 */
template <typename... Fields>
[[nodiscard]] auto as_tuple(msgpack::object const& object) -> std::tuple<Fields...> {
    REQUIRE((msgpack::type::ARRAY == object.type));
    REQUIRE((sizeof...(Fields) == object.via.array.size));
    return object.as<std::tuple<Fields...>>();
}

/**
 * Requires that the given bytes are exactly the handshake, the results written by `write_results`,
 * and the trailer.
 * @param bytes
 * @param num_results
 * @param message_size
 */
auto require_messages(std::string_view bytes, uint64_t num_results, size_t message_size) -> void {
    std::vector<msgpack::object_handle> messages;
    size_t offset{0};
    while (offset < bytes.size()) {
        messages.emplace_back(msgpack::unpack(bytes.data(), bytes.size(), offset));
    }
    REQUIRE((num_results + 2 == messages.size()));

    auto const [protocol_version, session_token, task_idx, archive_id]
            = as_tuple<uint64_t, std::string, uint64_t, std::string>(messages.front().get());
    REQUIRE((NetworkOutputHandler::cProtocolVersion == protocol_version));
    REQUIRE((cSessionToken == session_token));
    REQUIRE((cTaskIdx == task_idx));
    REQUIRE((cArchiveId == archive_id));

    for (uint64_t result_idx{0}; result_idx < num_results; ++result_idx) {
        auto const [received_result_idx, timestamp, message]
                = as_tuple<uint64_t, clp_s::epochtime_t, std::string>(
                        messages.at(result_idx + 1).get()
                );
        REQUIRE((result_idx == received_result_idx));
        REQUIRE((make_timestamp(result_idx) == timestamp));
        REQUIRE((make_message(result_idx, message_size) == message));
    }

    auto const [num_trailer_results] = as_tuple<uint64_t>(messages.back().get());
    REQUIRE((num_results == num_trailer_results));
}
}  // namespace

TEST_CASE("clp-s-network-output-handler-no-results", "[clp-s][network-output-handler]") {
    SocketPair socket_pair;
    auto const handler{socket_pair.make_handler()};

    REQUIRE((ErrorCode::ErrorCodeSuccess == handler->finish()));
    REQUIRE((0 == socket_pair.get_num_connections()));
}

TEST_CASE("clp-s-network-output-handler-acknowledged", "[clp-s][network-output-handler]") {
    auto const [num_results, message_size] = GENERATE(
            table<uint64_t, size_t>(
                    {{1, cSmallMessageSize},
                     {cNumResults, cSmallMessageSize},
                     // Enough results that the result indices and the trailer's count need wider
                     // msgpack integers.
                     {300, cSmallMessageSize},
                     {2, cLargeMessageSize}}
            )
    );

    SocketPair socket_pair;
    socket_pair.serve(make_ack(num_results));
    auto const handler{socket_pair.make_handler()};
    write_results(*handler, num_results, message_size);
    REQUIRE((ErrorCode::ErrorCodeSuccess == handler->finish()));
    REQUIRE((1 == socket_pair.get_num_connections()));

    auto const received{socket_pair.join()};
    REQUIRE(received.reached_eof);
    require_messages(received.bytes, num_results, message_size);
}

TEST_CASE(
        "clp-s-network-output-handler-mismatched-acknowledgement",
        "[clp-s][network-output-handler]"
) {
    auto const num_acked_results = GENERATE(cNumResults - 1, cNumResults + 1, uint64_t{0});

    SocketPair socket_pair;
    socket_pair.serve(make_ack(num_acked_results));
    auto const handler{socket_pair.make_handler()};
    write_results(*handler, cNumResults, cSmallMessageSize);
    REQUIRE((ErrorCode::ErrorCodeFailureNetwork == handler->finish()));

    auto const received{socket_pair.join()};
    REQUIRE(received.reached_eof);
    require_messages(received.bytes, cNumResults, cSmallMessageSize);
}

TEST_CASE(
        "clp-s-network-output-handler-malformed-acknowledgement",
        "[clp-s][network-output-handler]"
) {
    auto const reply = GENERATE(
            // A different array header.
            std::string{"\x92"} + make_ack(cNumResults).substr(1),
            // A different integer marker.
            std::string{"\x91\xce"} + make_ack(cNumResults).substr(2),
            // A valid msgpack encoding of the count, but not the 10-byte one.
            std::string{"\x91\x03"},
            // A cut-off acknowledgement.
            make_ack(cNumResults).substr(0, 5),
            // No acknowledgement at all.
            std::string{}
    );

    SocketPair socket_pair;
    socket_pair.serve(reply);
    auto const handler{socket_pair.make_handler()};
    write_results(*handler, cNumResults, cSmallMessageSize);
    REQUIRE((ErrorCode::ErrorCodeFailureNetwork == handler->finish()));

    auto const received{socket_pair.join()};
    REQUIRE(received.reached_eof);
    require_messages(received.bytes, cNumResults, cSmallMessageSize);
}

TEST_CASE("clp-s-network-output-handler-peer-closes-early", "[clp-s][network-output-handler]") {
    auto const num_results_before_close = GENERATE(uint64_t{0}, cNumResults);

    SocketPair socket_pair;
    auto const handler{socket_pair.make_handler()};
    write_results(*handler, num_results_before_close, cSmallMessageSize);
    socket_pair.close_destination();

    // Sending to a closed peer must fail instead of raising `SIGPIPE`, which would kill the test.
    REQUIRE(
            (ErrorCode::ErrorCodeFailureNetwork
             == handler->write(
                     make_message(num_results_before_close, cSmallMessageSize),
                     make_timestamp(num_results_before_close),
                     cArchiveId,
                     0
             ))
    );
    // The failure is sticky, so `finish` doesn't send a trailer that skips the lost result.
    REQUIRE((ErrorCode::ErrorCodeFailureNetwork == handler->finish()));
    REQUIRE((1 == socket_pair.get_num_connections()));
}

TEST_CASE("clp-s-network-output-handler-connection-failure", "[clp-s][network-output-handler]") {
    size_t num_connection_attempts{0};
    NetworkOutputHandler handler{
            [&]() -> int {
                ++num_connection_attempts;
                errno = ECONNREFUSED;
                return -1;
            },
            std::string{cDestination},
            std::string{cSessionToken},
            cTaskIdx
    };
    auto const message{make_message(0, cSmallMessageSize)};
    REQUIRE(
            (ErrorCode::ErrorCodeFailureNetwork
             == handler.write(message, make_timestamp(0), cArchiveId, 0))
    );

    // The failure is sticky: later calls return the same error without connecting again.
    REQUIRE(
            (ErrorCode::ErrorCodeFailureNetwork
             == handler.write(message, make_timestamp(1), cArchiveId, 1))
    );
    REQUIRE((ErrorCode::ErrorCodeFailureNetwork == handler.finish()));
    REQUIRE((1 == num_connection_attempts));
}
