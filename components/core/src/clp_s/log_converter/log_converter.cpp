#include <cstring>
#include <optional>
#include <string_view>

#include <curl/curl.h>
#include <log_surgeon/BufferParser.hpp>
#include <log_surgeon/Constants.hpp>
#include <log_surgeon/Schema.hpp>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <ystdlib/containers/Array.hpp>

#include "../../clp/NetworkReader.hpp"
#include "../../clp/ReaderInterface.hpp"
#include "CommandLineArguments.hpp"

using clp_s::log_converter::CommandLineArguments;

namespace {
/**
 * Basic timestamp schema which doesn't support month names, or day of week names.
 *
 * Once log-surgeon has better unicode support, we should also allow \u2202 as an alternative
 * minus sign for timezone offsets.
 */
constexpr std::string_view cTimestampSchema{
        R"(timestamp:\d{2,4}[ /\-]{0,1}[ 0-9]{2}[ /\-][ 0-9]{2}[ T:][ 0-9]{2}:[ 0-9]{2}:[ 0-9]{2})"
        R"(([,\.:]\d{1,9}){0,1}([ ]{0,1}(UTC){0,1}[\+\-]\d{2}(:{0,1}\d{2}){0,1}Z{0,1}){0,1})"
};

constexpr std::string_view cDelimeters{R"(delimiters: \t\r\n\[\(:)"};

auto check_and_log_curl_error(clp_s::Path const& path, std::shared_ptr<clp::ReaderInterface> reader)
        -> bool {
    if (auto network_reader = std::dynamic_pointer_cast<clp::NetworkReader>(reader);
        nullptr != network_reader)
    {
        if (auto const rc = network_reader->get_curl_ret_code();
            rc.has_value() && CURLcode::CURLE_OK != rc.value())
        {
            auto const curl_error_message = network_reader->get_curl_error_msg();
            SPDLOG_ERROR(
                    "Encountered curl error while converting {} - Code: {} - Message: {}",
                    path.path,
                    static_cast<int64_t>(rc.value()),
                    curl_error_message.value_or("Unknown error.")
            );
            return true;
        }
    }
    return false;
}

class LogConverter {
public:
    LogConverter() : m_buffer(cDefaultBufferSize) {}

    auto convert_file(clp_s::Path const& path, std::shared_ptr<clp::ReaderInterface>& reader)
            -> bool;

private:
    // Constants
    static constexpr size_t cDefaultBufferSize{64ULL * 1024ULL};  // 64 KiB
    static constexpr size_t cMaxBufferSize{64ULL * 1024ULL * 1024ULL};  // 64 MiB

    auto refill_buffer(std::shared_ptr<clp::ReaderInterface>& reader) -> std::optional<size_t>;

    ystdlib::containers::Array<char> m_buffer;
    size_t m_bytes_occupied{};
    size_t m_cur_offset{};
};

auto LogConverter::refill_buffer(std::shared_ptr<clp::ReaderInterface>& reader)
        -> std::optional<size_t> {
    if (m_cur_offset > 0) {
        std::memmove(
                m_buffer.data(),
                m_buffer.data() + m_cur_offset,
                m_bytes_occupied - m_cur_offset
        );
        m_bytes_occupied -= m_cur_offset;
        m_cur_offset = 0;
    }

    if (m_buffer.size() == m_bytes_occupied) {
        size_t new_size{2 * m_buffer.size()};
        if (new_size > cMaxBufferSize) {
            return std::nullopt;
        }
        ystdlib::containers::Array<char> new_buffer(new_size);
        std::memcpy(new_buffer.data(), m_buffer.data(), m_bytes_occupied);
        m_buffer = std::move(new_buffer);
    }

    size_t num_bytes_read{};
    auto const rc{reader->try_read(
            m_buffer.data() + m_cur_offset,
            m_buffer.size() - m_bytes_occupied,
            num_bytes_read
    )};
    m_bytes_occupied += num_bytes_read;
    if (clp::ErrorCode_EndOfFile == rc) {
        return num_bytes_read;
    }
    if (clp::ErrorCode_Success != rc) {
        return std::nullopt;
    }

    return num_bytes_read;
}

auto
LogConverter::convert_file(clp_s::Path const& path, std::shared_ptr<clp::ReaderInterface>& reader)
        -> bool {
    log_surgeon::Schema schema;
    schema.add_delimiters(cDelimeters);
    schema.add_variable(cTimestampSchema, -1);
    log_surgeon::BufferParser parser{std::move(schema.release_schema_ast_ptr())};

    bool reached_end_of_stream{false};
    while (false == reached_end_of_stream) {
        auto const num_bytes_read_option{refill_buffer(reader)};
        if (false == num_bytes_read_option.has_value()) {
            return false;
        }
        if (0 == num_bytes_read_option.value()) {
            reached_end_of_stream = true;
        }

        while (m_cur_offset < m_bytes_occupied) {
            auto const err{parser.parse_next_event(
                    m_buffer.data(),
                    m_bytes_occupied,
                    m_cur_offset,
                    reached_end_of_stream
            )};
            if (log_surgeon::ErrorCode::BufferOutOfBounds == err) {
                break;
            }
            if (log_surgeon::ErrorCode::Success != err) {
                return false;
            }

            auto const& event{parser.get_log_parser().get_log_event_view()};
            if (nullptr != event.get_timestamp()) {
                SPDLOG_INFO("ts: {}", event.get_timestamp()->to_string_view());
            }
        }
    }
    return m_cur_offset == m_bytes_occupied;
}

auto convert_files(CommandLineArguments const& command_line_arguments) -> bool {
    LogConverter log_converter;

    for (auto const& path : command_line_arguments.get_input_paths()) {
        auto reader{clp_s::try_create_reader(path, command_line_arguments.get_network_auth())};
        if (nullptr == reader) {
            return false;
        }

        auto [nested_readers, file_type] = clp_s::try_deduce_reader_type(reader);
        switch (file_type) {
            case clp_s::FileType::LogText:
                break;
            case clp_s::FileType::Json:
            case clp_s::FileType::KeyValueIr:
            case clp_s::FileType::Zstd:
            case clp_s::FileType::Unknown:
            default: {
                std::ignore = check_and_log_curl_error(path, reader);
                SPDLOG_ERROR("Received input that was not unstructured logtext from {}", path.path);
                return false;
            }
        }

        if (false == log_converter.convert_file(path, nested_readers.back())) {
            return false;
        }
    }

    return true;
}
}  // namespace

int main(int argc, char const* argv[]) {
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return 1;
    }

    CommandLineArguments command_line_arguments{"log-converter"};

    auto const parsing_result{command_line_arguments.parse_arguments(argc, argv)};
    switch (parsing_result) {
        case CommandLineArguments::ParsingResult::Success:
            break;
        case CommandLineArguments::ParsingResult::InfoCommand:
            return 0;
        case CommandLineArguments::ParsingResult::Failure:
        default:
            return 1;
    }

    if (false == convert_files(command_line_arguments)) {
        return 1;
    }
    return 0;
}
