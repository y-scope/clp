#include "DecomposedQuery.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include <log_surgeon/log_surgeon.hpp>
#include <log_surgeon/rust_compat.hpp>
#include <spdlog/spdlog.h>

#include <clp/ErrorCode.hpp>
#include <clp/ReaderInterface.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
auto DecomposedQuery::create_log_surgeon_schema(
        std::shared_ptr<clp::ReaderInterface> const& schema_reader
) -> ystdlib::error_handling::Result<log_surgeon::Schema*> {
    std::string schema_contents{};
    constexpr size_t cBufSize{4096};
    std::array<char, cBufSize> buf{};
    size_t bytes_read{};
    while (true) {
        auto code{schema_reader->try_read(buf.data(), buf.size(), bytes_read)};
        if (clp::ErrorCode_EndOfFile == code) {
            break;
        }
        if (clp::ErrorCode_Success != code) {
            return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
        }
        schema_contents.append(buf.data(), bytes_read);
    }
    if (schema_contents.empty()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
    }

    auto* schema_builder{log_surgeon::log_surgeon_schema_builder_new()};
    std::istringstream stream(schema_contents);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line.starts_with("//")) {
            continue;
        }

        auto const colon_pos{line.find(':')};
        if (colon_pos != std::string::npos) {
            auto const name{line.substr(0, colon_pos)};
            auto const escaped_pattern{line.substr(colon_pos + 1)};
            std::string pattern{};
            for (size_t i{0}; i < escaped_pattern.size(); ++i) {
                auto const c{escaped_pattern[i]};
                if ('\\' != c) {
                    pattern += c;
                    continue;
                }
                auto const escaped_c{escaped_pattern[i + 1]};
                switch (escaped_c) {
                    case 'n':
                        pattern += '\n';
                        break;
                    case 'r':
                        pattern += '\r';
                        break;
                    case 't':
                        pattern += '\t';
                        break;
                    default:
                        pattern += c;
                        pattern += escaped_c;
                        break;
                }
                ++i;
            }
            if ("delimiters" == name) {
                log_surgeon::log_surgeon_schema_builder_set_delimiters(
                        schema_builder,
                        log_surgeon::CCharArray::from_string_view(pattern)
                );
            } else {
                if (nullptr
                    != log_surgeon::log_surgeon_schema_builder_add_rule_with_priority(
                            schema_builder,
                            0,
                            log_surgeon::CCharArray::from_string_view(name),
                            log_surgeon::CCharArray::from_string_view(pattern)
                    ))
                {
                    return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
                }
            }
        }
    }
    return log_surgeon::log_surgeon_schema_builder_build(schema_builder);
}

auto DecomposedQuery::process_query(
        log_surgeon::ParserHandle& parser,
        std::optional<std::string_view> type,
        std::string_view query
) -> std::optional<DecomposedQuery> {
    size_t parser_pos{0};
    // parser.next_event(query, &parser_pos)
    return std::nullopt;
}

auto DecomposedQuery::process_query(
        log_surgeon::Schema const* schema,
        std::string_view type,
        std::string_view query
) -> std::optional<DecomposedQuery> {
    SPDLOG_INFO("query type name {}", type);
    auto const* search_result{log_surgeon::log_surgeon_search_by_named_type(
            schema,
            log_surgeon::CCharArray::from_string_view(type),
            log_surgeon::CCharArray::from_string_view(query)
    )};
    if (nullptr == search_result) {
        return std::nullopt;
    }

    for (size_t i{0};; ++i) {
        auto leaf{log_surgeon::log_surgeon_search_result_get_leaf_capture(search_result, i)};
        if (nullptr != leaf.lexeme.pointer) {
            break;
        }
    }

    return DecomposedQuery{};
}
}  // namespace clpp
