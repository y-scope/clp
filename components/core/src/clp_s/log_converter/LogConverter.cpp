#include "LogConverter.hpp"

#include <cstddef>
#include <cstring>
#include <string_view>
#include <system_error>
#include <utility>

#include <log_surgeon/BufferParser.hpp>
#include <log_surgeon/Constants.hpp>
#include <log_surgeon/Schema.hpp>
#include <ystdlib/containers/Array.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../clp/ErrorCode.hpp"
#include "../../clp/ReaderInterface.hpp"
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
        R"(header:(?<timestamp>((\d{2,4}[ /\-]{0,1}[ 0-9]{2}[ /\-][ 0-9]{2})|([ 0-9]{2}[ /\-])"
        R"(((Jan(uary){0,1})|(Feb(ruary){0,1})|(Mar(ch){0,1})|(Apr(il){0,1})|(May)|(Jun(e){0,1})|)"
        R"((Jul(y){0,1})|(Aug(ust){0,1})|(Sep(tember){0,1})|(Oct(ober){0,1})|(Nov(ember){0,1})|)"
        R"((Dec(ember){0,1}))[ /\-]\d{2,4}))[ T:][ 0-9]{2}:[ 0-9]{2}:[ 0-9]{2})"
        R"(([,\.:]\d{1,9}){0,1}([ ]{0,1}(UTC){0,1}[\+\-]\d{2}(:{0,1}\d{2}){0,1}Z{0,1}){0,1}))"
};

constexpr std::string_view cDelimiters{R"(delimiters: \t\r\n[(:)"};
}  // namespace

auto LogConverter::convert_file(
        clp_s::Path const& path,
        clp::ReaderInterface* reader,
        std::string_view output_dir
) -> ystdlib::error_handling::Result<void> {
    log_surgeon::Schema schema;
    schema.add_delimiters(cDelimiters);
    schema.add_variable(cTimestampSchema, -1);
    log_surgeon::BufferParser parser{std::move(schema.release_schema_ast_ptr())};
    parser.reset();

    // Reset internal buffer state.
    m_parser_offset = 0ULL;
    m_num_bytes_buffered = 0ULL;

    auto serializer{YSTDLIB_ERROR_HANDLING_TRYX(LogSerializer::create(output_dir, path.path))};

    bool reached_end_of_stream{false};
    while (false == reached_end_of_stream) {
        auto const num_bytes_read{YSTDLIB_ERROR_HANDLING_TRYX(refill_buffer(reader))};
        reached_end_of_stream = 0ULL == num_bytes_read;

        while (m_parser_offset < m_num_bytes_buffered) {
            auto const err{parser.parse_next_event(
                    m_buffer.data(),
                    m_num_bytes_buffered,
                    m_parser_offset,
                    reached_end_of_stream
            )};
            if (log_surgeon::ErrorCode::BufferOutOfBounds == err) {
                break;
            }
            if (log_surgeon::ErrorCode::Success != err) {
                return std::errc::no_message;
            }

            auto const& event{parser.get_log_parser().get_log_event_view()};
            auto const message{event.to_string()};
            if (auto timestamp{event.get_timestamp()}; timestamp.has_value()) {
                auto const message_without_timestamp{
                        std::string_view{message}.substr(timestamp->length())
                };
                YSTDLIB_ERROR_HANDLING_TRYV(
                        serializer.add_message(timestamp.value(), message_without_timestamp)
                );
            } else {
                YSTDLIB_ERROR_HANDLING_TRYV(serializer.add_message(message));
            }
        }
    }
    serializer.close();
    return ystdlib::error_handling::success();
}

auto LogConverter::refill_buffer(clp::ReaderInterface* reader)
        -> ystdlib::error_handling::Result<size_t> {
    compact_buffer();
    YSTDLIB_ERROR_HANDLING_TRYV(grow_buffer_if_full());

    size_t num_bytes_read{};
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto const rc{reader->try_read(
            m_buffer.data() + m_num_bytes_buffered,
            m_buffer.size() - m_num_bytes_buffered,
            num_bytes_read
    )};
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    m_num_bytes_buffered += num_bytes_read;
    if (clp::ErrorCode_EndOfFile == rc) {
        return num_bytes_read;
    }
    if (clp::ErrorCode_Success != rc) {
        return std::errc::not_enough_memory;
    }

    return num_bytes_read;
}

void LogConverter::compact_buffer() {
    if (0 == m_parser_offset) {
        return;
    }

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::memmove(
            m_buffer.data(),
            m_buffer.data() + m_parser_offset,
            m_num_bytes_buffered - m_parser_offset
    );
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    m_num_bytes_buffered -= m_parser_offset;
    m_parser_offset = 0;
}

auto LogConverter::grow_buffer_if_full() -> ystdlib::error_handling::Result<void> {
    if (m_buffer.size() != m_num_bytes_buffered) {
        return ystdlib::error_handling::success();
    }

    size_t const new_size{2 * m_buffer.size()};
    if (new_size > cMaxBufferSize) {
        return std::errc::result_out_of_range;
    }
    ystdlib::containers::Array<char> new_buffer(new_size);
    std::memcpy(new_buffer.data(), m_buffer.data(), m_num_bytes_buffered);
    m_buffer = std::move(new_buffer);
    return ystdlib::error_handling::success();
}
}  // namespace clp_s::log_converter
