#include "OutputHandlerImpl.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <fmt/format.h>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <msgpack.hpp>
#include <spdlog/spdlog.h>

#include <clp/ErrorCode.hpp>
#include <clp_s/MongoDBUtils.hpp>
#include <clp_s/ResultsCacheUtils.hpp>

#include "../clp/networking/socket_utils.hpp"
#include "../reducer/CountOperator.hpp"
#include "../reducer/network_utils.hpp"
#include "../reducer/Record.hpp"
#include "archive_constants.hpp"
#include "search/OutputHandler.hpp"

using std::string;
using std::string_view;

namespace clp_s {
namespace {
constexpr uint32_t cNumHandshakeFields{4};
constexpr uint32_t cNumResultFields{3};
constexpr uint32_t cNumTrailerFields{1};

// The acknowledgement is a msgpack array holding one uint64, always encoded as `0x91 0xcf` followed
// by the integer in big-endian order.
constexpr uint8_t cAckArrayHeader{0x91};
constexpr uint8_t cAckUint64Marker{0xcf};
constexpr size_t cAckHeaderSize{2};
constexpr size_t cAckSize{cAckHeaderSize + sizeof(uint64_t)};

/**
 * Receives exactly enough bytes over the socket to fill the given buffer, retrying interrupted
 * receives.
 * @param fd
 * @param buf
 * @return ErrorCode_EndOfFile if the peer closes the connection before the buffer is filled.
 * @return Forwards `clp::networking::try_receive`'s return values on any other failure.
 * @return ErrorCode_Success otherwise.
 */
[[nodiscard]] auto receive_exactly(int fd, std::span<char> buf) -> clp::ErrorCode;

auto receive_exactly(int fd, std::span<char> buf) -> clp::ErrorCode {
    auto remaining_buf{buf};
    while (false == remaining_buf.empty()) {
        size_t num_bytes_received{};
        auto const error_code{clp::networking::try_receive(
                fd,
                remaining_buf.data(),
                remaining_buf.size(),
                num_bytes_received
        )};
        if (clp::ErrorCode_errno == error_code && EINTR == errno) {
            continue;
        }
        if (clp::ErrorCode_Success != error_code) {
            return error_code;
        }
        remaining_buf = remaining_buf.subspan(num_bytes_received);
    }
    return clp::ErrorCode_Success;
}
}  // namespace

auto FileOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) -> ErrorCode {
    static constexpr string_view cOrigFilePathPlaceholder{""};
    msgpack::type::tuple<epochtime_t, string, string, string, int64_t> const
            src(timestamp, message, cOrigFilePathPlaceholder, archive_id, log_event_idx);
    msgpack::pack(m_file_writer, src);
    return ErrorCodeSuccess;
}

NetworkOutputHandler::NetworkOutputHandler(
        string const& host,
        int port,
        string session_token,
        uint64_t task_idx
)
        : NetworkOutputHandler{
                  [host, port_str = std::to_string(port)]() -> int {
                      return clp::networking::connect_to_server(host, port_str);
                  },
                  fmt::format("{}:{}", host, port),
                  std::move(session_token),
                  task_idx
          } {}

auto NetworkOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        [[maybe_unused]] int64_t log_event_idx
) -> ErrorCode {
    msgpack::packer<msgpack::sbuffer> packer{m_buffer};
    if (-1 == m_socket_fd) {
        m_socket_fd = m_connector();
        if (-1 == m_socket_fd) {
            auto const error{std::error_code{errno, std::generic_category()}};
            SPDLOG_ERROR(
                    "Failed to connect to {} - ({}) {}",
                    m_destination,
                    error.value(),
                    error.message()
            );
            return ErrorCodeFailureNetwork;
        }
        packer.pack_array(cNumHandshakeFields);
        packer.pack_uint8(cProtocolVersion);
        packer.pack(m_session_token);
        packer.pack_uint64(m_task_idx);
        packer.pack(archive_id);
    }
    packer.pack_array(cNumResultFields);
    packer.pack_uint64(m_num_results_sent);
    packer.pack_int64(timestamp);
    packer.pack(message);
    if (auto const error_code{send_buffer()}; ErrorCodeSuccess != error_code) {
        return error_code;
    }
    ++m_num_results_sent;
    return ErrorCodeSuccess;
}

auto NetworkOutputHandler::write([[maybe_unused]] string_view message) -> ErrorCode {
    SPDLOG_ERROR("The network output handler requires each result's metadata.");
    return ErrorCodeUnsupported;
}

