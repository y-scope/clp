#include "LogConverter.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

#include <log_surgeon/log_surgeon.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/containers/Array.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ErrorCode.hpp>
#include <clp/ReaderInterface.hpp>
#include <clp_s/InputConfig.hpp>
#include <clp_s/log_converter/LogSerializer.hpp>

namespace clp_s::log_converter {
namespace {
constexpr std::string_view cDelimiters{R"(\ \t\r\n[(:)"};

/**
 * Non-exhaustive timestamp parsing specification which covers many common patterns.
 * Based on the CLP heuristic.
 *
 * Once log-surgeon has better unicode support, we should also allow \u2202 as an alternative
 * minus sign for timezone offsets.
 */
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define DATE R"(\d{2,4}[ /-]?[ \d]{2}[ /-]?[ \d]{2})"
#define MONTH \
    R"((Jan(uary)?)|(Feb(ruary)?)|(Mar(ch)?)|(Apr(il)?)|(May)|(Jun(e)?)|(Jul(y)?)|(Aug(ust)?)|(Sep(tember)?)|(Oct(ober)?)|(Nov(ember)?)|(Dec(ember)?))"
#define SEP R"([ T:-])"
#define TIME R"([ \d]{2}:[ \d]{2}:[ \d]{2}([,\.:]\d{1,9})?)"
#define OFFSET R"(([\+-]\d{2}(:?\d{2})?))"
#define ZONE R"(( ?UTC)?(()" OFFSET R"(?Z)|()" OFFSET R"(Z?)))"
#define PREFIX R"(^(?<timestamp>)"
#define SUFFIX R"()$)"
// NOLINTEND(cppcoreguidelines-macro-usage)
constexpr std::array cHeaderRulePatterns{
        PREFIX DATE SEP TIME ZONE SUFFIX,
        PREFIX R"([ \d]{2}[ /-])" MONTH R"([ /-]\d{2,4})" SEP TIME ZONE SUFFIX,
        PREFIX DATE R"([T ])" TIME SUFFIX,
        PREFIX DATE R"( {1,2})" TIME SUFFIX,
        PREFIX R"(\[)" DATE R"([T\- ])" TIME R"(\]?)" SUFFIX,
        PREFIX R"(\[)" DATE R"([T\- ])" TIME R"(\]?)" SUFFIX,
        PREFIX R"(\[\d{2}/[A-Z][a-z]{2}/\d{4}:)" TIME SUFFIX,
        PREFIX R"(\[\d{2}/\d{2}/\d{4}:)" TIME SUFFIX,
        PREFIX R"((\d{2}|\d{4})/\d{2}/\d{2} )" TIME SUFFIX,
        PREFIX R"(\d{2}\d{2}\d{2} [ 0-9]{2}:\d{2}:\d{2})" SUFFIX,
        PREFIX R"(\d{2} [A-Z][a-z]{2} \d{4} )" TIME SUFFIX,
        PREFIX R"([A-Z][a-z]{2} \d{2}, \d{4} [ 0-9]{2}:\d{2}:\d{2} [AP]M)" SUFFIX,
        PREFIX R"([A-Z][a-z]+ \d{2}, \d{4} \d{2}:\d{2})$)",
        PREFIX R"([A-Z][a-z]{2} [A-Z][a-z]{2} [ 0-9]{2} )" TIME R"( \d{4})" SUFFIX,
        PREFIX R"([A-Z][a-z]{2} \d{2} )" TIME SUFFIX,
        PREFIX R"(\d{2}-\d{2} )" TIME SUFFIX,
        PREFIX R"(<<<)" DATE R"( )" TIME SUFFIX
};
#undef DATE
#undef MONTH
#undef SEP
#undef TIME
#undef OFFSET
#undef ZONE
#undef PREFIX
#undef SUFFIX
}  // namespace

auto LogConverter::create(size_t max_buffer_size) -> LogConverter {
    auto* builder{log_surgeon::log_surgeon_parsing_spec_builder_new()};
    log_surgeon::log_surgeon_parsing_spec_builder_set_delimiters(
            builder,
            log_surgeon::CCharArray::from_string_view(cDelimiters)
    );
    for (auto const& header_pattern : cHeaderRulePatterns) {
        if (false
            == log_surgeon::log_surgeon_parsing_spec_builder_add_rule_with_priority(
                    builder,
                    0,
                    log_surgeon::CCharArray::from_string_view("header"),
                    log_surgeon::CCharArray::from_string_view(header_pattern)
            ))
        {
            throw std::runtime_error("failed to add header rule parsing spec");
        }
    }
    return LogConverter(max_buffer_size, log_surgeon_parsing_spec_builder_build(builder));
}

auto LogConverter::convert_file(
        clp_s::Path const& path,
        clp::ReaderInterface* reader,
        std::string_view output_dir,
        bool compress_converted_file
) -> ystdlib::error_handling::Result<void> {
    // Reset internal buffer state.
    m_parser_offset = 0ULL;
    m_num_bytes_buffered = 0ULL;

    auto serializer{YSTDLIB_ERROR_HANDLING_TRYX(
            LogSerializer::create(output_dir, path.path, compress_converted_file)
    )};

    bool reached_end_of_stream{false};
    while (false == reached_end_of_stream) {
        auto const num_bytes_read{YSTDLIB_ERROR_HANDLING_TRYX(refill_buffer(reader))};
        reached_end_of_stream = 0ULL == num_bytes_read;

        std::string_view const buf{m_buffer.data(), m_num_bytes_buffered};
        while (m_parser_offset < m_num_bytes_buffered) {
            size_t pos{m_parser_offset};
            auto event{m_parser.next_event(buf, &pos)};
            if (false == event.has_value()) {
                SPDLOG_ERROR("failed to parse buffer contents: '{}'", buf);
                return std::errc::not_supported;
            }

            // If log-surgeon succeeded to parse to the end of the buffer it is possible the log
            // event is truncated. Until log-surgeon has an API that handles reads we need to fill
            // the buffer and try again.
            if (false == reached_end_of_stream && pos == m_num_bytes_buffered) {
                break;
            }
            m_parser_offset = pos;

            auto const match{event->get_leaf_match(0)};
            if (false == match.has_value()) {
                YSTDLIB_ERROR_HANDLING_TRYV(serializer.add_message(buf.substr(0, m_parser_offset)));
            } else if ("timestamp" == match->ffi_pointers.rule_name.as_cpp_view()
                       && "header" == match->ffi_pointers.root_rule_name.as_cpp_view())
            {
                YSTDLIB_ERROR_HANDLING_TRYV(serializer.add_message(
                        match->ffi_pointers.lexeme.as_cpp_view(),
                        buf.substr(match->range.end, m_parser_offset - match->range.end)
                ));
            } else {
                SPDLOG_ERROR(
                        "found a non-timestamp leaf match '{}': '{}'",
                        match->ffi_pointers.rule_name.as_cpp_view(),
                        match->ffi_pointers.lexeme.as_cpp_view()
                );
                return std::errc::not_supported;
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
    if (new_size > m_max_buffer_size) {
        return std::errc::result_out_of_range;
    }
    ystdlib::containers::Array<char> new_buffer(new_size);
    std::memcpy(new_buffer.data(), m_buffer.data(), m_num_bytes_buffered);
    m_buffer = std::move(new_buffer);
    return ystdlib::error_handling::success();
}
}  // namespace clp_s::log_converter
