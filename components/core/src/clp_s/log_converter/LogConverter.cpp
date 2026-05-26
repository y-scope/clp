#include "LogConverter.hpp"

#include <cstddef>
#include <cstring>
#include <string_view>
#include <system_error>
#include <utility>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/containers/Array.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../../clp/ErrorCode.hpp"
#include "../../clp/ReaderInterface.hpp"
#include "../../clp/Utils.hpp"
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
        R"((Dec(ember){0,1}))[ /\-]\d{2,4}))[ T:][ 0-9]{2}:[ 0-9]{2}:[ 0-9]{2}([,\.:]\d{1,9}){0,1})"
        // Timezone matching:
        R"(((( UTC){0,1}([\+\-]\d{2}(:{0,1}\d{2}){0,1}){0,1}Z{0,1})|)"
        R"(((UTC){0,1}([\+\-]\d{2}(:{0,1}\d{2}){0,1}){0,1}Z{0,1})){0,1})|)"
        R"((( [\+\-]\d{2}(:{0,1}\d{2}){0,1}){0,1}Z{0,1})|)"
        R"((( Z){0,1}))"
};

constexpr std::string_view cDelimiters{R"(delimiters: \t\r\n[(:)"};
}  // namespace

auto LogConverter::convert_file(
        clp_s::Path const& path,
        clp::ReaderInterface* reader,
        std::string_view output_dir
) -> ystdlib::error_handling::Result<void> {
    std::string rule_text{"delimiter:"};
    rule_text += cDelimiters;
    rule_text += "\n";
    rule_text += cTimestampSchema;
    log_surgeon::ParserHandle parser{clp::load_parser_from_rule_text(rule_text)};

    // Reset internal buffer state.
    m_parser_offset = 0ULL;
    m_num_bytes_buffered = 0ULL;

    auto serializer{YSTDLIB_ERROR_HANDLING_TRYX(LogSerializer::create(output_dir, path.path))};

    bool reached_end_of_stream{false};
    while (false == reached_end_of_stream) {
        auto const num_bytes_read{YSTDLIB_ERROR_HANDLING_TRYX(refill_buffer(reader))};
        reached_end_of_stream = 0ULL == num_bytes_read;

        while (m_parser_offset < m_num_bytes_buffered) {
            log_surgeon::CCharArray view{m_buffer.data(), m_num_bytes_buffered};
            auto optional_event{parser.next_event(view, &m_parser_offset)};
            // No error handling for failures?
            if (false == optional_event.has_value()) {
                break;
            }

            auto const& event{optional_event.value()};;

            // Ideally the event would just have its timestamp accessible as event.get_timestamp()
            std::string message;
            std::string timestamp;
            size_t leaf_id{0};
            while (true) {
                auto optional_leaf{event.get_leaf_match(leaf_id)};
                if (false == optional_leaf.has_value()) {
                    break;
                }
                auto leaf{optional_leaf.value()};
                size_t const leaf_len{leaf.range.end - leaf.range.start};
                if (timestamp.empty()
                    && "header" == leaf.ffi_pointers.root_rule_name.as_cpp_view()
                    && "timestamp" == leaf.ffi_pointers.rule_name.as_cpp_view()) {
                    timestamp = std::string{m_buffer.data() + leaf.range.start, leaf_len};
                } else {
                  message += std::string{m_buffer.data() + leaf.range.start, leaf_len};
                }
                ++leaf_id;
            }
            if (false == timestamp.empty()) {
                YSTDLIB_ERROR_HANDLING_TRYV(serializer.add_message(timestamp, message));
                continue;
            }
            YSTDLIB_ERROR_HANDLING_TRYV(serializer.add_message(message));
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
    if (new_size > m_max_buffer_size) {
        return std::errc::result_out_of_range;
    }
    ystdlib::containers::Array<char> new_buffer(new_size);
    std::memcpy(new_buffer.data(), m_buffer.data(), m_num_bytes_buffered);
    m_buffer = std::move(new_buffer);
    return ystdlib::error_handling::success();
}
}  // namespace clp_s::log_converter
