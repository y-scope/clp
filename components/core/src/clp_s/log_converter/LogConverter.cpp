#include "LogConverter.hpp"

#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <string_view>

#include <log_surgeon/BufferParser.hpp>
#include <log_surgeon/Constants.hpp>
#include <log_surgeon/Schema.hpp>
#include <ystdlib/containers/Array.hpp>

#include "../../clp/ErrorCode.hpp"
#include "../InputConfig.hpp"
#include "LogSerializer.hpp"

namespace clp_s::log_converter {
namespace {
/**
 * Non-exhaustive timestamp schema which covers many common patterns.
 *
 * Once log-surgeon has better unicode support, we should also allow \u2202 as an alternative
 * minus sign for timezone offsets.
 */
constexpr std::string_view cTimestampSchema{
        R"(timestamp:(\d{2,4}[ /\-]{0,1}[ 0-9]{2}[ /\-][ 0-9]{2})|([ 0-9]{2}[ /\-])"
        R"(((Jan(uary){0,1})|(Feb(ruary){0,1})|(Mar(ch){0,1})|(Apr(il){0,1})|(May)|(Jun(e){0,1})|)"
        R"((Jul(y){0,1})|(Aug(ust){0,1})|(Sep(tember){0,1})|(Oct(ober){0,1})|(Nov(ember){0,1})|)"
        R"((Dec(ember){0,1}))[ /\-]\d{2,4})[ T:][ 0-9]{2}:[ 0-9]{2}:[ 0-9]{2})"
        R"(([,\.:]\d{1,9}){0,1}([ ]{0,1}(UTC){0,1}[\+\-]\d{2}(:{0,1}\d{2}){0,1}Z{0,1}){0,1})"
};
constexpr std::string_view cDelimeters{R"(delimiters: \t\r\n\[\(:)"};
}  // namespace

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
            m_buffer.data() + m_bytes_occupied,
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

auto LogConverter::convert_file(
        clp_s::Path const& path,
        std::shared_ptr<clp::ReaderInterface>& reader,
        std::string_view output_dir
) -> bool {
    log_surgeon::Schema schema;
    schema.add_delimiters(cDelimeters);
    schema.add_variable(cTimestampSchema, -1);
    log_surgeon::BufferParser parser{std::move(schema.release_schema_ast_ptr())};

    auto serializer_option{LogSerializer::create(output_dir, path.path)};
    if (false == serializer_option.has_value()) {
        return false;
    }
    auto& serializer{serializer_option.value()};

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
            size_t event_start_offset{m_cur_offset};
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

            auto& event{parser.get_log_parser().get_log_event_view()};
            auto const message{event.to_string()};
            if (nullptr != event.get_timestamp()) {
                auto timestamp{event.get_timestamp()->to_string_view()};
                auto message_without_timestamp{
                        std::string_view{message}.substr(timestamp.length())
                };

                if (false == serializer.add_message(timestamp, message_without_timestamp)) {
                    return false;
                }
            } else {
                if (false == serializer.add_message(message)) {
                    return false;
                }
            }
        }
    }

    serializer.close();
    return true;
}
}  // namespace clp_s::log_converter