auto NetworkOutputHandler::finish() -> ErrorCode {
    if (-1 == m_socket_fd) {
        return ErrorCodeSuccess;
    }

    msgpack::packer<msgpack::sbuffer> packer{m_buffer};
    packer.pack_array(cNumTrailerFields);
    packer.pack_uint64(m_num_results_sent);
    if (auto const error_code{send_buffer()}; ErrorCodeSuccess != error_code) {
        return error_code;
    }

    if (0 != shutdown(m_socket_fd, SHUT_WR)) {
        auto const error{std::error_code{errno, std::generic_category()}};
        SPDLOG_ERROR(
                "Failed to shut down the connection to {} for writing - ({}) {}",
                m_destination,
                error.value(),
                error.message()
        );
        return ErrorCodeFailureNetwork;
    }

    if (auto const error_code{receive_ack()}; ErrorCodeSuccess != error_code) {
        return error_code;
    }

    close(m_socket_fd);
    m_socket_fd = -1;
    return ErrorCodeSuccess;
}

auto NetworkOutputHandler::send_buffer() -> ErrorCode {
    if (clp::ErrorCode_Success
        != clp::networking::try_send(m_socket_fd, m_buffer.data(), m_buffer.size()))
    {
        auto const error{std::error_code{errno, std::generic_category()}};
        SPDLOG_ERROR(
                "Failed to send search results to {} - ({}) {}",
                m_destination,
                error.value(),
                error.message()
        );
        m_buffer.clear();
        return ErrorCodeFailureNetwork;
    }
    m_buffer.clear();
    return ErrorCodeSuccess;
}

auto NetworkOutputHandler::receive_ack() const -> ErrorCode {
    std::array<char, cAckSize> ack{};
    auto const error_code{receive_exactly(m_socket_fd, ack)};
    if (clp::ErrorCode_EndOfFile == error_code) {
        SPDLOG_ERROR(
                "Failed to receive the acknowledgement from {} - connection closed by the peer.",
                m_destination
        );
        return ErrorCodeFailureNetwork;
    }
    if (clp::ErrorCode_Success != error_code) {
        auto const error{std::error_code{errno, std::generic_category()}};
        SPDLOG_ERROR(
                "Failed to receive the acknowledgement from {} - ({}) {}",
                m_destination,
                error.value(),
                error.message()
        );
        return ErrorCodeFailureNetwork;
    }

    auto const array_header{static_cast<uint8_t>(ack[0])};
    auto const uint64_marker{static_cast<uint8_t>(ack[1])};
    if (cAckArrayHeader != array_header || cAckUint64Marker != uint64_marker) {
        SPDLOG_ERROR(
                "Received a malformed acknowledgement from {} - unexpected header 0x{:02x}{:02x}.",
                m_destination,
                array_header,
                uint64_marker
        );
        return ErrorCodeFailureNetwork;
    }

    uint64_t num_acked_results{};
    for (auto const byte : std::span{ack}.subspan(cAckHeaderSize)) {
        num_acked_results = (num_acked_results << CHAR_BIT) | static_cast<uint8_t>(byte);
    }
    if (m_num_results_sent != num_acked_results) {
        SPDLOG_ERROR(
                "{} acknowledged {} results, but {} were sent.",
                m_destination,
                num_acked_results,
                m_num_results_sent
        );
        return ErrorCodeFailureNetwork;
    }
    return ErrorCodeSuccess;
}

ResultsCacheOutputHandler::ResultsCacheOutputHandler(
        string_view uri,
        string_view collection,
        uint64_t batch_size,
        std::optional<uint64_t> max_num_results,
        string_view dataset,
        bool should_output_timestamp
)
        : ::clp_s::search::OutputHandler{should_output_timestamp, true},
          m_batch_size{batch_size},
          m_max_num_results{max_num_results},
          m_dataset{dataset} {
    m_collection = connect_to_results_cache(uri, collection, m_client);
    m_insert_options.ordered(false);
    m_results.reserve(m_batch_size);
}

