#include "DecomposedQuery.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <log_surgeon/log_surgeon.hpp>
#include <log_surgeon/rust_compat.hpp>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ErrorCode.hpp>
#include <clp/ReaderInterface.hpp>
#include <clpp/ErrorCode.hpp>

namespace clpp {
namespace {
auto get_full_type_name(
        log_surgeon::CCapture const& cap,
        absl::flat_hash_map<uint32_t, log_surgeon::CCapture const> rules,
        absl::flat_hash_map<std::tuple<uint32_t, uint32_t>, log_surgeon::CCapture const>
                parent_captures
) -> std::vector<std::string>;

auto get_type_name(log_surgeon::CCapture const& cap) -> std::string;

auto get_type_name(log_surgeon::CCapture const& cap) -> std::string {
    if (0 == cap.capture_id) {
        return std::string{cap.variable_name.as_cpp_view()};
    }
    return std::string{cap.capture_name.as_cpp_view()};
}

auto get_full_type_name(
        log_surgeon::CCapture const& cap,
        absl::flat_hash_map<uint32_t, log_surgeon::CCapture const> rules,
        absl::flat_hash_map<std::tuple<uint32_t, uint32_t>, log_surgeon::CCapture const>
                parent_captures
) -> std::vector<std::string> {
    std::vector<std::string> types;
    auto const* cur{&cap};
    while (0 != cur->capture_id) {
        types.emplace_back(cap.capture_name.as_cpp_view());
        if (0 == cur->parent_id) {
            cur = &rules.at(cur->rule_id);
        } else {
            cur = &parent_captures.at({cur->rule_id, cur->parent_id});
        }
    }
    types.emplace_back(cur->variable_name.as_cpp_view());
    return types;
}
}  // namespace

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
        std::optional<std::string_view> rule_type,
        std::string_view query
) -> ystdlib::error_handling::Result<DecomposedQuery> {
    size_t parser_pos{0};
    auto event{parser.next_event(query, &parser_pos)};
    if (false == event.has_value()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }

    absl::flat_hash_map<uint32_t, log_surgeon::CCapture const> rules;
    absl::flat_hash_map<std::tuple<uint32_t, uint32_t>, log_surgeon::CCapture const>
            parent_captures;
    for (size_t i{0};; ++i) {
        auto const cap{event->get_non_leaf_capture(i)};
        if (false == cap.has_value()) {
            break;
        }
        if (0 == cap->capture_id) {
            rules.emplace(cap->rule_id, cap.value());
        } else {
            parent_captures.emplace(std::tuple{cap->rule_id, cap->capture_id}, cap.value());
        }
    }
    if (rule_type.has_value()
        && (1 != rules.size() || rule_type != rules.begin()->second.variable_name.as_cpp_view()))
    {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }

    DecomposedQuery decomposed_query;
    std::string log_type{};
    log_type.reserve(query.size());
    size_t query_pos{0};
    for (size_t i{0};; ++i) {
        auto const cap{event->get_leaf_capture(i)};
        if (false == cap.has_value()) {
            break;
        }

        decomposed_query.m_leaves.emplace_back(
                get_full_type_name(*cap, rules, parent_captures),
                cap->lexeme.as_cpp_view()
        );

        log_type.append(query.substr(query_pos, cap->start - query_pos));
        log_type.append(fmt::format("%{}%", get_type_name(*cap)));
        query_pos = cap->end;
    }
    log_type.append(query.substr(query_pos));
    decomposed_query.m_log_type = log_type;

    return decomposed_query;
}

// TODO clpp: need a way to get type names for parents
// atm this only works when the parent type only contains leaves (no nesting)
auto DecomposedQuery::process_query(
        log_surgeon::Schema const* schema,
        std::string_view type,
        std::string_view query
) -> ystdlib::error_handling::Result<DecomposedQuery> {
    auto const* search_result{log_surgeon::log_surgeon_search_by_named_type(
            schema,
            log_surgeon::CCharArray::from_string_view(type),
            log_surgeon::CCharArray::from_string_view(query)
    )};
    if (nullptr == search_result) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }

    DecomposedQuery decomposed_query;
    std::string log_type{};
    log_type.reserve(query.size());
    size_t query_pos{0};
    for (size_t i{0};; ++i) {
        auto cap{log_surgeon::log_surgeon_search_result_get_leaf_capture(search_result, i)};
        if (nullptr != cap.lexeme.pointer) {
            break;
        }

        decomposed_query.m_leaves.emplace_back(
                // get_full_type_name(*cap, rules, parent_captures),
                std::vector{get_type_name(cap)},
                cap.lexeme.as_cpp_view()
        );

        log_type.append(query.substr(query_pos, cap.start - query_pos));
        log_type.append(fmt::format("%{}%", get_type_name(cap)));
        query_pos = cap.end;
    }
    log_type.append(query.substr(query_pos));
    decomposed_query.m_log_type = log_type;

    return DecomposedQuery{};
}
}  // namespace clpp