ErrorCode ResultsCacheOutputHandler::finish() {
    while (false == m_latest_results.empty()) {
        auto result = std::move(*m_latest_results.top());
        m_latest_results.pop();
        if (auto const error_code{append_result(std::move(result))}; ErrorCodeSuccess != error_code)
        {
            return error_code;
        }
    }

    if (false == m_results.empty() && false == insert_results()) {
        return ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCodeSuccess;
}

auto ResultsCacheOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) -> ErrorCode {
    if (false == m_max_num_results.has_value()) {
        return append_result(
                QueryResult{string_view{}, message, timestamp, archive_id, log_event_idx, m_dataset}
        );
    }
    if (m_latest_results.size() < m_max_num_results.value()) {
        m_latest_results.emplace(
                std::make_unique<QueryResult>(
                        string_view{},
                        message,
                        timestamp,
                        archive_id,
                        log_event_idx,
                        m_dataset
                )
        );
    } else if (m_latest_results.top()->timestamp < timestamp) {
        m_latest_results.pop();
        m_latest_results.emplace(
                std::make_unique<QueryResult>(
                        string_view{},
                        message,
                        timestamp,
                        archive_id,
                        log_event_idx,
                        m_dataset
                )
        );
    }
    return ErrorCode::ErrorCodeSuccess;
}

auto ResultsCacheOutputHandler::append_result(QueryResult result) -> ErrorCode {
    try {
        m_results.emplace_back(
                std::move(
                        bsoncxx::builder::basic::make_document(
                                bsoncxx::builder::basic::kvp(
                                        constants::results_cache::search::cId,
                                        bsoncxx::builder::basic::make_document(
                                                bsoncxx::builder::basic::kvp(
                                                        constants::results_cache::search::
                                                                cArchiveId,
                                                        std::move(result.archive_id)
                                                ),
                                                bsoncxx::builder::basic::kvp(
                                                        constants::results_cache::search::
                                                                cLogEventIdx,
                                                        result.log_event_idx
                                                )
                                        )
                                ),
                                bsoncxx::builder::basic::kvp(
                                        constants::results_cache::search::cOrigFilePath,
                                        std::move(result.original_path)
                                ),
                                bsoncxx::builder::basic::kvp(
                                        constants::results_cache::search::cMessage,
                                        std::move(result.message)
                                ),
                                bsoncxx::builder::basic::kvp(
                                        constants::results_cache::search::cTimestamp,
                                        result.timestamp
                                ),
                                bsoncxx::builder::basic::kvp(
                                        std::string{constants::results_cache::search::cDataset},
                                        std::move(result.dataset)
                                )
                        )
                )
        );
    } catch (mongocxx::exception const& e) {
        SPDLOG_ERROR("Failed to build search result - {}", e.what());
        return ErrorCodeFailureDbBulkWrite;
    }

    if (m_results.size() >= m_batch_size && false == insert_results()) {
        return ErrorCode::ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCode::ErrorCodeSuccess;
}

auto ResultsCacheOutputHandler::insert_results() -> bool {
    try {
        m_collection.insert_many(m_results, m_insert_options);
    } catch (mongocxx::bulk_write_exception const& exception) {
        if (false == contains_only_duplicate_key_write_errors(exception, m_results.size())) {
            SPDLOG_ERROR("Failed to insert search results - {}", exception.what());
            return false;
        }
    } catch (mongocxx::exception const& exception) {
        SPDLOG_ERROR("Failed to insert search results - {}", exception.what());
        return false;
    }
    m_results.clear();
    return true;
}

CountReducerOutputHandler::CountReducerOutputHandler(int reducer_socket_fd)
        : search::OutputHandler(false, false),
          m_reducer_socket_fd(reducer_socket_fd),
          m_pipeline(reducer::PipelineInputMode::InterStage) {
    m_pipeline.add_pipeline_stage(std::make_shared<reducer::CountOperator>());
}

auto CountReducerOutputHandler::write(string_view message) -> ErrorCode {
    m_pipeline.push_record(reducer::EmptyRecord{});
    return ErrorCode::ErrorCodeSuccess;
}

auto CountReducerOutputHandler::finish() -> ErrorCode {
    if (false
        == reducer::send_pipeline_results(m_reducer_socket_fd, std::move(m_pipeline.finish())))
    {
        return ErrorCode::ErrorCodeFailureNetwork;
    }
    return ErrorCode::ErrorCodeSuccess;
}

auto CountByTimeReducerOutputHandler::finish() -> ErrorCode {
    if (false
        == reducer::send_pipeline_results(
                m_reducer_socket_fd,
                std::make_unique<reducer::Int64Int64MapRecordGroupIterator>(
                        m_bucket_counts,
                        reducer::CountOperator::cRecordElementKey
                )
        ))
    {
        return ErrorCode::ErrorCodeFailureNetwork;
    }
    return ErrorCode::ErrorCodeSuccess;
}
}  // namespace clp_s
